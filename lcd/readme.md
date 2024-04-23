Circle (partial) port of LCDWIKI library
=========================================

This directory contains a partial port of the LCDWIKI library found at
http://www.lcdwiki.com/Main_Page.

It implements a simplified fake Arduino API in **pins_arduino.h** and **cpp**.

It has only been tested with one **ILI9486** based eight bit 320x488 *Arduino* device.

I had some Arduino touchscreens that I wanted to try hooking up to the rPi.
I got the *display* working just fine.
But since the rPi has no analog inputs,
I was unable to implement the *touch screen* functionality, which, on the Arduino,
uses the **analog inputs** to read the resistive touch screen.

I messed around for a while trying to separate out the touchscreen and
have an Arduino Nano just do that part, but I could not get it working.

I made the whole thing into a i2c device,
and may post that separately as an Arduino project, but it was too
slow to use as a pixel oriented display device in Circle, so in the end
I abandoned the effort.

However, the partial port might be useful to someone for
some reason so I am posting it.
