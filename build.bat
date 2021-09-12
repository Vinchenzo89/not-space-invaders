@echo off

if not exist .\build mkdir build
pushd build
cl -Od -Oi -Z7 ../win32_nsi.cpp /link user32.lib gdi32.lib winmm.lib
popd
