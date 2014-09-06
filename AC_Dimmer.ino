/*
  AC Light Control
  Mark Chester <mark@chestersgarage.com>
  
  Dims household lights and other resistive loads.
  
  *******  CAUTION: Do not use on flourescent lights or inductive loads!  ********
  *******  Verify your device can handle dimming before using.  ********
  
  Fire triacs a certain time after detecting the zero-crossing time
  of the mains AC line voltage.
  
*/

// Test mode sets up a PWM signal at 127 on the test pin.
// Run a jumper from this pin to the zero-cross interrupt pin.
// Then watch the magic on your oscilloscope.
// Comment out this line to disable test mode.
#define TEST_MODE

// Set these for your installation
const int lineFrequency = 60;             // The frequency of the mains line voltage in Hz (Usually either 60 Hz or 50 Hz, and very rarely ~400 Hz)
const byte dimmerKnobPin[4] = {0,1,2,3};  // Which Analog pins are used for the dimmer knob inputs
const byte triacPin[4] = {4,5,6,7};       // Which digital IO pins are used for the triac gate
const byte zeroCrossInt = 0;              // Which interrupt is used for the zero-cross input (int 0 = pin D2, int 1 = D3)
#ifdef TEST_MODE
const byte testPin = 3;
#endif

// Mains line voltage zero-cross
volatile boolean zeroCross = false;                      // Have we detected a zero-cross?
unsigned long int zeroCrossTime = 0;                     // Timestamp in micros() of the latest zero-crossing interrupt
float linePeriod = (1.0/float(lineFrequency))*1000000.0; // The period (or wavelength) in microseconds.

// Dimmer knob inputs
int dimmerKnob[4];                       // The dimmer input value array
unsigned long int dimmerKnobReadTime;    // When it's time to read the dimmer knob inputs in millis()
const int dimmerKnobReadInterval = 100;  // How often to read the dimmer knob inputs in millis()

// Triac gate outputs
unsigned long int triacNextFireTime[4];  // Timestamp in micros() when it's OK to fire the triacs again.

// Initialize things
void setup() {
  attachInterrupt(zeroCrossInt, zeroCrossDetect, FALLING);  // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  pinMode(triacPin[0], OUTPUT);                             // Set the Triac pin as output
  pinMode(triacPin[1], OUTPUT);
  pinMode(triacPin[2], OUTPUT);
  pinMode(triacPin[3], OUTPUT);
#ifdef TEST_MODE
  analogWrite(3,127);
#endif
}

// ISR to run when the zero-cross interrupt trips
void zeroCrossDetect() {  // function to be fired at the zero-crossing
  zeroCross = true;       // All we do is set a flag that's picked up later in the code
}

// Main Loop
void loop() {
  // Read dimmer knobs
  if  ( millis() >= dimmerKnobReadTime+dimmerKnobReadInterval ) {  // If it's time to read the dimmer knobs
    dimmerKnobReadTime = millis()+dimmerKnobReadInterval;          // Set the next dimmer knob read time
    dimmerKnob[0] = analogRead(dimmerKnobPin[0]);                  // Read the dimmer knobs
    dimmerKnob[1] = analogRead(dimmerKnobPin[1]);
    dimmerKnob[2] = analogRead(dimmerKnobPin[2]);
    dimmerKnob[3] = analogRead(dimmerKnobPin[3]);
  }
  
  // Act on a zero-cross detection
  if ( zeroCross ) {                                                                    // Have we detected a zero cross?
    zeroCrossTime = micros();                                                           // Set the zero cross time in micros()
    zeroCross = false;                                                                  // Reset the flag
    triacNextFireTime[0] = zeroCrossTime + map(dimmerKnob[0],0,1023,0,int(linePeriod)); // Calc the next triac fire time
    triacNextFireTime[1] = zeroCrossTime + map(dimmerKnob[1],0,1023,0,int(linePeriod));
    triacNextFireTime[2] = zeroCrossTime + map(dimmerKnob[2],0,1023,0,int(linePeriod));
    triacNextFireTime[3] = zeroCrossTime + map(dimmerKnob[3],0,1023,0,int(linePeriod));
  }

  // Fire each of the triacs at the right time
  if ( micros() >= triacNextFireTime[0] ) {                  // Is it time to fire the triacs?
    digitalWrite(triacPin[0], HIGH);                         // Fire the Triac to turn on flow of electricity
    triacNextFireTime[0] = triacNextFireTime[0]+linePeriod;  // Push the next fire time out into the distant future
  }
  if ( micros() >= triacNextFireTime[1] ) {
    digitalWrite(triacPin[1], HIGH);
    triacNextFireTime[1] = triacNextFireTime[1]+linePeriod;
  }
  if ( micros() >= triacNextFireTime[2] ) {
    digitalWrite(triacPin[2], HIGH);
    triacNextFireTime[2] = triacNextFireTime[2]+linePeriod;
  }
  if ( micros() >= triacNextFireTime[3] ) {
    digitalWrite(triacPin[3], HIGH);
    triacNextFireTime[3] = triacNextFireTime[3]+linePeriod;
  }
  
  // Let go of the triac gates
  digitalWrite(triacPin[0], LOW);  // Turn off the Triac gate (Triac will not actually turn off until ~next zero-cross)
  digitalWrite(triacPin[1], LOW);
  digitalWrite(triacPin[2], LOW);
  digitalWrite(triacPin[3], LOW);
}
