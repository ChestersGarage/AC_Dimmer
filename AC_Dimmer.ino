/*
  AC Light Control
  
  Ryan McLaughlin <ryanjmclaughlin@gmail.com>
  
  The hardware consists of an Triac to act as an A/C switch and
  an opto-isolator to give us a zero-crossing reference.
  The software uses two interrupts to control dimming of the light.
  The first is a hardware interrupt to detect the zero-cross of
  the AC sine wave, the second is software based and always running
  at 1/128 of the AC wave speed. After the zero-cross is detected
  the function check to make sure the proper dimming level has been
  reached and the light is turned on mid-wave, only providing
  partial current and therefore dimming our AC load.
  
  Thanks to http://www.andrewkilpatrick.org/blog/?page_id=445
    and http://www.hoelscher-hi.de/hendrik/english/dimmer.htm
 */
 
/*
  Modified by Mark Chester <mark@chesterfamily.org>
  
  to use the AC line frequency (half-period) as a reference point 
  and fire the triacs based on that plus a dimmer delay value.
  I removed the second timer-based interrupt and replaced it with a
  means to reference the zero-crossing point as per interrupt 0.
*/

#include <TimerOne.h>                                  // http://www.arduino.cc/playground/Code/Timer1

// General
int lineFrequency = 60;                                // The frequency of the mains power in Hz (Usually either 60 Hz or 50 Hz)
float linePeriod = (1/float(lineFrequency))*1000;      // The period (or wavelength) in microseconds.
unsigned long int ZeroXTime1 = 0;                      // Timestamp in micros() of the latest zero crossing interrupt
unsigned long int ZeroXTime2 = 0;                      // Timestamp in micros() of the previous zero crossing interrupt
unsigned long int PeriodSample[3] = {0,0,0};           // We'll take three samples of the period to determine the average
unsigned int AvgPeriod;                                // The average line voltage period in micros()
unsigned long int NextTriacFire[4];                    // Timestamp in micros() when it's OK to fire the triacs again.
unsigned long int DimStep;                             // How many micros() in each step of dimming
int Dimmer[4];                                         // The dimmer input variable. One for each channel
byte TriacPin[4] = {4,5,6,7};                          // Which digital IO pins to use
boolean OKToFire[4];                                   // Bit to say it's OK for the triacs to fire
volatile boolean zero_cross = 0;                       // Boolean to store a "switch" to tell us if we have crossed zero

void setup() {                                         // Begin setup
  attachInterrupt(0, zero_cross_detect, FALLING);      // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
  pinMode(TriacPin[0], OUTPUT);                        // Set the Triac pin as output
  pinMode(TriacPin[1], OUTPUT);                        // Set the Triac pin as output
  pinMode(TriacPin[2], OUTPUT);                        // Set the Triac pin as output
  pinMode(TriacPin[3], OUTPUT);                        // Set the Triac pin as output
  // Run a modified version of the avg_period counter to get the startup period
  byte F = 0;                                          // Frequency counter counter  ;)
  while ( F < 3 ) {                                    // Loop three times
    if ( zero_cross ) {                                // Only run if a zero cross is detected
      PeriodSample[F] = micros();                      // set the new current zero cross time in micros()
      zero_cross = 0;                                  // Reset zero_cross.  This is what's diff from the function
      F++;                                             // Increment the counter counter
    }
  }
  AvgPeriod = (PeriodSample[0] + PeriodSample[1] + PeriodSample[2]) / 3;
}                                                      // End setup
  
void zero_cross_detect() {                             // function to be fired at the zero crossing
  zero_cross = 1;                                      // All we do is set a variable that's picked up later in the code
}

void avg_period() {                                    // Called every so often to calibrate the timer
  byte F = 0;                                          // Frequency counter counter
  while ( F < 3 ) {                                    // Loop three times
    if ( zero_cross ) {                                // Only run if a zero cross is detected
      PeriodSample[F] = micros();                      // set the new current zero cross time in micros()
      F++;
    }
  }
  AvgPeriod = (PeriodSample[0] + PeriodSample[1] + PeriodSample[2]) / 3;
}
void loop() {                                              // Main Loop
  if ( zero_cross ) {                                      // Did we detect a zero cross?
    ZeroXTime2 = ZeroXTime1;                               // shift the current zero cross value to the previous
    ZeroXTime1 = micros();                                 // set the new current zero cross time in micros()
    DimStep = (ZeroXTime1 - ZeroXTime2)/1024;              // Calc the duration of each dimming step
    Dimmer[0] = analogRead(0);                             // Read in a dimmer value (change to suit needs)
    Dimmer[1] = analogRead(1);
    Dimmer[2] = analogRead(2);
    Dimmer[3] = analogRead(3);
    NextTriacFire[0] = ZeroXTime1 + (Dimmer[0] * DimStep); // Calc the next triac fire time
    NextTriacFire[1] = ZeroXTime1 + (Dimmer[1] * DimStep);
    NextTriacFire[2] = ZeroXTime1 + (Dimmer[2] * DimStep);
    NextTriacFire[3] = ZeroXTime1 + (Dimmer[3] * DimStep);
    OKToFire[0] = 1;                                       // Tell us it's OK to fire the triacs
    OKToFire[1] = 1;
    OKToFire[2] = 1;
    OKToFire[3] = 1;
    zero_cross = 0;                                        // Done.  Don't try again until we cross zero again
  }
  if ( OKToFire[0] && micros() >= NextTriacFire[0] ) { // Are we OK and past the delay time?
    digitalWrite(TriacPin[0], HIGH);                   // Fire the Triac mid-phase
    OKToFire[0] = 0;                                   // We fired - no longer OK to fire
    digitalWrite(TriacPin[0], LOW);                    // Turn off the Triac gate (Triac will not turn off until next zero cross)
  }
  if ( OKToFire[1] && micros() >= NextTriacFire[1] ) { // Are we OK and past the delay time?
    digitalWrite(TriacPin[1], HIGH);                   // Fire the Triac mid-phase
    OKToFire[1] = 0;                                   // We fired - no longer OK to fire
    digitalWrite(TriacPin[1], LOW);                    // Turn off the Triac gate (Triac will not turn off until next zero cross)
  }
  if ( OKToFire[2] && micros() >= NextTriacFire[2] ) { // Are we OK and past the delay time?
    digitalWrite(TriacPin[2], HIGH);                   // Fire the Triac mid-phase
    OKToFire[2] = 0;                                   // We fired - no longer OK to fire
    digitalWrite(TriacPin[2], LOW);                    // Turn off the Triac gate (Triac will not turn off until next zero cross)
  }
  if ( OKToFire[3] && micros() >= NextTriacFire[3] ) { // Are we OK and past the delay time?
    digitalWrite(TriacPin[3], HIGH);                   // Fire the Triac mid-phase
    OKToFire[3] = 0;                                   // We fired - no longer OK to fire
    digitalWrite(TriacPin[3], LOW);                    // Turn off the Triac gate (Triac will not turn off until next zero cross)
  }
}
