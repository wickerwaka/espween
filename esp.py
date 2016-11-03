#! /usr/bin/env python

import sys
import time
from RPi import GPIO

RST = 22
GPIO0 = 27
GPIO2 = 17

GPIO.setwarnings( False )
GPIO.setmode( GPIO.BCM )
GPIO.setup( [ RST, GPIO0, GPIO2 ], GPIO.OUT )

if len( sys.argv ) == 2:
	if sys.argv[1] == "serial":
		GPIO.output( GPIO0, GPIO.LOW )
		GPIO.output( GPIO2, GPIO.HIGH )
	elif sys.argv[1] == "flash":
		GPIO.output( GPIO0, GPIO.HIGH )
		GPIO.output( GPIO2, GPIO.HIGH )

# toggle reset
GPIO.output( RST, GPIO.LOW )
time.sleep( 0.01 )
GPIO.output( RST, GPIO.HIGH )

