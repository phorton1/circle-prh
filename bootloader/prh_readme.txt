prh

This is the latest in a string of bootloaders.

The goal is to create a rPi USB Serial bootloader,
semi compatible with the Arduino IDE and avrdude.

It is compiled within the circle framework to allow it
access to circle's standard devices, including especially,
initially, the Serial port and a timer for counting down,
but possibly to include other more complicated devices like
the screen, ethernet port, and even bt and/or wifi.

The most complicated goal will be to nicely implement a
usbSerialDevice "device" (peripheral, gadget) within circle.
All of circle's current usb classes are host related, as are
almost all rPi linux device drivers. There are no bare metal
usb device examples, and the only documentation or code of any
kind I could find was to reverse engineer the rpi-linux "usb
gadget" driver.  This was the ONLY code or doc I could find
on the rPi chip's USB module being programmed as a device.

So we'll see.

The first thing to do is to get a basic serial HEX receiver
up within the Circle environment with a timeout chain load
of kernel(n).img.   This will then be used as a bootloader while
I develope the usb portions.

The other generally messy part of the process is to "flash" the
rpi, that is to ovewrite the sd card version of kernel(n).img so
that it will be started from the next file boot.

--------------------------------------------------------

So far, the iterations of rPi bare metal bootloaders has been

none  	no config.txt, just manually copy the compiled kernel.img to the sdcard.

SREC  	the initial dave welch bootloader10 that I started working with.
		I don't think it had a timer.  It just waited for the srec's.
		i'm not sure how far I got integrating this into the devleopment
		environment, but I'm pretty sure I got it into an earlier version
		of the console program.
		
PLACID 	that was weird and hard, and there may be some good code in placid
XMODEM	worth moving forward, but I think I can do without the xmodem stuff.
		has a timer, though.
		
CIRCLE	amazed and amused by running circle with an ethernet cable from an	
ETHNET	rPi b3+ on a fixed ip address to the laptop ("sharing" the wifi
FTP     connection so that there's dhcp someplace), logging to the hdmi
        screen, there's an HTTP server as well as the TFTP server.
		
		I added Perl Net:TFTP in the console to easily transfer files
		from the laptop to the rPi and integrated it into Komodo to more
		rapidly build and upload a given program (as well as komodo stuff
		to make phyical sdcard updates of the bootloader itself easier)
		
		Had a timer?
		
		Drawbacks:
				
				There's no ethernet port on the rPi zero.
				
				I don't have a mini-hdmi cable for the rPi zero,
				so cant hook it to a big screen, andnot sure I want to.
				
				Maybe a touch screen for application UI ...
				
				But otherwise, I would prefer that all programming and
				monitoring was "doable" through the USB serial port, ala
				the arduino experience.
		
				
THIS ONE

		so here we are, making a hex loader within the circle framework
		probably already existed, but I don't want to use the vectors.s
		or periph.c from dwelch.
		
		
		





