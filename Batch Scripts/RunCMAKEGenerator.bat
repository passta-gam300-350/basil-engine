@echo off
set "CurrentDir=%cd%"
:: set color
color 0e
:: set title CMAKE VS2022 Generator
title CMAKE Solutions Generator
 
:logo
 type ascii.txt
:mainmenu
echo.
echo Build System Generator
echo ========================================================
echo 1. Generate ^& Clean VS2022 Solution
echo 2. Generate VS2022 Solution
echo 3. Clean VS2022 Solution
echo --------------------------------------------------------
echo 4. Generate ^& Clean VS2019 Solution
echo 5. Generate VS2019 Solution
echo 6. Clean VS2019 Solution
echo --------------------------------------------------------
echo 7. Build Solution - Multi Core
echo 8. Build Solution - Single Core
echo --------------------------------------------------------
echo 9. Exit
echo ========================================================
set /p choice=Select Option: 

if %choice%==1 goto generatecleanvs2022
if %choice%==2 goto generatevs2022
if %choice%==3 goto cleanvs2022
if %choice%==4 goto generatecleanvs2019
if %choice%==5 goto generatevs2019
if %choice%==6 goto cleanvs2019
if %choice%==7 goto buildvs
if %choice%==8 goto buildvs
if %choice%==9 goto exit



echo Invalid Choice, Try Again.
echo.
goto mainmenu



:buildvs
    echo Building Solution...
    cd ..\build
    for /f "tokens=2 delims==" %%a in ('wmic cpu get NumberOfLogicalProcessors /value') do set core_count=%%a
    echo Your CPU have %core_count% Cores.
    set /a core_count=(%core_count% *3)/2
    cmake --build . -j %core_count%
    cd "%CurrentDir%"
    echo Build Completed.
    pause
    goto checkError
:buildvssingle
    echo Building Solution...
    cd ..\build
    cmake --build .
    cd "%CurrentDir%"
    echo Build Completed.
    pause
    goto checkError
:generatecleanvs2022
    echo Generating and Cleaning VS2022 Solution...
    rmdir /s /q ..\build
    mkdir ..\build
    cd ..\build

    cmake -G "Visual Studio 17 2022" ..

    cd "%CurrentDir%"

    echo Solutions Generated, Access them in the build folder.
    pause
    goto checkError

:generatevs2022
    echo Generating VS2022 Solution...
    cd ..\build
    cmake -G "Visual Studio 17 2022" ..

    cd "%CurrentDir%"

    echo Solutions Generated, Access them in the build folder.
    pause
    goto checkError

:cleanvs2022
    echo Cleaning VS2022 Solution...
    rmdir /s /q ..\build
    mkdir ..\build

    cd "%CurrentDir%"

    echo Solutions Cleaned.
    pause
    goto checkError

:generatecleanvs2019
    echo Generating and Cleaning VS2019 Solution...
    rmdir /s /q ..\build
    mkdir ..\build
    cd ..\build

    cmake -G "Visual Studio 16 2019" ..

    cd "%CurrentDir%"

    echo Solutions Generated, Access them in the build folder.
    pause
    goto checkError

:generatevs2019
    echo Generating VS2019 Solution...
    cd ..\build
    cmake -G "Visual Studio 16 2019" ..

    cd "%CurrentDir%"

    echo Solutions Generated, Access them in the build folder.
    pause
    goto checkError

:cleanvs2019
    echo Cleaning VS2019 Solution...
    rmdir /s /q ..\build
    mkdir ..\build

    cd "%CurrentDir%"

    echo Solutions Cleaned.
    pause
    goto checkError

:exit
    exit

rem Error handling
:checkError
    IF %ERRORLEVEL% NEQ 0 (
        echo.
        echo Error: %ERRORLEVEL%
        echo.
        pause
        
    )
    goto mainmenu