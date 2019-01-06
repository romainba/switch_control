#!/usr/bin/python

import serial, struct, time, sys, datetime
from enum import Enum
import pymysql.cursors
import db

TABLE_NAME = "db0"

class cid(Enum):
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

cids = [ cid.bw, cid.capt_toit, cid.capt_depart, cid.capt_retour,
         cid.haut_ballon, cid.bas_ballon, cid.power, cid.energy,
         cid.heure_fonc, cid.ps ]

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


def ebus_getch(c):
    global m, idx, header, data, length, measures

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
            length = c
            #~ print 'header', header

    elif m == mode.data or m == mode.respData:
        if idx < length:
            if idx > 0 and header[-1] == 0x09 and c == 0x01:
                data.append(prefix.sync)
            else:
                data.append(c)
            idx = idx + 1

        # crc
        elif idx == length:
            #~ print 'data', data
            if data[0] == 0x41:
                dtype = data[1]
                value = (data[3] << 8) | data[2]
                value = struct.unpack('h', struct.pack('H', value))[0]
                try:
                    index = cids.index(dtype)
                except ValueError:
                    index = None

                if index is not None:
                    #~ print 'type', dtype, 'value', value
                    measures[index] = value

            m = mode.waitAck

    elif m == mode.waitAck:
        if c == prefix.nack:
            print 'nack'
        elif c != prefix.ack:
            print 'ack expected, received %x' % c
        m = mode.notSync


def db_create_table(con):
    try:
        with con.cursor() as cursor:
            sql = """
            create table db0(Id INT PRIMARY KEY AUTO_INCREMENT,
            date DATETIME, bw INT, capt_toit INT, capt_depart INT, capt_retour INT,
            haut_ballon INT, bas_ballon INT, power INT, energy INT, heure_fonc INT)
            """
            cursor.execute(sql)
            print "created table"
    finally:
        pass


def db_insert(date, data):
    global TABLE_NAME
    con = db.connect()
    try:
        with con.cursor() as cursor:
            sql = "insert into " + TABLE_NAME + \
                  "(date, bw, capt_toit, capt_depart, capt_retour, " + \
                  "haut_ballon, bas_ballon, power, energy, heure_fonc) " + \
                  "VALUES('" + date + "', " + ", ".join([str(d) for d in data]) + ")"
            cursor.execute(sql)
            print sql
    finally:
        con.commit()
        con.close()


m = mode.notSync
idx = 0
header = []
data = []
length = 0
t = time.time()
measures = [None] * len(cids)
prevMeasures = None

# create table if it does not exist
con = db.connect()
if not db.checkTableExists(con, TABLE_NAME):
    db_create_table(con)
con.close()

ser = serial.Serial('/dev/ttyS0', 2400, timeout=1)

while 1:
    try:
        buf = ser.read(30)
    except (OSError, serial.SerialException):
        print 'ser read failed'

    for c in buf:
        ebus_getch(ord(c))

    if None in measures:
        continue

    if prevMeasures == measures:
        continue

    d = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    db_insert(d, [measures[i] for i in range(9)])

    prevMeasures = measures
    measures = [None] * len(cids)
