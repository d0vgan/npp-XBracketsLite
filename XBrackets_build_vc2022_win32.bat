@echo off

set VC_ROOT=%ProgramFiles%\Microsoft Visual Studio\2022

if exist "%VC_ROOT%\Professional\VC\Auxiliary\Build\vcvarsall.bat" goto UseVcProfessional
if exist "%VC_ROOT%\Community\VC\Auxiliary\Build\vcvarsall.bat" goto UseVcCommunity

goto ErrorNoVcVarsAll

:UseVcProfessional
call "%VC_ROOT%\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
goto Building

:UseVcCommunity
call "%VC_ROOT%\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
goto Building

:Building
cd .\XBrackets
msbuild XBrackets_VC2022.vcxproj /t:Clean /p:Configuration=Release;Platform=Win32 /m:2
msbuild XBrackets_VC2022.vcxproj /t:Build /p:Configuration=Release;Platform=Win32 /m:2
goto End

:ErrorNoVcVarsAll
echo ERROR: Could not find "vcvarsall.bat"
goto End

:End
