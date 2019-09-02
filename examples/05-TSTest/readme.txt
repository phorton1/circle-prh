README.TXT for examples/05-TSTest

This project does not make use of the "std_kernel.h" or any of my UI or AUDIO
subsystems.  It has it's own kernel.cpp ala a typical circle example program.

It connects a cheap chinese 3.5" 320x480 resistive touch screen with an
ili9486 based display and an xpt2046 based touch to the rPI.

All it does is initialize the display (which is hopefully set to display
a distinctive pattern) and report any touches to the serial port.

It was used to debug this device for subsequent use in 06-UITest

