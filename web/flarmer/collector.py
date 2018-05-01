import SocketServer
import pymysql
import struct
import math

SOCK_HOST = ''
SOCK_PORT = 9268

DB_ADDR = '192.168.0.100'
DB_USER = 'flarmer'
DB_PASS = ''
DB_NAME = 'flarmer_db'

INSERT_MSMT = "insert into stream (ts, room, warmer, temp, humidity) "\
              "values (now(), %(room)s, %(wstate)s, %(temperature)s, %(humidity)s)"
MATCH_ROOM = "SELECT r.code as code from room as r join device as d on r.device_id = d.id where mac=%s"

class Measurement(object):
    def __init__(self, mac, wstate, temperature, humidity):
        self.mac = mac
        self.wstate = int(wstate)
        self.temperature = float(temperature)
        self.humidity = float(humidity)

def insertMeasurement(msm):
    try:
        conn = pymysql.connect(host=DB_ADDR, user=DB_USER, password=DB_PASS, db=DB_NAME)
        room = None
        with conn.cursor() as curs:
            curs.execute(MATCH_ROOM, msm.mac)
            room = curs.fetchone()
            if room is None:
                print "Couldn't find room for device with MAC: {}".format(msm.mac)
                return
            else:
                print "Device '{}' -> Room '{}'".format(msm.mac, room[0])

        with conn.cursor() as curs:
            curs.execute(INSERT_MSMT,
                dict(room=room[0], wstate=msm.wstate,
                    temperature=None if math.isnan(msm.temperature) else msm.temperature,
                    humidity=None if math.isnan(msm.humidity) else msm.humidity))
            if curs.rowcount:
                print "Added {} records".format(curs.rowcount)
                conn.commit()

    except StandardError as e:
        print "SQL Exception: "
        print e
    finally:
        conn.close()

class ReportHandler(SocketServer.StreamRequestHandler):
    def handle(self):
        msg = self.rfile.read()
        print "{} has sent {} bytes".format(self.client_address[0], len(msg))

        if len(msg) > 1:
            vers = ord(msg[0])
            if vers == 1:
                (macBytes, wstate, temperature, humidity) = struct.unpack_from('!6sBff', msg[1:])
                mac = ""
                for b in macBytes:
                    mac += "{:02x}".format(ord(b))
                print "mac {}, wstate {}, temp: {}, humi: {}".format(mac, wstate, temperature, humidity)

                msm = Measurement(mac, wstate, temperature, humidity)
                insertMeasurement(msm)
            else:
                print "Unknown message version {:02x}".format(vers)


def startServer():
    srv = SocketServer.ThreadingTCPServer((SOCK_HOST, SOCK_PORT), ReportHandler)
    srv.serve_forever()

if __name__ == "__main__":
    startServer()
