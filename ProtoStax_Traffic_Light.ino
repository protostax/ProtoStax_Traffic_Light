/*************************************************** 
  ProtoStax Traffic Light Redux Demo
  This is a example sketch for an Traffic Light Demo for 
  two streams of traffic and pedestrian lights for two
  streams (including walk request buttons), using Arduino,
  ProtoStax for Arduino --> https://www.protostax.com/products/protostax-for-arduino
  and 
  ProtoStax for Breadboards/Custom --> https://www.protostax.com/products/protostax-for-breadboard

  It demonstrates the use of a State Machine to manage the timing of the traffic lights
  and the requests for walk signs
 
  Written by Sridhar Rajagopal for ProtoStax
  BSD license. All text above must be included in any redistribution
 */

#include <JC_Button.h>

#define RED1 A0
#define YELLOW1 A1
#define GREEN1 A2
#define DONTWALK1 A3
#define WALK1 A4

#define RED2 8
#define YELLOW2 7
#define GREEN2 6
#define DONTWALK2 5
#define WALK2 4

#define  WALKREQ1     2    //Connect a tactile button switch (or something similar)
                           //from Arduino pin 2 to ground.
#define  WALKREQ2     3    //Connect a tactile button switch (or something similar)
                           //from Arduino pin 3 to ground.

#define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.
#define LONG_PRESS 1000    //We define a "long press" to be 1000 milliseconds.

Button walkReqBtn1(WALKREQ1, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button walkReqBtn2(WALKREQ2, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button

//The list of possible states for the state machine. 
// This state machine has a fixed sequence of states, namely
// MS_GREEN_STATE --> TO_MS_YELLOW_STATE --> MS_YELLOW_STATE --> TO_MS_RED_STATE --> TO_MS_RED_STATE --> TO_SS_YELLOW_STATE --> SS_YELLOW_STATE --> TO_MS_GREEN_STATE

//  State Name                MAIN STREAM OF TRAFFIC        SECONDARY STREAM OF TRAFFIC
//  MS_GREEN_STATE            GREEN                         RED
//  MS_YELLOW_STATE           YELLOW                        RED
//  MS_RED_STATE              RED                           GREEN
//  SS_YELLOW_STATE           RED                           YELLOW

// The above are the main states, with the state transitions represented by TO_MS_GREEN_STATE, TO_MS_RED_STATE, etc

enum {MS_GREEN_STATE, TO_MS_YELLOW_STATE, MS_YELLOW_STATE, TO_MS_RED_STATE, MS_RED_STATE, TO_SS_YELLOW_STATE, SS_YELLOW_STATE, TO_MS_GREEN_STATE};   

// The Pedestrian Cross Walks by default do not change - i.e. they show DONT WALK all the time, until a Walk sign is "requested" by the pedestrian
// by pressing the Walk Request Button for a given stream (to cross the Main Stream of Traffic or to cross the Secondary Stream)
// A Walk Request is triggered when pressing the walk request sign when the stream to cross is green or yellow. 

// When the main stream goes RED, the pedestrian sign transitions to WALK. When the secondary stream goes to YELLOW (signalling the end of the 
// secondary stream and for the main stream to turn GREEN, the main stream pedestrian sign transitions from WALK to FLASHING DONT WALK to give time  
// for the pedestrian to clean the walk. When the main stream turns GREEN, the main stream pedestrian sign transitions to DONT WALK. A similar transition
// happens for the secondary stream pedestrian sign, which has its own Walk Request Button. 

uint8_t STATE;                   //The current state machine state

enum TrafficLight {
  R1 = 0x01,
  R2 = 0x02,
  Y1 = 0x04,
  Y2 = 0x08,
  G1 = 0x10,
  G2 = 0x20,
  W1 = 0x40,
  W2 = 0x80,
  DW1 = 0x100,
  DW2 = 0x200
};


// Corresponding to above state transition enum, in millisecs
// the TO_STATE timings do not matter for now
// Main Stream Green is 10 seconds plus 3 seconds of Yellow, 
// while Main Stream Red is 8 seconds (main stream red + secondary stream yellow)
int stepTiming[] = {5000, 1, 2500, 1, 5000, 1, 2500, 1}; 
    
unsigned long ms;                //The current time from millis()
unsigned long msLast;            //The time the current state started

#define BLINK_INTERVAL 100 //In the BLINK state, switch the LED every 100 milliseconds.
boolean ledState;          //The current LED status
unsigned long msSwitch = 0;          //The last time the LED was switched

boolean walkReq1 = false;
boolean walkReq2 = false;

boolean inWalk1 = false;
boolean inWalk2 = false;

void setTrafficLights(int bits, int bbits = 0);
void fastBlink(int led);

void setup()
{
  Serial.begin(9600);

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }
  Serial.println(F("ProtoStax Traffic Light Demo")); 
  
  //Set pin configurations
  pinMode(RED1, OUTPUT);
  pinMode(YELLOW1, OUTPUT);
  pinMode(GREEN1, OUTPUT);
  pinMode(DONTWALK1, OUTPUT);
  pinMode(WALK1, OUTPUT);

  pinMode(RED2, OUTPUT);
  pinMode(YELLOW2, OUTPUT);
  pinMode(GREEN2, OUTPUT);
  pinMode(DONTWALK2, OUTPUT);
  pinMode(WALK2, OUTPUT); 

  setTrafficLights(0);
  
  STATE = TO_MS_RED_STATE;
  Serial.println(F("Finished Setup!")); 
  
}

void loop() 
{
  ms = millis();               //record the current time
  walkReqBtn1.read();
  walkReqBtn2.read();  
  int lights;
  int blinklights;

  if (walkReqBtn1.wasReleased()) { 
    Serial.println(F("Walk Request 1 received"));
    walkReq1 = true;
  }

  if (walkReqBtn2.wasReleased()) { 
    Serial.println(F("Walk Request 2 received"));          
    walkReq2 = true;
  }    

  switch (STATE) {
      case MS_GREEN_STATE:
          lights = 0 | G1 | R2 | DW2;
          if (inWalk1) {
            lights |= W1;
          } else {
            lights |= DW1;
          }
          setTrafficLights(lights);
          
          if (ms - msLast > stepTiming[STATE]) {
              // If we have been long enough in Green, transition 
              // to next state
              STATE = TO_MS_YELLOW_STATE;
          }
          break;

      case TO_MS_YELLOW_STATE: 
          msLast = ms;
          Serial.println(F("To YELLOW STATE")); 
          STATE = MS_YELLOW_STATE;
          break;

      case MS_YELLOW_STATE: 
          lights = 0 | Y1 | R2 | DW2;
          blinklights = 0;
          if (inWalk1) {
            blinklights |= DW1;                   
          } else {
            lights |= DW1;              
          }          
          setTrafficLights(lights, blinklights);       
                    
          if (ms - msLast > stepTiming[STATE]) {
              // If we have been long enough in Yellow, transition 
              // to next state            
              STATE = TO_MS_RED_STATE;
          }      
          break;
          
      case TO_MS_RED_STATE:
          msLast = ms;
          inWalk1 = false;
          if (walkReq2) {
            inWalk2 = true;
          }
          // Cancel any walk request on the secondary stream
          walkReq2 = false;
          Serial.println(F("To MS RED STATE"));           
          STATE = MS_RED_STATE;
          break;
          
      case MS_RED_STATE:
          lights = 0 | R1 | G2 | DW1;
          if (inWalk2) {
            lights |= W2;
          } else {
            lights |= DW2;
          }
          setTrafficLights(lights);            
          
          if (ms - msLast > stepTiming[STATE]) {
              // If we have been long enough in Red, transition 
              // to next state (secondary yellow)           
              STATE = TO_SS_YELLOW_STATE;
          }          
          break;

      case TO_SS_YELLOW_STATE:
          msLast = ms;
          Serial.println(F("To SS YELLOW STATE"));
          STATE = SS_YELLOW_STATE;      
          break;

      case SS_YELLOW_STATE:
          lights = 0 | Y2 | R1 | DW1;
          blinklights = 0;
          if (inWalk2) {
            blinklights |= DW2;                   
          } else {
            lights |= DW2;              
          }          
          setTrafficLights(lights, blinklights);              
                 
          if (ms - msLast > stepTiming[STATE]) {
              STATE = TO_MS_GREEN_STATE;
          }      
          break;
         
      case TO_MS_GREEN_STATE:
         msLast = ms;
         inWalk2 = false;
         if (walkReq1) {
            inWalk1 = true;
         }
         // Cancel any walk requests
         walkReq1 = false;
         Serial.println(F("To MS GREEN STATE")); 
         STATE = MS_GREEN_STATE;
         break;
  }              
   
}

// Supports Fast blinking either DONTWALK1 or DONTWALK2
// Change as appropriate
void setTrafficLights(int bits, int bbits = 0) 
{ 
  digitalWrite(RED1, (bits & R1)?HIGH:LOW);
  digitalWrite(RED2, (bits & R2)?HIGH:LOW);
  digitalWrite(YELLOW1, (bits & Y1)?HIGH:LOW);
  digitalWrite(YELLOW2, (bits & Y2)?HIGH:LOW);
  digitalWrite(GREEN1, (bits & G1)?HIGH:LOW);
  digitalWrite(GREEN2, (bits & G2)?HIGH:LOW); 
  digitalWrite(WALK1, (bits & W1)?HIGH:LOW);
  digitalWrite(WALK2, (bits & W2)?HIGH:LOW);
  if (bbits & DW1) {
    fastBlink(DONTWALK1);
  } else {
    digitalWrite(DONTWALK1, (bits & DW1)?HIGH:LOW);
  }
  
  if (bbits & DW2) {
    fastBlink(DONTWALK2);
  } else {
    digitalWrite(DONTWALK2, (bits & DW2)?HIGH:LOW);
  } 
}


//Reverse the current LED state. If it's on, turn it off. If it's off, turn it on.
//Works when there is only one blinking LED at a time, since ledState and msSwitch are global
// If you want it to work for multiple LEDs, create an array to correpond to the LED enums
// and use that to figure out the appropriate element of ledState/msSwitch from the array
void switchLED(int led)
{
    msSwitch = ms;                 //record the last switch time
    ledState = !ledState;
    digitalWrite(led, ledState);
}

//Switch the LED on and off every BLINK_INETERVAL milliseconds.
void fastBlink(int led)
{
    if (ms - msSwitch >= BLINK_INTERVAL)
        switchLED(led);
}
