:: must be native (x64 for x64, x86 for x86)
cd /d %~dp0
cd icon
magick convert 16.png 24.png 32.png 48.png 64.png -colors 256 icon.ico
cd ..

cl /EHsc /c main.cpp
cl /EHsc /c windowed.cpp
rc resources.rc
link main.obj windowed.obj resources.res Magnification.lib Shell32.lib Ole32.lib Advapi32.lib User32.lib -out:ColorEffectCmd.exe
