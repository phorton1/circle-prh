Bootloader
==========

This is the latest in a string of bootloaders.

It is compiled within the Circle framework to allow it access to circle's standard devices, including the Serial port and a timer for counting down, as well as, optionally, other more complicated devices like the screen, ethernet port, and even bt and/or wifi.

Please see the source code in **kernel.h** and **kernel.cpp** for various compile flags you can change to change it's behavior.

History
-------

So far, the iterations of rPi bare metal bootloaders has been

* **none** - no config.txt, just manually copy the compiled kernel.img to the sdcard.

* **SREC** - the initial dave welch bootloader10 that I started working with. I don't think it had a timer.  It just waited for the srec's. i'm not sure how far I got integrating this into the devleopment environment, but I'm pretty sure I got it into an earlier version of the console program.
		
* **PLACID XMODEM** - that was weird and hard, and there may be some good code in Placid worth moving forward, but I think I can do without the xmodem stuff. Has a timer, though.
		
* **CIRCLE** - amazed and amused by running circle with an ethernet cable from an rPi b3+ on a fixed ip address to the laptop ("sharing" the wifi connection so that there's dhcp someplace), logging to the hdmi screen, there's an HTTP server as well as the TFTP server.
		
Although I used the Circle bootloader with TFTP as is (with only minor mods) for quite a while, since the rPi zero does not have an ethernet port, at some point I decided to go back through to dave welch's stuff (SREC) and ended up adding my own **binary protocol** which is documented in the source code.

console.pm
----------

Although any serial console program (i.e. Putty) is compatible with this rPi bootloader (i.e. with XMODEM) at some point I decided also to write my own serial monitor program.   I am including a (denormalized) copy of this program in the source directory.  It normally lives, on my machine, in a folder **base/bat** which is in my **path** variable

The **Perl** program **console.pm** implements host support for the various protocols (SREC, XMODEM, and TFTP) as well as providing the reference implementation of the host side of my binary protocol.  So, if nothing more, it can serve as an example of how you might encode similar functionality ih any other language.

If you want to use the actual Perl script itself, there are some variables in the program that you *may* (would) need to change to get it to work on your machine.  Specifically, the script is called from a windows batch file to invoke it with Perl and pass arguments to it:

    rem console.bat
    perl /base/bat/console.pm  %1  %2  %3  %4

The perl code also references a file **$registry_filename** = "/base/bat/console_autobuild_kernel.txt" which contains the name of a kernelN.img file that gets auto-uploaded to the rPi.  This level of indirection allows the console program to detect changes (rebuilds) of the kernel (the program you are uploading) as well as allowing you to change the program by merely rewriting the registry file.

Once again, please see the source for **console.pm** for details on the binary protocol and usage scheme.


TeensyPi.ino
------------

**Not** included (yet) at this time is a program that runs on a Teensy 3.2 which provides a far superior experience to just hooking a FTL232 serial-to-usb convertor up to the rPi. The main thing it provides is the ability to reboot the rPi from the serial port by sending ctrl-B to the teensyPi program (all other characters are passed thru in both directions).

I present TeensyPi, and the associated circuit that I use separately, as a Hackaday project, at

    http:://blah.blah
    
    
Previous Notes on the Circle Bootloader
---------------------------------------

These notes are included verbatim from the original Circle bootloader, still available, unchanged, in /circle/tools/bootloader.  They generally still apply to my version, but all I am using, and regularly
testing, at the current time is my own bihnary protocol.


README

This sample program is a boot-loader with in-memory update of the kernel image
(chain boot). When the sample is running, you can send an other kernel*.img
file via the local network to your Raspberry Pi and automatically start it. The
kernel image is not written out to the SD card. The boot-loader has two user
interfaces, a HTTP-based web front-end and a TFTP file server daemon.

The boot-loader does not implement any authorization method (e.g. a password).
Be sure to be the only user on your local network, who has access to it!


USING HTTP

Open the following URL in a web browser on your host computer, enter a kernel
image file to be uploaded (kernel*.img) and press the "Boot now!" button.
ip_address is the IP address of your Raspberry Pi and is displayed on its
screen.

	http://ip_address:8080/


USING TFTP

You need a TFTP client installed on your host computer to use this. Enter the
following command to send the kernel image:

	tftp -m binary ip_address -c put kernel.img

ip_address is the IP address of your Raspberry Pi and is displayed on its
screen. The file name of the kernel image may be slightly different, depending
on your Circle project settings. Some TFTP clients do not allow to specify all
options on the command line. You may have to specify the "binary" and "put"
commands manually behind the tftp> prompt.

Please note that there may be a "Transfer timed out" message on your Raspberry
Pi, before the transfer runs and completes successfully. This is normal and can
be ignored.


SOME NOTES

If you want to include the boot-loader support into your own application, please
note that:

* The chain boot mechanism is not supported in multi-core applications.

* The accelerated graphics and HDMI sound support cannot be restored after
  chain boot, if used before. There may be problems with the Act LED too.
  That's why it is not used in this bootloader.

* Some devices are not reset in the destructors of their driver classes.
  This may cause problems with some devices. Please report such issues.
  The devices used in this sample should work in any way.

* The source file main.cpp of this sample is slightly different from that,
  which is used in other sample programs.

