#!/bin/bash

rs485-setup /dev/ttyS4

export LD_LIBRARY_PATH=/home/smm/bin/lib
./SmartMeterMonitor/SmartMeterMonitor -c=config.ini


