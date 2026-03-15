@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86
echo test > nmake_output.log
nmake /A /f Makefile >> nmake_output.log 2>&1
