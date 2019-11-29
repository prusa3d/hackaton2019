%~d0
CD %~dp0
CD ..
DEL /Q Middlewares
DEL /Q Drivers
DEL /Q Common
MKLINK /D Common ..\Marlin_A3ides_Common
MKLINK /D Drivers ..\Drivers
MKLINK /D Middlewares ..\Middlewares
PAUSE
EXIT
