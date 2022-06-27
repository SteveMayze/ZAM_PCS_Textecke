# This file is executed on every boot (including wake-boot from deepsleep)
#import esp
#esp.osdebug(None)

try:
    import usocket as socket
except:
    import socket

import machine, sys
from machine import Pin
from machine import UART
import time

#uos.dupterm(None, 1) # disable REPL on UART(0)
import gc
import webrepl
import network
import json

webrepl.start()
gc.collect()

ESSID = "WN0463FB2"
PASSWORD = "53240958296039385782"
 
def connect():
    sta_if = network.WLAN(network.STA_IF)
    if not sta_if.isconnected():
        print('connecting to network...')
        sta_if.active(True)
        sta_if.connect(ESSID, PASSWORD)
        while not sta_if.isconnected():
            pass
    print('network config:', sta_if.ifconfig())
 
connect()

led = Pin(2, Pin.OUT)