# Marlin_A3ides
Marlin 2.0 firmware port for A3ides board

# building 
  Firmware can be built using make utility. 
  following shall be available in system path
  1. Make 
  2. arm-none-eabi-gcc 
  3. gcc
  4. utilites such as grep, sed, cat, etc.

## windows specific configs

### building from powershell
  add following to PATH env variable
  1. MSYS + MINGW
  2. arm-none-eabi-gcc
  
### using the STM workbench/eclipse based IDE)
  1. set an environmental variable CYGWIN_BIN_PATH pointing to cygwin bin folder
  2. add arm-gcc binary path to system path environment variable

## build automation
  1. build automation is achieved using Travis CI integrated with Github.
  2. When a new commit is pushed to master branch Travis runs release.py script.
  3. Release of the firmware happens if pushed commit has tag.
  4. On successful release results (*.elf *.hex, *.bin, *.dfu, etc.) are uploaded to the Github Release
  5. It is important to have the tagged commit for release


## Signing the firmware and generating .bbf package
This feature is not yet integrated to Travis CI
packages needed:
  1. python3
  2. python3-ecdsa

use the script "../Scripts/pack_fw.py" with appropriate arguments and private key

## Webserver integration
manual configurations and changes
1. Add Ethernet and LWIP from CubeMX 
2. In CubeMX configurations for Ethernet
	a. parameter setting -> Set PHY address = 0
	b. Mode -> RMII
3. LWIP
	a. enable HTTPD

4. add fsdata.c (from original cubeMX library) in ..lwip/apps/httpd folder and exclude from build
   fsdata.c contains the ST example html pages
   if you want to add custom page change the #define HTTPD_USE_CUSTOM_FSDATA 1 in LWip/src/include/lwip/apps/httpd_opts.h
   and provide "fsdata_custom.c"

5. to open web files from usb flash one has to implement fs_custom.c

6. httpd options are configured in LWip/src/include/lwip/apps/httpd_opts.h

To test html file load from external usb flash and page contents at the root location and connect to the board

## Fonts

* **big** - DejaVu Sans Mono, Bold 20px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **normal** - DejaVu Sans Mono 18px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **small** - DejaVu Sans Mono, Bold 20px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **terminal** - IBM ISO9 15px
  ([CC-BY-SA-4.0 license](https://int10h.org/oldschool-pc-fonts/fontlist/))
