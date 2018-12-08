@ECHO off

IF "%1"=="" (
GOTO :helpInfo
)
IF /I "%1"=="-h" (
GOTO :helpInfo
)
IF /I "%1"=="-help" (
GOTO :helpInfo
)

IF /I "%1"=="all" (
SET x86=1
SET x64=1
SET ARM=1
SET ARM64=1
ECHO Building all available platforms...
)
IF /I "%1"=="x86" (
SET x86=1
)
IF /I "%1"=="x64" (
SET x64=1
)
IF /I "%1"=="ARM" (
SET ARM=1
)
IF /I "%1"=="ARM64" (
SET ARM64=1
)

IF DEFINED x86 (
ECHO Building x86 platform...
cmake --build build-x86 --config Release
)
IF DEFINED x64 (
ECHO Building x64 platform...
cmake --build build-x64 --config Release
)
IF DEFINED ARM (
ECHO Building ARM platform...
cmake --build build-arm --config Release
)
IF DEFINED ARM64 (
ECHO Building ARM64 platform...
cmake --build build-arm64 --config Release
)
GOTO :end

:helpInfo
ECHO Available options are:
ECHO - all
ECHO - x86
ECHO - x64
ECHO - ARM
ECHO - ARM64
PAUSE
GOTO :end

:end
:: Require to unset variables at end of the program when console is still open.
SET x86=
SET x64=
SET ARM=
SET ARM64=