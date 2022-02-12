@echo off

set ARC_EXE=7z.exe
for /f "tokens=1-3 delims=/.- " %%a in ('DATE /T') do set ARC_DATE=%%c%%b%%a

%ARC_EXE% u -tzip XBrackets%ARC_DATE%_src.zip @XBrackets_src.txt -mx5
%ARC_EXE% t XBrackets%ARC_DATE%_src.zip

if not exist XBrackets\Release goto no_dll_file_32bit
cd XBrackets\Release
%ARC_EXE% u -tzip ..\..\XBrackets%ARC_DATE%_dll.zip XBrackets.dll -mx5
cd ..
%ARC_EXE% u -tzip ..\XBrackets%ARC_DATE%_dll.zip XBrackets*.txt -mx5
cd ..
%ARC_EXE% t XBrackets%ARC_DATE%_dll.zip

:no_dll_file_32bit
if not exist XBrackets\x64\Release goto no_dll_file_64bit
cd XBrackets\x64\Release
%ARC_EXE% u -tzip ..\..\..\XBrackets%ARC_DATE%_dll_x64.zip XBrackets.dll -mx5
cd ..\..
%ARC_EXE% u -tzip ..\XBrackets%ARC_DATE%_dll_x64.zip XBrackets*.txt -mx5
cd ..
%ARC_EXE% t XBrackets%ARC_DATE%_dll_x64.zip

:no_dll_file_64bit
