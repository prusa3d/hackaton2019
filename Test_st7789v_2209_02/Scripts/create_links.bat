%~d0
CD %~dp0
CD ..

REM remove "symlink" files after git clone under windows
IF EXIST Common IF NOT EXIST Common\ DEL /Q Common
IF EXIST Drivers IF NOT EXIST Drivers\ DEL /Q Drivers
IF EXIST Middlewares IF NOT EXIST Middlewares\ DEL /Q Middlewares
IF EXIST GuiAPI IF NOT EXIST GuiAPI\ DEL /Q GuiAPI
IF EXIST Gui IF NOT EXIST Gui\ DEL /Q Gui

REM create symlinks (administrator privileges requiered)
MKLINK /D Common ..\Marlin_A3ides_Common
MKLINK /D Drivers ..\Drivers
MKLINK /D Middlewares ..\Middlewares
MKLINK /D GuiAPI ..\GuiAPI\Src
MKLINK /D Gui ..\Gui

PAUSE
EXIT
