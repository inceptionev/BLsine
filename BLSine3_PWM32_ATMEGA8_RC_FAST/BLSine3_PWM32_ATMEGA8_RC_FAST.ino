/*
  BLSine3-ATMEGA8
  Pinned for the Blueseries 12A ESC
  True sine wave brushless motor controller for low speed gimbal motor applications.
  
  TODO:
  extend the sine table to the edges safely
  make speed 0 a true shutoff
  mode switch to sensed trapezoidal commutation at high speeds
  rewind the motors :(
  low battery cut off
 */
 

//Arduino assignments
#define PHA_HI 4
#define PHA_LO 5
#define PHB_HI A5
#define PHB_LO A4
#define PHC_HI A3
#define PHC_LO 8
#define RCPIN 0

//Fast access assignment
#define PHA_HI_PORT PORTD
#define PHA_HI_PIN 4
#define PHA_LO_PORT PORTD
#define PHA_LO_PIN 5
#define PHB_HI_PORT PORTC
#define PHB_HI_PIN 5
#define PHB_LO_PORT PORTC
#define PHB_LO_PIN 4
#define PHC_HI_PORT PORTC
#define PHC_HI_PIN 3
#define PHC_LO_PORT PORTB
#define PHC_LO_PIN 0
#define RCPIN_PORT PORTD
#define RCPIN_PIN 0

int A_HI_ON;
int A_HI_OFF;
int A_LO_ON;
int A_LO_OFF;
int B_HI_ON;
int B_HI_OFF;
int B_LO_ON;
int B_LO_OFF;
int C_HI_ON;
int C_HI_OFF;
int C_LO_ON;
int C_LO_OFF;

boolean lastrc = LOW;
boolean thisrc = LOW;
unsigned long redgetime;
unsigned int rcpulse;


int PWMSTEPS = 32;
int SINESTEPS = 32;
int SINECOUNTER = 0;
int PWMCOUNTER = 0;
int ACB = 1; //anti-crowbar.  be careful if causes one of the triggers to exceed PWMSTEPS
int PHASESTEP = 10; //120 degrees as a fraction of SINESTEPS
int SINEMOD = SINESTEPS-ACB;
int SPEED = 0; //right now this is 6 bits (64 steps)
int SPEEDCOUNTER = 0;
///int SPEED2 = 0;  //this is for testing

char sinetable [32];

void arraysetup(void){
   sinetable[0]=15;  // Put 32 step 5 bit sine table into array.
   sinetable[1]=18;  
   sinetable[2]=21;
   sinetable[3]=24;
   sinetable[4]=26;
   sinetable[5]=28;
   sinetable[6]=29;
   sinetable[7]=30;
   sinetable[8]=31; 
   sinetable[9]=30;
   sinetable[10]=29;
   sinetable[11]=28;
   sinetable[12]=26;
   sinetable[13]=24;
   sinetable[14]=21;
   sinetable[15]=18;
   sinetable[16]=15;
   sinetable[17]=12;
   sinetable[18]=9;
   sinetable[19]=6;
   sinetable[20]=4;
   sinetable[21]=2;
   sinetable[22]=1;
   sinetable[23]=0;
   sinetable[24]=0;
   sinetable[25]=0;
   sinetable[26]=1;
   sinetable[27]=2;
   sinetable[28]=4;
   sinetable[29]=6;
   sinetable[30]=9;
   sinetable[31]=12;
 }

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(PHA_HI, OUTPUT); 
  pinMode(PHA_LO, OUTPUT); 
  pinMode(PHB_HI, OUTPUT); 
  pinMode(PHB_LO, OUTPUT); 
  pinMode(PHC_HI, OUTPUT); 
  pinMode(PHC_LO, OUTPUT);   

  arraysetup();
}

// the loop routine runs over and over again forever:
void loop() {
  SINECOUNTER = 0;
  SPEEDCOUNTER = 0;
  
  while(SINECOUNTER < SINESTEPS) {
    A_HI_OFF = sinetable[SINECOUNTER];
    A_LO_ON = A_HI_OFF+ACB;   
    
    B_HI_OFF = sinetable[((SINECOUNTER+PHASESTEP)%SINEMOD)];
    B_LO_ON = B_HI_OFF+ACB;
    
    C_HI_OFF = sinetable[((SINECOUNTER+PHASESTEP+PHASESTEP)%SINEMOD)];
    C_LO_ON = C_HI_OFF+ACB;
    
    SPEEDCOUNTER = SPEEDCOUNTER + SPEED;
    SINECOUNTER = SPEEDCOUNTER >> 6; 
    
    
   //process the rcpulse
   if(rcpulse > 976 && rcpulse < 2000) { //valid pulse
     SPEED = (rcpulse - 976) >> 4;
   }
   else {
     SPEED = 0;
   }

    PWMCOUNTER = 0;
    //all phases turn on at the start of a PWM cycle
    //remember that the HI switch is inverted
    if(SPEED !=0){ //shutoff
      PHA_HI_PORT &= ~_BV(PHA_HI_PIN);//digitalWrite(PHA_HI, LOW);
      PHB_HI_PORT &= ~_BV(PHB_HI_PIN);//digitalWrite(PHB_HI, LOW);
      PHC_HI_PORT &= ~_BV(PHC_HI_PIN);//digitalWrite(PHC_HI, LOW);
    }
    while(PWMCOUNTER < PWMSTEPS) {
      if(PWMCOUNTER==A_HI_OFF){
        PHA_HI_PORT |= _BV(PHA_HI_PIN);//digitalWrite(PHA_HI, HIGH);
      }
      if(PWMCOUNTER==A_LO_ON){
        PHA_LO_PORT |= _BV(PHA_LO_PIN);//digitalWrite(PHA_LO, HIGH);
      }
      if(PWMCOUNTER==B_HI_OFF){
        PHB_HI_PORT |= _BV(PHB_HI_PIN);//digitalWrite(PHB_HI, HIGH);
      }
      if(PWMCOUNTER==B_LO_ON){
        PHB_LO_PORT |= _BV(PHB_LO_PIN);//digitalWrite(PHB_LO, HIGH);
      }
      if(PWMCOUNTER==C_HI_OFF){
        PHC_HI_PORT |= _BV(PHC_HI_PIN);//digitalWrite(PHC_HI, HIGH);
      }
      if(PWMCOUNTER==C_LO_ON){
        PHC_LO_PORT |= _BV(PHC_LO_PIN);//digitalWrite(PHC_LO, HIGH);
      }
     
    PWMCOUNTER++;  //increment the PWM counter
    
    //check for RC pulse
    thisrc = digitalRead(RCPIN);
    if(!lastrc && thisrc){
      redgetime = micros();
    }
    if(lastrc && !thisrc){
      rcpulse = micros()-redgetime;
    }
    lastrc=thisrc;
    //
    
    }
    //all phases turn off at the end of a PWM cycle
    PHA_LO_PORT &= ~_BV(PHA_LO_PIN);//digitalWrite(PHA_LO, LOW);
    PHB_LO_PORT &= ~_BV(PHB_LO_PIN);//digitalWrite(PHB_LO, LOW);
    PHC_LO_PORT &= ~_BV(PHC_LO_PIN);//digitalWrite(PHC_LO, LOW);
      
    

  }
      
        
  
  //digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(1000);               // wait for a second
  //digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  //delay(1000);               // wait for a second
}

