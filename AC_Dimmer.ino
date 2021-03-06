/*
  AC Light Control
  Mark Chester <mark@chestersgarage.com>
  
  Dims household lights and other resistive loads.
  Triggers triacs a certain time after detecting the zero-crossing time
  of the mains AC line voltage.
  
  This sketch requires a separate circuit board that consists of
  triacs, opto-isolators and a zero-crossing detector at the very least.
  
  DO NOT use on flourescent lights or inductive loads.
  Verify your device can handle dimming before using.
  
  BE CAREFUL!  Mains voltage CAN KILL YOU if anything goes wrong.
  Care and safe physical design are paramount to a successful installation.
*/

// Test mode (look for other lines with this designation and remove for normal use)
// To use test mode, run a wire from testPin to Int 0 (pin D2)
const int testPin = 11;
// End test mode

// Zero-cross detector
const byte zeroCrossInt = 0;         // Interrupt used for the zero-cross input (int 0 = pin D2, int 1 = D3)
const int lineFrequency = 122;       // The frequency of the mains line voltage in Hz
// Usually either 60 Hz or 50 Hz, and very rarely ~400 Hz; test mode is 488Hz
volatile boolean zeroCross = false;  // Have we detected a zero-cross?
unsigned long zeroCrossTime = 0;     // Timestamp in micros() of the latest zero-crossing interrupt
const int delayPeriod = 300;         // On the ATMega 328p/16MHz, the soonest we can fire the triacs is about 250-300us after zero-cross
// Run with delayPeriod at 0 to see how well your sketch responds, then adjust for better usability.
float linePeriod = (1.0/float(lineFrequency))*1000000.0-delayPeriod; // The line voltage period in microseconds (adjusted for delay).

// Dimmer knobs
const byte dimmerKnobPin[4] = {0,1,2,3};     // Analog pins used for the dimmer knob inputs
const long dimmerKnobReadInterval = 100000;  // How often to read the dimmer knob inputs in micros()
int dimmerKnob[4];                           // The dimmer input value array
unsigned long dimmerKnobReadTime = 0;        // When it's time to read the dimmer knob inputs in micros()

// Triacs
const byte triacPin[4] = {4,7,8,12};  // Digital IO pins used for the triac gates
unsigned long triacNextFireTime[4];   // Timestamp in micros() when it's OK to fire the triacs again.
boolean triacFired[4] = {0,0,0,0};    // Triac has been fired since the last zero-cross
int triacPulseDuration = 20;          // The minimum duration of the triac pulse in micros()
// Review the data sheet for your triacs for the best triacPulseDuration value

// Initialize things
void setup() {
  // Test mode
  TCCR2B = TCCR2B & 0b11111000 | 0x06;  // Sets the PWM frequency on testPin to 122Hz
  // End test mode
  // Attach the zero-cross interrupt (verify which 'mode' parameter works best with the zero-cross detector you use)
  attachInterrupt(zeroCrossInt, zeroCrossDetect, FALLING);
  pinMode(triacPin[0], OUTPUT);  // Set the Triac pin as output
  pinMode(triacPin[1], OUTPUT);
  pinMode(triacPin[2], OUTPUT);
  pinMode(triacPin[3], OUTPUT);
  // Test mode
  pinMode(testPin, OUTPUT);
  analogWrite(testPin, 127);
  // End test mode
}

// Main Loop
void loop() {
  // Keep these three grouped together and put other code above or below them.
  // There is a certain need for good timing, so you can't use too much other code on a 328p/16MHz.
  // Use a faster board such as the Due if you need to do a lot of other stuff.
  readDimmerKnobs();
  checkZeroCross();
  fireTriacs();
}

// ISR to run when the zero-cross interrupt trips
void zeroCrossDetect() {  // function to be fired at the zero-crossing
  zeroCross = true;       // All we do is set a flag that's picked up later in the code
}

// Read dimmer knobs
void readDimmerKnobs(){
  if  ( micros() >= dimmerKnobReadTime+dimmerKnobReadInterval ) {  // If it's time to read the dimmer knobs
    dimmerKnobReadTime = micros()+dimmerKnobReadInterval;          // Set the next dimmer knob read time
    dimmerKnob[0] = analogRead(dimmerKnobPin[0]);                  // Read the dimmer knobs
    dimmerKnob[1] = analogRead(dimmerKnobPin[1]);
    dimmerKnob[2] = analogRead(dimmerKnobPin[2]);
    dimmerKnob[3] = analogRead(dimmerKnobPin[3]);
  }
}

// Act on a zero-cross detection
void checkZeroCross() {
  if ( zeroCross ) {           // Have we detected a zero-crossing?
    zeroCrossTime = micros();  // Set the zero cross time in micros()
    zeroCross = false;         // Reset the flag
    triacNextFireTime[0] = zeroCrossTime + map(dimmerKnob[0],0,1023,delayPeriod,long(linePeriod)); // Calculate the next triac fire time
    triacNextFireTime[1] = zeroCrossTime + map(dimmerKnob[1],0,1023,delayPeriod,long(linePeriod));
    triacNextFireTime[2] = zeroCrossTime + map(dimmerKnob[2],0,1023,delayPeriod,long(linePeriod));
    triacNextFireTime[3] = zeroCrossTime + map(dimmerKnob[3],0,1023,delayPeriod,long(linePeriod));
    triacFired[0] = false;  // Unset the flag so the triacs will fire
    triacFired[1] = false;
    triacFired[2] = false;
    triacFired[3] = false;
  }
}

// Fire each of the triacs at the right time.
// When two or more dimmer knobs are near the same value,
// they will contend for timing and may be jumpy.
void fireTriacs() {
  if ( triacFired[0] == false && micros() >= triacNextFireTime[0] ) {  // Is it time to fire the triacs?
    digitalWrite(triacPin[0], HIGH);                                   // Fire the Triac to turn on flow of electricity
    triacFired[0] = true;                                              // Set a flag so we don't multi-fire
  }
  if ( triacFired[1] == false && micros() >= triacNextFireTime[1] ) {
    digitalWrite(triacPin[1], HIGH);
    triacFired[1] = true;
  }
  if ( triacFired[2] == false && micros() >= triacNextFireTime[2] ) {
    digitalWrite(triacPin[2], HIGH);
    triacFired[2] = true;
  }
  if ( triacFired[3] == false && micros() >= triacNextFireTime[3] ) {
    digitalWrite(triacPin[3], HIGH);
    triacFired[3] = true;
  }
  // Let go of the triac gates
  if ( triacFired[0] == true && micros() >= triacNextFireTime[0]+triacPulseDuration ) {  // make sure it's OK
    digitalWrite(triacPin[0], LOW);  // Turn off the triac gate (Triac will not actually turn off until ~next zero-cross)
  }
  if ( triacFired[1] == true && micros() >= triacNextFireTime[1]+triacPulseDuration ) {
    digitalWrite(triacPin[1], LOW);
  }
  if ( triacFired[2] == true && micros() >= triacNextFireTime[2]+triacPulseDuration ) {
    digitalWrite(triacPin[2], LOW);
  }
  if ( triacFired[3] == true && micros() >= triacNextFireTime[3]+triacPulseDuration ) {
    digitalWrite(triacPin[3], LOW);
  }
}
