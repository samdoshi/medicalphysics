#!/bin/sh
dfu-programmer at32uc3b0256 erase
dfu-programmer at32uc3b0256 flash medicalphysics.hex --suppress-bootloader-mem
dfu-programmer at32uc3b0256 launch
