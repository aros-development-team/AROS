@ECHO OFF
REM
REM lzenv.bat - script to set up laszlo env vars
REM
REM * R_LZ_COPYRIGHT_BEGIN ***************************************************
REM * Copyright 2001-2005 Laszlo Systems, Inc.  All Rights Reserved.         *
REM * Use is subject to license terms.                                       *
REM * R_LZ_COPYRIGHT_END *****************************************************

set LPS_HOME=c:\Program Files\Laszlo Presentation Server 3.0b2\Server\lps-3.0b2

set LZCP=%LPS_HOME%\WEB-INF\lps\server\build
for /f "usebackq delims=" %%d in (`dir /s /b "%LPS_HOME%\3rd-party\jars\dev\*.jar"`) do call :add %%~sd
for /f "usebackq delims=" %%d in (`dir /s /b "%LPS_HOME%\WEB-INF\lib\*.jar"`) do call :add %%~sd
set LZCP=%LZCP%;%ANT_HOME%\lib\junit.jar;%LPS_HOME%\WEB-INF\classes
goto :EOF

:add
set LZCP=%LZCP%;%*
goto :EOF

