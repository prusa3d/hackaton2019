import os
import sys
import argparse
import subprocess
import shutil
from collections import namedtuple
from shutil import copyfile

SCRIPT_DIR    = os.path.dirname(os.path.realpath(sys.argv[0]))
MAKEFILE_DIR  = os.path.join(SCRIPT_DIR, "Marlin_A3ides_2209_02_noLCD")
BUILD_OUT_DIR = os.path.join(SCRIPT_DIR, "Marlin_A3ides_2209_02_noLCD", "Release_Boot")

FW_RELEASE_DIR   = os.path.join(SCRIPT_DIR, "FW_RELEASE")
MINI_FW_RELEASE_DIR   = os.path.join(FW_RELEASE_DIR, "MINI")
printer = namedtuple('printer', 'name relese_dir id')
MINI = printer('MINI', MINI_FW_RELEASE_DIR, 2)

def build(printer_type):
	if os.path.exists(printer_type.relese_dir):
	    shutil.rmtree(printer_type.relese_dir)
	os.makedirs(printer_type.relese_dir)
	PRINTER_TYPE_DEFINE = 'PRINTER=' + str(printer_type.id)
	FW_BUILDNR = 'FW_BUILDNR='+ str(os.environ.get("FW_BUILDNR"))
	subprocess.call(["make", "clean", PRINTER_TYPE_DEFINE, "CONFIG=Release_Boot"])
	subprocess.call(["make", "build", "-j8", FW_BUILDNR, PRINTER_TYPE_DEFINE, "CONFIG=Release_Boot"])

	copyfile(BUILD_OUT_DIR + '/Marlin_A3ides_2209_02_noLCD.bin', printer_type.relese_dir + "/Marlin_A3ides_2209_02_noLCD_" + printer_type.name + ".bin")
	copyfile(BUILD_OUT_DIR + '/Marlin_A3ides_2209_02_noLCD.dfu', printer_type.relese_dir + "/Marlin_A3ides_2209_02_noLCD_" + printer_type.name + ".dfu")
	copyfile(BUILD_OUT_DIR + '/Marlin_A3ides_2209_02_noLCD.elf', printer_type.relese_dir + "/Marlin_A3ides_2209_02_noLCD_" + printer_type.name + ".elf")
	copyfile(BUILD_OUT_DIR + '/Marlin_A3ides_2209_02_noLCD.hex', printer_type.relese_dir + "/Marlin_A3ides_2209_02_noLCD_" + printer_type.name + ".hex")

if not os.path.exists(FW_RELEASE_DIR):
	os.makedirs(FW_RELEASE_DIR)

os.chdir(MAKEFILE_DIR)

build(MINI)
