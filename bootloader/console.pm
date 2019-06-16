# Generic Windows Perl (ActivePerl) Script
#
# Includes no custom perl libraries, so should run
# against a generic Perl on Windows. I am using
# ActivePerl (v5.12.4) built for MSWin32-x86-multi-thread
#
# There is a denormalized copy of this in /src/circle
# _prh/bootLoader.
#
# command line
#
#   defaults to -teensy
# 
#   Any number by itself on the command line that is less than 100 is considered a COM port number
#   Any number by itself on the command line that is geq 100 is considered a baud rate
#
#   -teensy    default port = 21
#              default baud = 115200
#              implies -arduino
#              
#   -rpi       default port = 23
#              default baud = 921600
#              implies -upload,-auto,-arduino,-only_if_changed
#
#   -arduino   watches for arduino builds and disconnects the comm port
#              while the build is active, and reconnects 2 seconds after
#              it finishes
#
#   -upload   allows ctrl-X uploads to device using default (my Binary) protocol
#             which is implemented in my rpi circle bootloader.
#             other allowed values are:
#
#             -uploadSREC      - runs against dwelch bootloader10, my initial bare metal attempt
#             -uploadPlacid    - uses Xmodem to send to my second attempt, the placid bootloader
#             -uploadTFTP      - runs against circle/sample/38-bootloader, which has an TFTP server
#                                and *should* also work with my bootloader if WITH_TFTP is defined
#
#   -auto      watches for bootloader upload signature and
#              automatically uploads latest kernel if it sees the signature
#
#   -only_if_changed	only uploads a new program (kernel) on a pi reboot
#                       if that file has changed since our last upload.
#
# 			  Anytime it changes on the host machine, we still need to reboot, but we
# 			  set a flag saying that it is a new version.  This flag is cleared when
# 			  the console starts. We only auto-upload the file if the flag has changed.
#              
#--------------------------------------------------------------------------
# Generalized rPi auto-upload scheme for use with my Circle bootloader
#--------------------------------------------------------------------------
# The system reads, and watches for, changes to a file called
# /base/bat/console_autobuild_kernel.txt.
#
# This file contains a single line specifying a kernel.img to be uploaded
# to the rPi, ie:  /src/Arduino/_circle/audioDevice/kernel7.img
#
# This file is uploaded when -upload and ctrl-X is pressed, or
# with -auto, when the console_autobuild_kernel.txt changes,
# or the file it points to changes. When this happens the rPi will
# be rebooted and when the console sees the upload_signature_re,
# it will upload the given kernel.img to the bootloader.

#------------------------------------------
# KOMODO keystroke commands and macros:
#------------------------------------------
# Documented here for lack of a better place.
# This is how my development IDE (Komodo) uses this script.
#
# The scripts are found in
#
#    Users/Patrick/AppData/Local/ActiveState/KomodoEdit/8.5/tools
#
# and are manually normalized to
#
#     /bat/Komodo_Tools-Patrick-AppData-Local-ActiveState-KomodoEdit-8.5-tools
#
# checked into git under the "base" project.
#
#    ctrl-I = build the left most INO (arduino) project
#       calls compile_arduino.js komodo script
#
#       The project HAS to live in an outer level /src/Arduino
#       folder of the same name as the INO file.
#
#       Uses the /base/bat/arduino.cmd scheme where before
#       you can build it in komodo, you must double click on
#       the INO file in windows explorer, which is hooked up
#       to execute arduino.cmd and create a preferences.txt
#       in the folder.  You modify the configuration (the
#       board type, com port, teensy serial port type, etc)
#       in the arduino IDE (and usually quit it), and those
#       options are picked up when komodo ctrl-I calls
#       the arduino CLI (commandl line interface) with the
#       given preferences file.
#
#   ctrl-J = build the left most kernel.cpp
#       calls compile_rpi.js komodo script
#
#       executes a make in the directory containing the
#       left most kernel.cpp file.  If the first time per
#       komodo invocation, or if the kernel.cpp is different
#       than the previous build, komodo writes a new
#       /base/bat/console_autobuild_kernel.txt pointing
#       to the new kernel.img.  The rest happens here.
#
#   ctrl-K = build the circle/_prh/bootloader/recovery.img
#       calls compile_bootloader.js komodo script
#
#       at this time there is no automatic upload, or indeed,
#       no soft upload of recovery.img to the SD card.  For
#       build/test cycles of the bootloader, the makefile needs
#       to be modified to produce kernel.img instead of recovery.img
#       and ctrl-J used.  Usually, ctrl-K is used in conjunction with
#       ctrl-L build/test cycles with SD card swapping.
#
#   ctrl-L = update the rpi SD memory card
#       calls update_rpi_memory_card.js komodo script
#
#       which calls /bat/updatePiMemoryCard.pm which (a) waits
#       for upto 5 seconds for an SD card (on D:), then compares
#       the recovery.img on it to the one in /src/circle/_prh/bootloader,
#       and writes that to the SD card if it has changed.
#
#   ctrl-O = clean a bare metal project directory
#       calls clean_rpi.js komodo script
#
#       which calls "make clean" in the directory of
#       the left most kernel.cpp file. Usually used before
#       pressing ctrl-J

use strict;
use warnings;
use threads;
use threads::shared;
use Time::HiRes qw( sleep usleep  );
use Win32::Pipe;
use Win32::Console;
use Win32::Process::List;
use Win32::Process::Info qw{NT};
use Win32::SerialPort qw(:STAT);
use Net::TFTP;
# use My::Utils qw(display_bytes);


$| = 1;     # IMPORTANT - TURN PERL BUFFERING OFF MAGIC
    

my $kSREC   	= 'SREC';
my $kPLACID 	= 'Placid';
my $kTFTP		= 'TFTP';
my $kBINARY    	= 'Binary';

my $SYSTEM_CHECK_TIME = 3;
    # check for changed COM connections and new kernel.img
    # every this many seconds
my $USE_BOOTLOADER = $kBINARY;   # $kTFTP;	
    # specify which bootloader we are using
    # which in turn determines the file to look for
    # and the method of uploading ...
    
	
my $registry_filename = "/base/bat/console_autobuild_kernel.txt";
my $registry_filetime = getFileTime($registry_filename);
my $kernel_filename = '';
my $kernel_filetime = 0;
my $kernel_file_changed = 0;


# Default setup of ports and behavior

my $RPI_COM_PORT  = 25;
my $TEENSY_COM_PORT  = 24;
my $RPI_BAUD_RATE = 115200; # 921600;     
my $TEENSY_BAUD_RATE = 115200;    
    # 1843200, 921600, 460800, 230400, 115200, 38400


# Console color attributes
# low order nibble of $attr = foreground color
# high order nibble of $attr = background color
# by default the color is left as $COLOR_CONSOLE

my $color_black            = 0x00;	
my $color_blue             = 0x01;	
my $color_green            = 0x02;	
my $color_cyan             = 0x03;	
my $color_red              = 0x04;	
my $color_magenta          = 0x05;	
my $color_brown            = 0x06;	    
my $color_light_gray       = 0x07;	
my $color_gray             = 0x08;	
my $color_light_blue       = 0x09;	
my $color_light_green      = 0x0A;	
my $color_light_cyan       = 0x0B;	
my $color_light_red        = 0x0C;	
my $color_light_magenta    = 0x0D;	
my $color_yellow           = 0x0E;	
my $color_white            = 0x0F;



my $COLOR_CONSOLE = $color_light_gray;

sub colorAttr
{
	my ($ansi_color) = @_;
	return
		
		# circle puts out 0 and 1 as "panics"
		$ansi_color == 0  ?  $color_light_red 	 	:
		$ansi_color == 1  ?  $color_light_magenta 	 	:
		# ansi standards
		
		$ansi_color == 30  ?  $color_black 	     	:
		$ansi_color == 31  ?  $color_red 	     	:
		$ansi_color == 32  ?  $color_green 	     	:
		$ansi_color == 33  ?  $color_brown 	 		:
		$ansi_color == 34  ?  $color_blue 	     	:
		$ansi_color == 35  ?  $color_magenta 	 	:
		$ansi_color == 36  ?  $color_cyan 	     	:
		$ansi_color == 37  ?  $color_white 	     	:
		$ansi_color == 90  ?  $color_light_gray  	:
		$ansi_color == 91  ?  $color_light_red 	 	:
		$ansi_color == 92  ?  $color_light_green 	:
		$ansi_color == 93  ?  $color_yellow 		:
		$ansi_color == 94  ?  $color_light_blue  	:
		$ansi_color == 95  ?  $color_light_magenta 	:
		$ansi_color == 96  ?  $color_light_cyan 	:
		$ansi_color == 97  ?  $color_white  		:
		$color_light_gray;
}






#-----------------------
# command line params
#-----------------------

my $COM_PORT = $TEENSY_COM_PORT;
my $BAUD_RATE = $TEENSY_BAUD_RATE;

my $rpi = 0;
my $teensy = 0;
my $xlat_crlf = 0;
my $echo = 0;

my $watch_arduino = 0;
my $watch_kernel  = 0;
my $allow_uploads = 0;
my $auto_upload   = 0;
my $only_if_changed = 0;



#-----------------------
# working variables
#-----------------------

my $con = Win32::Console->new(STD_OUTPUT_HANDLE);
my $in = Win32::Console->new(STD_INPUT_HANDLE);
$in->Mode(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT );
$con->Attr($COLOR_CONSOLE);


my $port;
my @event;
my $system_check_time = 0;
my $in_arduino_build:shared = 0;


#---------------------------------------------------
# methods
#---------------------------------------------------

sub showStatus
{
    $con->Title(
		($rpi ? "rPI " : "").
		($teensy ? "teensy " : "").
		"COM$COM_PORT ".
        ($in_arduino_build ? "in arduino build" :
        $port ?
			"connected at $BAUD_RATE ".
				($auto_upload   ? " -auto" : "").
				($allow_uploads ? " -upload" : "").
				($watch_arduino ? " -arduino" : "").
				($watch_kernel  ? " -kernel" : "").
				(($auto_upload | $allow_uploads | $watch_kernel) ? " $kernel_filename" : "") :
			"disconnected"));
}


sub isEventCtrlC
    # my ($type,$key_down,$repeat_count,$key_code,$scan_code,$char,$key_state) = @event;
    # my ($$type,posx,$posy,$button,$key_state,$event_flags) = @event;
{
    my (@event) = @_;
    if ($event[0] &&
        $event[0] == 1 &&      # key event
        $event[5] == 3)        # char = 0x03
    {
        print "ctrl-C pressed ...\n";
        return 1;
    }
    return 0;
}    

    
sub getChar
{
    my (@event) = @_;
    if ($event[0] &&      
        $event[0] == 1 &&       # key event
        $event[1] == 1 &&       # key down
        $event[5])              # char
    {
        return chr($event[5]);
    }
    return undef;
}


sub getFileTime
{
    my ($filename) = @_;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	  	$atime,$mtime,$ctime,$blksize,$blocks) = stat($filename);
    # print "file_time=$mtime\n";
    return $mtime || 0;
}


sub initComPort
{
    # print "initComPort($name,$com_port,$baud_rate)\n";
    
    $port = Win32::SerialPort->new("COM$COM_PORT",1);
    
    if ($port)    
    {
        print "COM$COM_PORT opened\n";
        
        # This code modifes Win32::SerialPort to allow higher baudrates
        
        $port->{'_L_BAUD'}{230400} = 230400;
        $port->{'_L_BAUD'}{460800} = 460800;
        $port->{'_L_BAUD'}{921600} = 921600;
        $port->{'_L_BAUD'}{1843200} = 1843200;
        
        $port->baudrate($BAUD_RATE);  
        $port->databits(8);
        $port->parity("none");
        $port->stopbits(1);
        
        # $port->buffers(8192, 8192);
        $port->buffers(60000,8192);
        
        $port->read_interval(100);    # max time between read char (milliseconds)
        $port->read_char_time(5);     # avg time between read char
        $port->read_const_time(100);  # total = (avg * bytes) + const
        $port->write_char_time(5);
        $port->write_const_time(100);
        
        $port->handshake("none");
            # handshaking needed to be turned off for uploading binary files
            # or else sending 0's, for instance, would freeze
            
        # $port->binary(1);
        # $port->handshake("rts");   # "none", "rts", "xoff", "dtr".
        
        if (!$port->write_settings())
        {
            print "Could not configure COM$COM_PORT\n";
            $port = undef;
        }
        else
        {
            $port->binary(1);
            showStatus();
        }
    }
    return $port;
}



sub systemCheck
{
    # print "system check ...\n";
    # check for dropped ports
    
    if ($port)
    {
        my ($BlockingFlags, $InBytes, $OutBytes, $LatchErrorFlags) = $port->status();
        if (!defined($BlockingFlags))
        {
            # $save_port = $port;
            # save the port to prevent reporting of errors
            # when Win32::SerialPort tries to close it when
            # we set it to undef below.  So far, no negative
            # side effects from this ...
            
            print "COM$COM_PORT disconnected\n";
            $port = undef;
            showStatus();
        }
    }

    # check if the kernel registry file has changed and update
	# the kernel filename if it has ...
	
    if ($watch_kernel || $auto_upload || $allow_uploads)
    {
        my $check_time = getFileTime($registry_filename);
		if ($check_time != $registry_filetime)
		{
			$registry_filetime = $check_time;
			print "kernel registry file has changed\n";
			if ($check_time)
			{
				my $save_kernel_filename = $kernel_filename;
				getKernelFilename();
				if ($kernel_filename ne $save_kernel_filename)
				{
					print "kernel_filename changed to $kernel_filename\n";
					$kernel_filetime = 0;	# will always upload it if auto_upload
				}
			}
			else
			{
				print "WARNING: $registry_filename disappeared!\n";
			}
		}
		if ($watch_kernel)	# == $auto_upload ?!?
		{
	        $check_time = getFileTime($kernel_filename);
			if ($check_time != $kernel_filetime)
			{
				$kernel_filetime = $check_time;
				if ($check_time)
				{
					print "$kernel_filename changed\n";
					$kernel_file_changed = 1;
					if ($port)
					{
						print "AUTO-REBOOTING rpi (sending ctrl-B)\n";
						$port->write("\x02");
						$kernel_filetime = $check_time;
					}
					else
					{
						print "WARNING: cannot reboot rpi - COM$COM_PORT is not open!\n";
					}
				}
				else
				{
					print "WARNING: kernel $kernel_filename not found!\n";
				}
			}
		}
	}
    
}   # systemCheck()



sub listen_for_arduino_thread
	# watch for a process indicating an Arduino build is happening
	# and disconnect the comm port if it is
{
    while (1)
    {
        my $pl = Win32::Process::List->new();
        my %processes = $pl->GetProcesses();

        # print "PROCESS::LIST\n";
        my $found = 0;
        foreach my $pid (sort {$processes{$a} cmp $processes{$b}} keys %processes )
        {
            my $name = $processes{$pid};
            # print "$name\n" if $name;
            if ($name eq "arduino-builder.exe")
            {
                # print "Found process arduino-builder.exe\n"; 
                $found = 1;
                last;
            }
        }
            
        if ($found && !$in_arduino_build)
        {
            $in_arduino_build = 1;
            print "in_arduino_build=$in_arduino_build\n";
        }
        elsif ($in_arduino_build && !$found)
        {
            sleep(1);
            $in_arduino_build = 0;
            print "in_arduino_build=$in_arduino_build\n";
        }
        
        sleep(1);
    }
}



#--------------------------------------------------
# uploadSREC() for original dwelch bootloader10
#--------------------------------------------------

sub uploadSREC
{
    print "uploadSREC($kernel_filename)\n";
    
    # open the file and send it
    
    my $count = 0;
    if (open(INFILE,"<$kernel_filename"))
    {
        binmode INFILE;
        while (my $line = <INFILE>)
        {
            print "line($count)\n" if $count++ % 100 == 0;
            # print "<-- $line";

            # for (my $i=0; $i<length($line); $i++)
            # {
            
                my ($bytes,$s) = $port->read(1);
                while ($bytes)
                {
                    print $s;
                    sleep(0.0001);
                    ($bytes,$s) = $port->read(1);
                }
                
                if ($in->GetEvents())
                {
                    my @event = $in->Input();
                    if (isEventCtrlC(@event))
                    {
                        print "quitting due to ctrl-C\n";
                        last;
                    }
                }
    
                # my $c = substr($line,$i,1);
                my $sent = $port->write($line);
                if ($sent != length($line))
                {
                    close INFILE;
                    print "ERROR could not send byte\n";
                    return;
                }
                
                sleep(0.0001);    # 0.001);

            # }   # for each byte
        }   # for each line
        
        close INFILE;
        
		sleep(0.01);
		print "sending 'g' for GO\n";
		if (!$port->write('G'))
		{
			print "ERROR could not send 'G'\n";
		}
    }
    else
    {
        print "ERROR could not open $kernel_filename\n";
    }
}


#---------------------------------------------------
# uploadXModem for use with Placid bootloader
#---------------------------------------------------
# concerns: limit of 255x128 = 32K bytes

my $DEBUG_XMODEM = 0;

my $MODEM_SOH = 0x01 ;       
my $MODEM_ACK = 0x06 ;
my $MODEM_EOF = 0x28;
my $MODEM_EOT = 0x04 ;

my $MODEM_STX = 0x02 ;
my $MODEM_NAK = 0x15 ;
my $MODEM_CAN = 0x18 ;
my $MODEM_C   = 0x43 ;

my $BLOCK_SIZE = 128;


sub uploadXModem
{
    print "uploadXModem($kernel_filename)\n";
    
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	  	$atime,$mtime,$ctime,$blksize,$blocks) = stat($kernel_filename);
    if (!$size)
    {
        print "XMODEM ERROR - could get filesize for $kernel_filename\n";
        return;
    }
    if (!open(FH,"<$kernel_filename"))
    {
        print "XMODEM ERROR - could get open $kernel_filename\n";
        return;
    }

    binmode(FH);
    my $data;
    my $bytes =  read (FH, $data, $size);
    close(FH);
    
    if ($bytes != $size)
    {
        print "XMODEM ERROR - could get read expected: $size got $bytes\n";
        return;
    }
    if (length($data) != $size)
    {
        print "WARNING - size=$size length=".length($data)."\n";
        return;
    }
    
    print "XMODEM sending $kernel_filename($size)\n";
    
    my $off = 0;
    my $block = 1;
    my $retries = 0;
    while ($off < $size)
    {
NAK:        
        my $bytes = $size - $off;
        $bytes = $BLOCK_SIZE if $bytes > $BLOCK_SIZE;
        
        print "sending block $block\n" if $DEBUG_XMODEM;
        
        my $check = 0 + $MODEM_SOH + 255;
        
        my $buf = '';
        $buf .= chr($MODEM_SOH);
        $buf .= chr($block);
        $buf .= chr(255 - ($block & 0xff));
        
        for (my $i=0; $i<$bytes; $i++)
        {
            my $c = substr($data,$off+$i,1);
            $check += ord($c);
            $buf .= $c;
        }
        for (my $i=$bytes; $i<128; $i++)
        {
            $buf .= chr(0);
        }

        # if ($off + $bytes >= $size)
        # {
        #     $buf .= chr($MODEM_EOF);
        #     $check += $MODEM_EOF;
        # }
        
        $buf .= chr($check & 0xff);
        # display_bytes(0,0,"block($block)",$buf,length($buf)) if $DEBUG_XMODEM;
        $port->write($buf);
        
        my ($len,$c) = $port->read(1);
        while (!$len)
        {
            ($len,$c) = $port->read(1);
        }
        if (ord($c) != $MODEM_ACK)
        {
            print "XMODEM ERROR - block($block) expected ACK($MODEM_ACK)  got(".ord($c).")\n";
            goto NAK if $retries++ < 3;
            $port->write(0x1b);
            return;
        }
        print "sent block $block\n" if $DEBUG_XMODEM;
        $block++;
        $off += $bytes;
        $retries = 0;
    }
    
    print "sending EOT\n" if $DEBUG_XMODEM;
    $port->write(chr($MODEM_EOT));  
    my ($len,$c) = $port->read(1);      # he always sends a NAK here ...
    ($len,$c) = $port->read(1);
    if (ord($c) != $MODEM_ACK)
    {
        print "XMODEM ERROR - EOT expected ACK($MODEM_ACK)  got(".ord($c).")\n";
        return;
    }
    
    ($len,$c) = $port->read(1);     # read one more funny character
    ($len,$c) = $port->read(1);     # read one more funny character
    print "finished sending $kernel_filename\n";
    return 1;
}




#-------------------------------------------
# uploadBinary - my binary protocol
#-------------------------------------------
# as implemented in my circle/_prh/bootloader

my $BS_ACK = 'k';
my $BS_NAK = 'n';
my $BS_QUIT ='q';
my $BS_BLOCKSIZE = 2048;
my $BS_TIMEOUT = 500;


sub send32Binary
{
    my ($value) = @_;
    for (my $i=3; $i>=0; $i--)
    {
        my $byte = $value >> ($i*8) & 0xff;
        # print "send32binary($byte)\n";
        $port->write(chr($byte));
    }
}


sub getAckNak
{
    my $now = time();
    # print "getting acknack ...\n";
    while (time() < $now + $BS_TIMEOUT)
    {
        my ($bytes,$s) = $port->read(1);
        if ($bytes)
        {
            # print "Got acknak($s)\n";
            return substr($s,0,1);
        }
    }
    print "timed out getting acknak\n";
    return 0;
}


sub clearInputBuffer
{
    my ($BlockingFlags, $InBytes, $OutBytes, $LatchErrorFlags) = $port->status();
    # print ">$BlockingFlags, $InBytes, $OutBytes, $LatchErrorFlags\n";
    my ($t1,$t2) = $port->read($InBytes) if $InBytes;
    $t2 = '' if !$t1;
    $t2 =~ s/\n|\r//g;
    print "clearing input buffer($t2) ...\n" if $t2;
}


sub uploadBinary
    # implements protocol as described in kernel.cpp
    # 	  <--- send 8 byte hex encoded length
    # 	  ---> receive ACK or QUIT
	# 	  <--- send filename, create and mod time
    # 	  ---> receive ACK or QUIT
	# repeat as necessary:
    # 	  <--- send packet (4 byte block_num, 2048 bytes, 2 byte checksum)
    # 	  ---> receive ACK, NAK, or QUIT
    #          resend packet if NAK, quit if QUIT
	# finish:     
    # 	  <--- send 8 byte hex encoded checksum
    # 	  ---> receive ACK or QUIT
{
    print "uploadBinary($kernel_filename)\n";
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	  	$atime,$mtime,$ctime,$blksize,$blocks) = stat($kernel_filename);

    if (!open(FH,"<$kernel_filename"))
    {
        print "ERROR - could get open $kernel_filename\n";
        return;
    }

    binmode(FH);
    my $data;
    my $bytes =  read (FH, $data, $size);
    close(FH);
    
    if ($bytes != $size)
    {
        print "ERROR - could not read expected: $size got $bytes\n";
        return;
    }


    print "sending size=$size\n";
    clearInputBuffer();
    send32Binary($size);
    my $reply = getAckNak();
    return if !$reply || $reply ne $BS_ACK;
	
	# send the actual root filename 
	
	$ctime = time();
	my $filename = $kernel_filename;  # "kernel_test.img";
	$filename =~ s/^.*\///;
	
	$mtime =
		(2019 	<< (16+9)) 	|
		(4 		<< (16+5)) 	|
		(6      << 16)		|
		(17     << 11)	    |
		(35     << 5) 		|
		11;

	#	The FAT date and time is a 32-bit value containing two 16-bit values:
	#		* The date (lower 16-bit).
	#		* bits 0 - 4:  day of month, where 1 represents the first day
	#		* bits 5 - 8:  month of year, where 1 represent January
	#		* bits 9 - 15: year since 1980
	#		* The time of day (upper 16-bit).
	#		* bits 0 - 4: seconds (in 2 second intervals)
	#		* bits 5 - 10: minutes
	#		* bits 11 - 15: hours
	
	print "sending filename($filename),mtime($mtime),and ctime now($ctime)\n";
    clearInputBuffer();
	$port->write($filename);
	$port->write(chr(0));
    send32Binary($mtime);
    send32Binary($ctime);
    $reply = getAckNak();
    return if !$reply || $reply ne $BS_ACK;

	my $addr = 0;
    my $num_retries = 0;
    my $total_sum = 0;
    my $block_num = 0;
    my $num_blocks = int(($size + $BS_BLOCKSIZE - 1) / $BS_BLOCKSIZE);
    while ($block_num < $num_blocks)
    {
redo_block:

        clearInputBuffer();

        my $part_sum = 0;
        my $left = $size - $addr;
        $left = $BS_BLOCKSIZE if $left > $BS_BLOCKSIZE;
        my $temp_sum = $total_sum;

        # printf("sending block($block_num)\n");
        send32Binary($block_num);
        $reply = getAckNak();
        return if !$reply || ($reply ne $BS_ACK);
        
        # printf("checksum data $left at $addr ...\n");
        for (my $i=0; $i<$left; $i++)
        {
            my $c = substr($data,$addr + $i,1);
            $part_sum += ord($c);
            $temp_sum += ord($c);
            # $port->write($c);    
        }

        # printf("sending data $left at $addr ...\n");
        $port->write(substr($data,$addr,$left));

        # printf("sending part_sum($part_sum)\n");
        send32Binary($part_sum);
        
        $reply = getAckNak();
        return if !$reply || ($reply ne $BS_ACK && $reply ne $BS_NAK);
        if ($reply eq $BS_NAK)
        {
            print "--> retry($num_retries) block($block_num)\n";
            if ($num_retries++ > 5)
            {
                printf("ERROR - retry($num_retries) timeout on block($block_num)\n");
                return;
            }
            sleep(0.05);
            goto redo_block
        }
        
        $block_num++;
        $addr += $left; # $BS_BLOCKSIZE
        $total_sum = $temp_sum;
        $num_retries = 0;
    }
    
    print "sending data final checksum ($total_sum) ...\n";
    send32Binary($total_sum);
    $reply = getAckNak();
    return if !$reply || ($reply ne $BS_ACK);
    
    print "upload finished sucessfully\n";
}



#--------------------------------
# uploadTFTP
#--------------------------------
# For use with ethernet connection to rpi
# I think I had to make some

sub uploadTFTP
{
	my $TFTP_IP_ADDRESS = "192.168.0.250";
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	  	$atime,$mtime,$ctime,$blksize,$blocks) = stat($kernel_filename);
	
	print "uploadTFTP($kernel_filename($size)\n";
	
	my $tftp = Net::TFTP->new(
		$TFTP_IP_ADDRESS,
		# Port => 8080,
		# Debug => 1,
		Timeout => 1,
		Retries => 5,
		BlockSize => 1024);
	if ($tftp)
	{
		$tftp->binary();
		my $dest_name = $kernel_filename;
		$dest_name =~ s/^.*\///g;
		if (!$tftp->put($kernel_filename,$dest_name))
		{
			print "ERROR - could not TFTP PUT $kernel_filename\n".$tftp->error();
		}
	}
	else
	{
		print "ERROR - could not connect to TFTP server at $TFTP_IP_ADDRESS\n";
	}
}



sub prh_yield_console()
	# I modified my local copy of Perl/site/lib/Net/TFTP.pm
	# to add calls to this from the TFTP recv() and read() to
	# allow me to monitor the TFTP transfer from the serial
	# console, particularly to allow CTRL-C while a transfer
	# was in progress.  Otherwise you cannot quit the console
	# if the TFTP does not work right .. you have to close the
	# whole DOS box. It is not needed, of course, if you don't use
	# this script.  The bootloader TFTP implementation is
	# independent of (does not depend on) this script.
{
	if ($port)
	{
		readProcessPort();
	}
}



#--------------------------------
# upload dispatcher
#--------------------------------

sub uploadGeneric
{
    if ($USE_BOOTLOADER eq $kPLACID)
    {
        print("uploadGeneric() calling uploadXModem()\n");
        uploadXModem();
    }
    elsif ($USE_BOOTLOADER eq $kSREC)
    {
        print("uploadGeneric() calling uploadSREC()\n");
        uploadSREC();
    }
	elsif ($USE_BOOTLOADER eq $kTFTP)
	{
        print("uploadGeneric() calling uploadTFTP()\n");
        uploadTFTP();
	}
    else
    {
        print("uploadGeneric() calling uploadBinary()\n");
        uploadBinary();
    }
}


#--------------------------------------------------
# Main Command Line
#--------------------------------------------------

$con->Title("initializing ...");

# parse command line

my $arg_num = 0;
while ($arg_num < @ARGV)
{
    my $arg = $ARGV[$arg_num++];
    if ($arg =~ /^-(.*)/)
    {
		my $val = $1;
		$echo = 1 if ($val eq 'echo');
		$xlat_crlf = 1 if ($val eq 'crlf');
        if ($val eq 'arduino')
        {
            $watch_arduino = 1;
        }
        if ($val eq 'teensy')
        {
			$teensy = 1;
            $watch_arduino = 1;
            # $watch_kernel = 1;
        }
        if ($val eq 'rpi')
        {
			$rpi = 1;
            $COM_PORT = $RPI_COM_PORT;
            $BAUD_RATE = $RPI_BAUD_RATE;
            $allow_uploads = 1;
			$auto_upload = 1;
            $watch_kernel = 1;
			$only_if_changed = 1;
		}
		if ($val eq 'only_if_changed')
		{
			$only_if_changed = 1;
		}
		if ($val eq 'auto')
		{
			$auto_upload = 1;
			$watch_kernel = 1;
		}
		if ($val =~ s/^upload//)
		{
			$USE_BOOTLOADER = $val;
            $allow_uploads = 1;
		}
    }
    elsif ($arg =~ /^\d+$/)
    {
        if ($arg >= 100)
        {
            $BAUD_RATE = $arg;
        }
        else
        {
            $COM_PORT = $arg;
        }
    }
}


print "COM$COM_PORT at $BAUD_RATE baud\n";
print "watch_arduino\n" if $watch_arduino;
print "allow_uploads $USE_BOOTLOADER\n" if $allow_uploads;
print "auto_upload\n" if $auto_upload;
print "watch_kernel\n" if $watch_kernel;

        
my $kernel_upload_re =
    $USE_BOOTLOADER eq $kPLACID ?
        'press \[space\] for X\/YMODEM upload' :
    $USE_BOOTLOADER eq $kSREC ?
        '^\s*SREC\s*$' :
	$USE_BOOTLOADER eq $kTFTP ?
		'Try "tftp -m binary' :
	# default = my binary bootloader		
        'Press <space> within \d+ seconds to upload file';
		
my $kernel_upload_wait_for =
    $USE_BOOTLOADER eq $kPLACID ?
        'Start X\/YMODEM upload when ready' :
    $USE_BOOTLOADER eq $kBINARY ?
        'Upload file ...' :
        '';

if ($watch_arduino)
{
    my $arduino_thread = threads->create(\&listen_for_arduino_thread);
    $arduino_thread->detach();
}


# setup kernel filename for watching

sub getKernelFilename
{
	if (!open(IFILE,"<$registry_filename"))
	{
		printf("ERROR - could not open registry_file $registry_filename for reading!\n");
	}
	else
	{
		$kernel_filename = <IFILE>;
		$kernel_filename =~ s/\s+$//g;
		$kernel_filename =~ s/\\/\//g;
		close IFILE;
		printf "got kernel filename=$kernel_filename\n";
	}
}


if ($allow_uploads || $auto_upload || $watch_kernel)
{
	if (!$registry_filetime)
	{
		printf("ERROR - could not open registry_file $registry_filename!\n");
	}
	else
	{
		getKernelFilename();
		if (!$kernel_filename)
		{
			printf("WARNING - no kernel filename found in $registry_filename!\n");
		}
		else
		{
			$kernel_filetime = getFileTime($kernel_filename);
			if (!$kernel_filetime)
			{
				printf("WARNING - kernel $kernel_filename not found!\n");
			}
			else
			{
				print "kernel=$kernel_filename\n";
			}
		}
	}
}



#-------------------------------------------
# Main Loop
#-------------------------------------------

my $esc_cmd = '';
my $in_line = '';

sub readProcessPort
{
	my ($BlockingFlags, $InBytes, $OutBytes, $LatchErrorFlags) = $port->status();
	# print ">$BlockingFlags, $InBytes, $OutBytes, $LatchErrorFlags\n";
	if ($InBytes)
	{
		my ($bytes,$s) = $port->read($InBytes);
		for (my $i=0; $i<$bytes; $i++)
		{
			my $c = substr($s,$i,1);
			if ($esc_cmd)
			{
				$esc_cmd .= $c;
				# print("adding escape command(".ord($c).")=$c\n");
				if ($esc_cmd =~ /\x1b\[(\d+)m/)
				{
					my $color = $1;
					# print "setting color($color)\n";
					$con->Attr(colorAttr($color));
					$esc_cmd = '';
				}
				elsif ($esc_cmd =~ /\x1b\[[23]J/)
				{
					$con->Cls();
					$esc_cmd = '';
				}
				elsif (length($esc_cmd) > 5)
				{
					$esc_cmd = '';
				}
			}
			elsif (ord($c) == 27)
			{
				# print("starting escape command\n");
				$esc_cmd = $c;
			}
			else
			{
				$in_line .= $c;
				print $c;
				$con->Attr($COLOR_CONSOLE)
					if ord($c) == 13 ||
					   ord($c) == 10;
			}
		}
		$con->Flush();
		
		if ($in_line =~ /\n/)
		{
			if ($auto_upload && (!$only_if_changed || $kernel_file_changed))
			{
				my $do_upload = ($in_line =~ $kernel_upload_re);
				$in_line = '' if $in_line =~ /\r|\n/;
				if ($do_upload)
				{
					if (-f $kernel_filename)
					{
						$kernel_file_changed = 0;
						$port->write(" ") if $USE_BOOTLOADER eq $kBINARY;
						sleep(0.5);
						uploadGeneric();
					}
					else
					{
						print "WARNING - $kernel_filename not found. Not uploading!\n";
					}
				}
			}
			$in_line = '';
		}
	}

}





while (1)
{
    #--------------------------
    # receive and display 
    #--------------------------
    
    if ($port)
    {
		readProcessPort();
    }   # if $port
    
    
    #-----------------------------
    # check for any input events
    #-----------------------------
    # highest priority is ctrl-C
    
    if ($in->GetEvents())
    {
        @event = $in->Input();
        # print "got event '@event'\n" if @event;
        if (@event && isEventCtrlC(@event))				# CTRL-C
        {
            print "exiting console\n";
            if ($port)
            {
                $port->close();
                $port = undef;
            }
            exit(0);
        }

        my $char = getChar(@event);
		if (defined($char))
		{
			if ($con && ord($char) == 4)                  	# CTRL-D
			{
				$con->Cls();    # manually clear the screen
			}
			elsif ($port)
			{
				if ($allow_uploads && ord($char) == 24)     # CTRL-X
				{
					$port->write(" ") if $USE_BOOTLOADER eq $kBINARY;
					sleep(0.5);
					uploadGeneric();
				}
			
				# otherwise, send the character to the com port
		
				else	# if (defined($char))
						# ALL OTHER CHARACTERS
				{
					$port->write($char);
					if (ord($char) == 13)
					{
						$port->write(chr(10)) if $xlat_crlf;
						print "\r\n" if $echo;
					}
					elsif ($echo && ord($char) >= 32)
					{
						print $char;
					}
				}
			}
		}
    }

    #---------------------
    # arduino check
    #---------------------
    # take teensy offline if in_arduino

    if ($in_arduino_build && $port)
    {
        $con->Title("COM$COM_PORT closed for Arduino Build");
        $port->close();
        $port = undef;
        showStatus();
    }            
    
    
    #---------------------
    # do a system check
    #---------------------
    # check immediately for opened port
    # but only every so often for closed / kernel changes ...

    if (!$port && !$in_arduino_build)
    {
        $port = initComPort();
    }
    
    if (time() > $system_check_time + $SYSTEM_CHECK_TIME)
    {
        $system_check_time = time();
        systemCheck();
    }
	
	sleep(0.1);		# keep machine from overheating

    
}   # while (1) main loop





1;
