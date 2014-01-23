import gevent.monkey
gevent.monkey.patch_all()
import geventwebsocket.handler
import gevent.pywsgi
import bottle
from OSC import OSCServer
import sys
from geventwebsocket import WebSocketError
from serial import Serial
from serial.serialutil import SerialException
app = bottle.Bottle()
import atexit
from threading import Timer
from gevent import Timeout

class ScriptState(object):
    
    def __init__(self):
        self.webapps = []

    def register_webapp(self, ws):
        self.webapps.append(ws)
    
    def update_osc(self, args):

        if len(args) > 0:
            print("The data from osc %s" % args[0])
            theData = ""
            if args[0] == 16256:
                print("Found scott!")
                theData = "scottgreenwald.jpg"
                toggle_lockstate()
            elif args[0] == 10922 or args[0] == 10920:
                print("Found david!")
                theData = "davidcranor.jpg"
                toggle_lockstate()
            else:
                theData = args[0]
            WS_NEW = []
            for ws in self.webapps:
                try: 
                    ws.send(theData)
                    WS_NEW.append(ws)
                except WebSocketError:
                    print("Caught errror on dead websocket")
            self.webapps = WS_NEW

SCRIPT = ScriptState()

@app.get('/')
def index():
    return bottle.static_file('bcWebapp.html', root='.')

@app.get('/profiles')
def profiles():
    return bottle.static_file('bcProfiles.html', root='.')

@app.get('/static/:name')
def static(name):
    return bottle.static_file(name, root='./static/')

@app.route('/webapp')
def api(num=0):
    if bottle.request.environ.get('wsgi.websocket'):
        print('Webapp connected')
        ws = bottle.request.environ['wsgi.websocket']
        SCRIPT.register_webapp(ws)
        while True:
            message = ws.receive()
            if message is None:
                break
            print(message)

def serve_forever(server):
    server.serve_forever()

def toggle_lockstate():
    global lockState
    # TODO: limit on frequency of lock/unlock enforced by the Arduino,
    # but should probably be enforced here too with a (software) lock
    # and timed release. 
    if (lockState == True):
        print("unlocking")
        myserialport.write('u')
        lockState = False
    else:
        print("locking")
        myserialport.write('l')
        lockState = True    

def getReadyForMore():
    global readyForMore
    readyForMore = True
    print("Now I'm ready for more")

def setReadyTimer():
    r = Timer(5.0, getReadyForMore, ())
    r.start()

def flushIncessantly(serialport):
    global readyForMore
    cnt = 0
    while not readyForMore:
        cnt += 1
        if cnt % 100000 == 0:
            print("Flushed %s times" % cnt)
        serialport.flushInput()
        gevent.sleep()
    return 

def killThisGuy(greenlet):
    print "killing greenlet"
    greenlet.kill()

def listen_forever(serialport):
    global readyForMore
    global bigBuff
    while True:
        data = serialport.readline()
        if len(data) > 0:
            print("Main loop iteration with data")
        if readyForMore:
            cnt = 0
            if len(data) > 0:
                readyForMore = False
                print 'Got:', data
                toggle_lockstate()
                gevent.spawn_later(5, getReadyForMore)
                #gevent.spawn(setReadyTimer)
                #r = Timer(5.0, getReadyForMore, ())
                #r.start()
                # print("I'm past the timer")
                #THIS IS WHERE THE ACTION SHOULD HAPPEN
        else:
            # timer = Timeout(5).start()
            myFlushGreenlet = gevent.with_timeout(5, flushIncessantly, serialport)
            if myFlushGreenlet != None:
                gevent.spawn_later(6.0, killThisGuy, myFlushGreenlet)
                myFlushGreenlet.join()
            # myFlushGreenlet = gevent.spawn(flushIncessantly, serialport)
            # myFlushGreenlet.join(timeout=timer)
            print("FlushGreenlet returned")
            # while not readyForMore:
            #     cnt += 1
            #     if cnt % 10 == 0:
            #         print("Flushed %s times" % cnt)
            #     #numbytes = serialport.readinto(bigBuff)
            #     #print("Not ready for more and just read %s bytes" % numbytes)
            #     #serialport.readline()
            #     #serialport.flush()
            #     # #print("Input length %s" % len(data))
            #     # if len(data) > 100:
            #     #     print("Flushing input length %s" % len(data))
            #     # else:
            #     #     print("Not flushing data too short length %s" % len(data))
            #     serialport.flushInput()

def debug_callback(path, tags, args, source):
    global SCRIPT
    print("Args from debug message: %s" % args)
    SCRIPT.update_osc(args)

@atexit.register
def do_cleanup():
    print "Closing ports serial ports..."
    try:
        myserialport.close()
        myBodySerialPort.close()
    except SerialException:
        print("SerialException")
    print "goodbye"    

if __name__ == '__main__':
    bigBuff = bytearray(520)
    readyForMore = True
    lockState = True
    myServer = gevent.pywsgi.WSGIServer(('0.0.0.0', 8833), app,
                             handler_class=geventwebsocket.handler.WebSocketHandler)
    serverThread = gevent.spawn(serve_forever, myServer);
    print("Started the wsgi server thread.")

    # THIS THING BURNS CPU LIKE A MAD MOTHER FOOL
    # myOSCServer = OSCServer( ("localhost", 5006) )
    # myOSCServer.timeout = 0
    # myOSCServer.addMsgHandler( "/bcdata", debug_callback )
    # myOSCServer.addDefaultHandlers()
    # oscServerThread = gevent.spawn(serve_forever, myOSCServer)
    # print("Started the osc server thread.")

    # TODO: make serial devices command line arguments
    try:
        myserialport = Serial('/dev/tty.usbmodem1411', baudrate=9600)
        lockState = True;
    except SerialException:
        print("Couldn't connect to doorlock serial device.")

    try:
        myBodySerialPort = Serial('/dev/tty.usbmodem1431', baudrate=115200)
        incomingSerialServerThread = gevent.spawn(listen_forever, myBodySerialPort)
        print("Started incoming serial server thread.")
    except SerialException:
        print("Couldn't connect to incoming body serial")

    gevent.joinall([serverThread, incomingSerialServerThread])
    # gevent.joinall([serverThread, oscServerThread, incomingSerialServerThread])
