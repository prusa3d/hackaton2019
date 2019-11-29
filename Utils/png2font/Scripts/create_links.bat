%~d0
CD %~dp0
CD ..
DEL /Q src\zlib
DEL /Q src\lpng
MKLINK /D src\zlib ..\..\..\Middlewares\Third_Party\zlib
MKLINK /D src\lpng ..\..\..\Middlewares\Third_Party\lpng
PAUSE
EXIT
