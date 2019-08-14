#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import pyudev

flash = (Path(__file__).parent / 'flash.sh').absolute()

def main():
    monitor = pyudev.Monitor.from_netlink(pyudev.Context())
    monitor.filter_by(subsystem='usb')
    monitor.start()

    for device in iter(monitor.poll, None):
        vendor = device.get('ID_VENDOR_ID')
        product = device.get('ID_MODEL_ID')
        if (device.action == 'add' and vendor == '03eb' and product == '2ff6'):
            subprocess.run([str(flash)])

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        sys.exit()

