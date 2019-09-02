README.TXT for examples/00-lcdDisplay

This project does not make use of the "std_kernel.h" or any of my UI or AUDIO
subsystems.  It has it's own kernel.cpp ala a typical circle example program.

It was an attempt to hook an cheap arduino resistive touch screen up to a rPi
using a crude, rude port of the LCDWIKI libraries.

I got as far as making the display work correctly, but could not get a reasonable
implementation of the touch screen working with the rPI.  That is because the
rPi has no analog inputs, and using touchscreens generally requires an analog
input.

I tried interjecting an Arduino into the mix, that would just sit there and
monitor the resistive touch screen, but that does not work because the same
wires are used for the display I2C as for the resistive touch screen (analog),
and the rPI has to be aware of that, and change to high impedance inputs (NC)
for the Arduino to be able to read the values from the resistors.  That, in
turn required a complicated interaction beween the rPi and the Arduino which
ended up just not being worth the effort.

So, this is basically a legacy project, and NOT A GOOD EXAMPLE to start with.

I keep it because it was a lot of work.

If you are trying to use my code to get things like the Audio Library or UI
working on an rPI, I suggest that you skip this example, and move onto one
of the other example projects.

