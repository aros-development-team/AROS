@ECHO OFF
REM
REM lzc.bat - Bash script to run laszlo compiler.
REM
REM * R_LZ_COPYRIGHT_BEGIN ***************************************************
REM * Copyright 2001-2005 Laszlo Systems, Inc.  All Rights Reserved.         *
REM * Use is subject to license terms.                                       *
REM * R_LZ_COPYRIGHT_END *****************************************************

if exist %~dp0..\Server (
    set LPS_HOME=%~dp0..\Server\lps-3.0b2
)

call "%LPS_HOME%\WEB-INF\lps\server\bin\lzenv.bat"

set CLASSPATH_SAVE=%CLASSPATH%
set CLASSPATH=%LZCP%
%JAVA_HOME%\bin\java %JAVA_OPTS% -DLPS_HOME="%LPS_HOME%" org.openlaszlo.compiler.Main %1 %2 %3 %4 %5 %6 %7 %8 %9
set CLASSPATH=%CLASSPATH_SAVE%
