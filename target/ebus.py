#!/usr/bin/python

import serial
from enum import Enum

class id(Enum):
    bw = 0x00
    capt_toit = 0x02 # TK0
    capt_depart = 0x05
    capt_retour = 0x06
    haut_ballon = 0x09
    bas_ballon = 0x0b
    power = 0x1b
    energy = 0x1c
    heure_fonc = 0x1f
    ps = 0x5e

    @classmethod
    def has_value(cls, value):
        return any(value == item.value for item in cls)

class mode(Enum):
    notSync = 0
    sync = 1
    header = 2
    data = 3
    waitAck = 4
    resp = 5
    respData = 6

class prefix(Enum):
    sync = 0xaa
    ack = 0x00
    nack = 0xff

ser = serial.Serial('/dev/ttyS0', 2400, timeout=1)
m = mode.notSync
idx = 0
header = []
data = []
len = 0

def ebus_getch(c):
    global m, idx, header, data, len

    #~ print 'm', m, hex(c)
    if c == prefix.sync:
        # if m != mode.notSync:
        #     print 'unexpected sync'
        idx = 0
        m = mode.header
        header = []
        data = []
        #~ print 'sync'
        return
    elif m == mode.notSync:
        return

    if m == mode.header:
        header.append(c)
        idx = idx + 1
        #~ print 'header', idx
        if idx == 5:
            m = mode.data
            idx = 0
            len = c
            #~ print 'header', header

    elif m == mode.data or m == mode.respData:
        if idx < len:
            if idx > 0 and header[-1] == 0x09 and c == 0x01:
                data.append(prefix.sync)
            else:
                data.append(c)
            idx = idx + 1

        # crc
        elif idx == len:
            #~ print 'data', data
            if data[0] == 0x41:
                type = data[1]
                value = (data[3] << 8) | data[2]

                if id.has_value(type):
                    print 'type', type, 'value', value
            m = mode.waitAck

    elif m == mode.waitAck:
        if c == prefix.nack:
            print 'nack'
        elif c != prefix.ack:
            print 'ack expected, received %x' % c
        m = mode.notSync


while 1:
    try:
        buf = ser.read(30)
    except (OSError, serial.SerialException):
        print 'ser read failed'
        pass

    for c in buf:
        ebus_getch(ord(c))
