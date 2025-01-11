To create a bootable rPi circle App SD Card
i.e. for the Looper

- Uses a Regular FAT32 formatted SD Card
- Copy the contents of this directory to it
- Copy an appropriate kernel7.img to the SD Card

Note that the recovery.img is special and MUST BE
built with #define ARM_ALLOW_MULTI_CORE commented out
of /circle/include/sysconfig.h !!!


