My Additions to Circle
=======================

Clone this repository into the Circle repository from
https://github.com/phorton1/circle.  It should occupy
a directory called "_prh" within the Circle project,
that is it's own GIT repository.

There is a separate, additional "makeall.bat" in this
project to build all the libraries and example programs.

The source code directory structure includes the following
sub directories.  Subdirectories with links below have their
own readme files for more information.

* /audio - a port of (part of) the Teensy Audio Library to the rPI
* /bootloader - a bootloader (and perl script) for development use
* /bt - a complete rewrite and actual implementation of a BT stack in Circle
* /devices - support for 3.5" ili9846 320x480 screen with xpt2046 touch device
* /lcd - a partial port of the LCDWIKI library to support various 8 bit displays
* /system - a standardized kernel that is growing into an actual operating system
* /ws - a wxWidgets like event driven windowing system
* /utils - a few minor utilities and additions, including a miniUartDevice


Please See
----------

Based on this repository, I am in the process of developing a number of projects.  I have made the first
of those public.

Please see my repository at [https://github.com/phorton1/circle-prh-apps-Looper](https://github.com/phorton1/circle-prh-apps-Looper)
for the implementation of a 3D printed, gig-ready, multi-track, floor based, **Audio Looper** which is based
on this circle fork, and my extensions.

You *might* also be interested in my Teensy based (Arduino development environent)
**[teensyExpression Pedal](https://github.com/phorton1/Arduino-teensyExpression)** which
connects to, and controls, that Looper (as well as my iPad based guitar rig).

All of this is part of my long term ongoing
**[rPi bare metal vGuitar rig](https://hackaday.io/project/165696-rpi-bare-metal-vguitar-rig)**
project at hackaday.

Credits
--------

To **rst** for his amazing long term work on Circle, without which this would not
have been possible.

To **Paul Stoffregen** for the audio library, and his wonderful **teensy** series of MPU boards.
Please visit his website at [prjc.com](https://www.prjc.com) and support him by purchasing teensy
boards and teensy related parts and accessories.    He also has github, hackaday, and facebook pages
that you can find easily on the internet.

Also thanks to the many folks who have contributed to the [lcdwiki](http://www.lcdwiki.com) project.
