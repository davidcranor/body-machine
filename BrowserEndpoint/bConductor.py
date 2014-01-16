import gevent.monkey
gevent.monkey.patch_all()
import geventwebsocket.handler
import gevent.pywsgi
import msgpack
import bottle
import time
import pymongo
import json
from OSC import OSCServer
import sys
from time import sleep
import httplib
from geventwebsocket import WebSocketError

#[{'name': y, 'photo': 'url(https://lh5.googleusercontent.com/-p1Pu9my5DRk/AAAAAAAAAAI/AAAAAAAAO7M/XhO8H6GO5oQ/s120-c/photo.jpg)', 'color': "sky", 'thumbScore': 0, 'speakupScore': 0, 'cameraman': False} for y in sorted(list(set([x['character'] for x in lines])))]

app = bottle.Bottle()

class WebSocketHolder(object):
    def __init__(self):
        self.webapps = [];

    def register_webapp(self, ws):
        self.webapps.append(ws)


class ScriptState(object):
    
    def __init__(self, lines):
        for x, _ in enumerate(lines):
            lines[x]['number'] = x
        self.lines = lines
        self.character_lines = [x['character'] for x in lines]
        self.characters = [{'name': y, 'number': x} for x, y in enumerate(sorted(set(x['character'] for x in lines)))]
        self.name_to_num = {y['name']: x for x, y in enumerate(self.characters)}
        self.webapps = []
        self.glasses = []
        self.update_line(0)

    def _update_webapp(self, ws):
        ws.send(json.dumps({'line': self.current_line, 'subline': self.current_subline, 'character': self.current_character}))

    def register_webapp(self, ws):
        self.webapps.append(ws)
        self._update_webapp(ws)

    def register_glass(self, ws):
        self.glasses.append(ws)
        self.refresh_cards(self.characters[len(self.glasses) - 1:])
    
    def update_osc(self, args):

        if len(args) > 0:
            print("The data from osc %s" % args[0])
            theData = ""
            if args[0] == 16256:
                print("Found scott!")
                theData = "scottgreenwald.jpg"
            elif args[0] == 10922:
                print("Found david!")
                theData = "davidcranor.jpg"
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

    def update_line(self, line=None, subline=0):
        if line is not None:
            self.current_line = line
        try:
            if subline >= self.current_sublines:
                print('Auto advance')
                self.current_line += 1
                subline = 0
            if self.current_line >= len(self.lines):
                # TODO: Done!
                return
        except AttributeError:
            pass
        self.current_subline = subline
        self.current_sublines = len(self.lines[self.current_line]['line'])
        self.lines_future = self.lines[self.current_line:]
        self.character_lines_future = self.character_lines[self.current_line:]
        self.current_character = self.name_to_num[self.character_lines_future[0]]
        for ws in self.webapps:
            self._update_webapp(ws)
        self.refresh_cards(self.characters)

    def increment_line(self, inc):
        current_line = self.current_line + inc
        if current_line < 0:
            current_line = 0
        if current_line >= len(self.lines):
            current_line = len(self.lines) - 1
        if current_line != self.current_line or self.current_subline:
            update_line(current_line)

    def lines_to_time(self, lines):
        return sum([len(x) * .08 for x in lines])

    def refresh_cards(self, characters):
        if not len(self.glasses):
            return
        ws_send = []  # list of (character_next_line_ind, ws_ind, data)
        for c in characters:
            character = c['name']
            try:
                character_next_line_ind = self.character_lines_future.index(character)
                time_to_talk = self.lines_to_time(self.lines_future[0]['line'][self.current_subline:]) + sum(self.lines_to_time(x['line']) for x in self.lines_future[1:])
                time_to_talk = round(time_to_talk / 5) * 5
                time_to_talk = '%d' % time_to_talk if time_to_talk else '<5'
                cue_line = ('Cue (%d line, %s sec.): ' % (character_next_line_ind, time_to_talk)) + self.lines_future[character_next_line_ind - 1]['line'][-1][-8:] if character_next_line_ind else ' Your turn!'
                line = self.lines_future[character_next_line_ind]
                nline = len(line['line'])
                extra_cards = [['Done : D', 'Thanks']] if character_next_line_ind == 0 else []
                #if character_next_line_ind == 1:
                #    self.glass_send(self.glasses[self.name_to_num[character]], 'say', 'next');
                out = json.dumps([[y, '%s (%d/%d) %s' % (character, x, nline, cue_line)] for x, y in enumerate(line['line'])] + extra_cards)
                ws_send.append((character_next_line_ind, min(self.name_to_num[character], len(self.glasses) - 1), self.name_to_num[character], out))
                print((c, character_next_line_ind, cue_line, out))
            except ValueError:
                out = json.dumps([['You are done!', 'Thanks you were a star!']])
                ws_send.append((float('inf'), min(self.name_to_num[character], len(self.glasses) - 1), self.name_to_num[character], out))
                print((character, 'Done!'))
        ws_inds_prev = set()
        for _, ws_ind, character_num, data in sorted(ws_send):
            if ws_ind in ws_inds_prev:
                continue
            ws_inds_prev.add(ws_ind)
            if not (self.current_character == character_num and SCRIPT.current_subline):
                name = 'cardsReset' if self.current_character == character_num else 'cards'
                self.glass_send(self.glasses[ws_ind], name, data)

    def glass_send(self, ws, name, blob):
        try:
            ws.send(msgpack.dumps(['blob', name, blob]), binary=True)
        except KeyError:
            print('Could not send')

    def data(self):
        return {'characters': self.characters, 'lines': self.lines}


SCRIPT = ScriptState(json.load(open('lines.js')))

@app.get('/')
def index():
    return bottle.static_file('bcWebapp.html', root='.')

@app.get('/static/:name')
def static(name):
    return bottle.static_file(name, root='./static/')

@app.get('/data.js')
def data():
    return SCRIPT.data()

@app.get('/fromosc')
def hello():
    print("Got a message from osc.")
    # SCRIPT.update_osc()

@app.route('/osc')
def api(num=0):
    if bottle.request.environ.get('wsgi.websocket'):
        print('osc client connected')
        ws = bottle.request.environ['wsgi.websocket']
        while True:
            message = ws.receive()
            if message is None:
                break
            # message = json.loads(message)
            print(message)

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
            #message = json.loads(message)
            print(message)
            #SCRIPT.update_line(message['line'])
 
@app.route('/ws/:num')
@app.route('/ws/')
def api(num=0):
    if bottle.request.environ.get('wsgi.websocket'):
        ws = bottle.request.environ['wsgi.websocket']
        SCRIPT.register_glass(ws)
        try:
            while True:
                message = ws.receive()
                if message is None:
                    break
                message = msgpack.loads(message)
                print(message)
                is_current = SCRIPT.current_character == num or (len(SCRIPT.glasses) - 1 == num and SCRIPT.current_character >= num)
                if message[0] == 'blob' and message[1] == 'position' and is_current and int(message[2]) != SCRIPT.current_subline:
                    print('Updating line from position')
                    SCRIPT.update_line(subline=int(message[2]))
            ws.close()
        except geventwebsocket.WebSocketError, ex:
            print '%s: %s' % (ex.__class__.__name__, ex)

def serve_forever(server):
    server.serve_forever()
 
def debug_callback(path, tags, args, source):
    global SCRIPT
    print("Args from debug message: %s" % args)
    SCRIPT.update_osc(args)
    # conn = httplib.HTTPConnection("localhost:8833")
    # conn.request("GET", "/fromosc")

if __name__ == '__main__':
    myServer = gevent.pywsgi.WSGIServer(('0.0.0.0', 8833), app,
                             handler_class=geventwebsocket.handler.WebSocketHandler)
    serverThread = gevent.spawn(serve_forever, myServer);
    print("Started the wsgi server thread.")

    myOSCServer = OSCServer( ("localhost", 5006) )
    myOSCServer.timeout = 0
    myOSCServer.addMsgHandler( "/bcdata", debug_callback )
    oscServerThread = gevent.spawn(serve_forever, myOSCServer)
    print("Started the osc server thread.")

    gevent.joinall([serverThread, oscServerThread])

# WS = []
 
# def osc_listen(data):
#     global WS
#     # NOTE: We should catch errors here and create a new WS_NEW list for sockets that don't error, then swap them at the end
#     for ws in WS:
#         ws.send(data)
 
# if __name__ == '__main__':
#     gevent.spawn(osc_listener, osc_listen)  # assumes osc_listener called osc_listen when it gets something
#     def websocket_app(environ, start_response):
#         logging.info('Glass connected')
#         if environ["PATH_INFO"] == '/':
#             ws = environ["wsgi.websocket"]
#             WS.append(ws)
#             while True:
#                 self.ws.receive()
#             gevent.sleep()
#     wsgi_server = pywsgi.WSGIServer(("", websocket_port), websocket_app,
#                                     handler_class=WebSocketHandler)
#     wsgi_server.serve_forever()