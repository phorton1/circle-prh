rem
rem This is makeall.bat for building all of my stuff
rem on circle that is not already built by the outer makeall.bat

cd audio
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd devices
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd lcd
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd system
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd utils
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd ws
make %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..


cd examples

    cd 00-lcdDisplay
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 01-HardwareTest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 02-StereoPassThru
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 03-FreeverbTest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 04-OctoTest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 05-TSTest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 06-UITest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 07-LinkerTest
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 07-TestProgram
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    cd ..

    cd 08-LinkerTest2
    make %1 %2
    if %errorlevel% neq 0 exit /b %errorlevel%
    
        cd testProgram1
        make %1 %2
        if %errorlevel% neq 0 exit /b %errorlevel%
        cd ..
        
        cd testProgram2
        make %1 %2
        if %errorlevel% neq 0 exit /b %errorlevel%
        cd ..
    
    cd ..

cd ..
    