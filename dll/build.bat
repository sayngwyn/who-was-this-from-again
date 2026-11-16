@echo off
rem SKSE DLL for SE 1.5.97, AE 1.6.x and 1.7.x.
rem Output: build\windows\x64\releasedbg\WhoWasThisFromAgain.dll
rem Needs VS 2022 Build Tools (MSVC 14.44) and xmake: on PATH, or in a shared\tools folder two levels up;
rem CommonLibSSE-NG is the lib\commonlibsse-ng submodule, or the folder named by COMMONLIBSSE_NG
rem (here: borrowed read only from Im Using This Now's submodule when lib\commonlibsse-ng is absent).
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 -vcvars_ver=14.44 >nul
set TOOLS=%~dp0..\..\shared\tools
if exist "%TOOLS%\xmake" (
    set "PATH=%TOOLS%\xmake;%PATH%"
    set "XMAKE_GLOBALDIR=%TOOLS%\xmake-cache"
)
if not exist "%~dp0lib\commonlibsse-ng" set "COMMONLIBSSE_NG=%~dp0..\..\Im Using This Now\dll\lib\commonlibsse-ng"
rem Strip this machine's folders from file names compiled into the DLL (__FILE__ / source_location).
rem Passed through cl's CL variable because xmake can't keep a flag with a space in it as one argument.
for %%I in ("%~dp0..\..") do set CL=/d1trimfile:"%%~fI\\"
cd /d "%~dp0"
xmake f -c -p windows -a x64 -m releasedbg --skyrim_vr=n -o build -y || exit /b 1
xmake -y || exit /b 1
