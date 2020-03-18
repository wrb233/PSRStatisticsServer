@echo off

call %CPS_ROOT%\CPS\compile\pre_build-xp.bat

rem echo generate vcxproj.....
call protovcproj.bat

echo building project...

if "%1" == "debug" goto DEBUG
if "%1" == "" goto RELEASE 
if "%1" == "release" goto RELEASE
if "%1" == "all" goto RELEASE
if "%1" == "clean" goto CLEAN

echo Usage: build-xp.bat release/debug/all/clean/""
echo "      release|"" : Make version release          "
echo "      debug      : Make version debug            "
echo "      all        : Make version debug and release"
echo "      clean      : Make clean                    "
goto END

:RELEASE
MSbuild /t:rebuild /p:Configuration=Release
if "%1" == "all" goto DEBUG
goto END

:DEBUG
echo  --nmake projects
MSbuild /t:rebuild /p:Configuration=Debug
goto END

:CLEAN
goto END

:END
call %CPS_ROOT%\CPS\compile\post_build-xp.bat %1

rem
rem if mutil output,eg:
rem MSbuild name1.vcxproj /t:rebuild /p:Configuration=Debug
rem

echo  --build finished!

rem pause
