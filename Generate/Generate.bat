@echo off

set GENERATOR=%1
echo GENERATOR: %GENERATOR%

rem generator with underline
set generator_ul=%GENERATOR: =_%
echo generator_ul: %generator_ul%

cd ../
mkdir Build
cd Build

echo ÕýÔÚÉ¾³ý%generator_ul%...
if exist %generator_ul% rd %generator_ul% /q /s
timeout 2

md %generator_ul%
cd %generator_ul%
cmake -G %GENERATOR% ../..

pause