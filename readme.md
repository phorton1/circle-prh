My Additions to Circle
=======================

Clone this repository into the Circle repository from
https://github.com/phorton1/circle.  It should occupy
a directory called "_prh" within the Circle project,
that is it's own GIT repository.

There is a separate, additional "makeall.bat" in this
project to build all the libraries and test programs.

Significant additions (libraries) include:

/audio - a port of (part of) the Teensy Audio Library to the rPI
/bootloader - a bootloader (and perl script) for development use
/bt - a complete rewrite and actual implementation of a BT stack in Circle
/devices - support for 3.5" ili9846 320x480 screen with xpt2046 touch device
/lcd - a partial port of the LCDWIKI library to support various 8 bit displays
/system - a standardized kernel that is growing into an actual operating system
/ws - a wxWidgets like event driven windowing system
/utils - a few minor utilities and additions, including a miniUartDevice

[this file under contruction]
