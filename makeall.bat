rem
rem This is makeall.bat for building all of my stuff
rem on circle that is not already built by the outer makeall.bat
rem
rem Does not bulid the bootloader
rem
rem makeall - by default builds the libraries 
rem makeall clean - cleans all libraries and examples
rem makeall examples - make the libraries and examples
rem makeall clean examples - just clean the examples


set PARAM1=%1
set PARAM2=%2

set DO_EXAMPLES=0
set DO_CLEAN=


if "%1" neq "clean" goto NO_CLEAN1
set DO_CLEAN=clean
:NO_CLEAN1
if "%2" neq "clean" goto NO_CLEAN2
set DO_CLEAN=clean
:NO_CLEAN2

if "%1" neq "examples" goto NO_EXAMPLES1
set DO_EXAMPLES=1
:NO_EXAMPLES1
if "%2" neq "examples" goto NO_EXAMPLES2
set DO_EXAMPLES=1
:NO_EXAMPLES2

rem if not clean, we build the whole thing
rem but if clean and examples, we only clean the examples

if "%DO_CLEAN%" neq "clean" goto DO_LIBS
if "%DO_EXAMPLES%" equ "1" goto DO_EXAMPLES


:DO_LIBS

cd audio
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd bt
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd devices
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd lcd
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd system
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd utils
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd ws
make %DO_CLEAN%
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

rem falling through on build, may be examples, or not

if "%DO_EXAMPLES%" equ "0" goto END_MACRO

rem if we are cleaning just the examples we will land here

:DO_EXAMPLES

cd examples

    cd 00-lcdDisplay
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 01-HardwareTest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 02-StereoPassThru
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 03-FreeverbTest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 04-OctoTest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 05-TSTest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 06-UITest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 07-LinkerTest
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 07-TestProgram
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 08-LinkerTest2
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    
        cd testProgram1
        make %DO_CLEAN%
        if %errorlevel% neq 0 exit /b %errorlevel%
        cd ..
        
        cd testProgram2
        make %DO_CLEAN%
        if %errorlevel% neq 0 exit /b %errorlevel%
        cd ..
    
    cd ..

    cd 09-testBT
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 10-VUMeter
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 11-aLooper
    make %DO_CLEAN%
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

cd ..
    
:END_MACRO
