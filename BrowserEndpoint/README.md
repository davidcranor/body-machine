# What is this thing

The overall setup is that when someone wear a touch bracelet
touches the receiver station, an osc event is sent out. The
`bcOscServer.py` listens for osc events, and is connected
to a browser client over a websocket. When an osc event
comes in, it forwards an appropriate message to the browser.

# How to use

Run `bConductor.py` with python. Navigate to localhost:8833
in a browser. Test OSC messages can be sent by testOscLock.py.

# What files matter

The python script `bConductor.py` is the only python script
that is used in the final application. Other files
accumulated during development but don't do anything now
[todo: delete from latest revision]. The web app is 
contained in `bcWebapp.html`. 

scottgwald, documented on 16.1.2014
