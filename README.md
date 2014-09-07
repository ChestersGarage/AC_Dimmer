AC_Dimmer
=========

Tested with internal test mode. It's pretty good, but seems a little jumpy on my cheapo digital o-scope.

Mark Chester <mark@chestersgarage.com>
  
Dims household lights and other resistive loads.
Triggers triacs a certain time after detecting the zero-crossing time of the mains AC line voltage.
  
This sketch requires a separate circuit board that consists of triacs, opto-isolators and a zero-crossing detector at the very least.
  
DO NOT use on flourescent lights or inductive loads.
Verify your device can handle dimming before using.
  
BE CAREFUL!  Mains voltage CAN KILL YOU if anything goes wrong.

Care and safe physical design are paramount to a successful installation.

To-Do:
* Run it against real hardware. I don't have any triacs or zero-crossing detectors. So I will have to build something and test before I can call this code done.
