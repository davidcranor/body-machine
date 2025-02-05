"""
This program sends 10 random values between 0.0 and 1.0 to the /filter address,
waiting for 1 seconds between each value.
"""
import argparse
import random
import time

from pythonosc import osc_message_builder
from pythonosc import udp_client


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--ip", default="127.0.0.1",
      help="The ip of the OSC server")
  parser.add_argument("--port", type=int, default=8000,
      help="The port the OSC server is listening on")
  args = parser.parse_args()

  client = udp_client.UDPClient(args.ip, args.port)

  for x in range(10):
    msg = osc_message_builder.OscMessageBuilder(address = "/debug")
    msg.add_arg("scottgreenwald.jpg" if x % 2 == 1 else "brandynwhite.jpg")
    msg = msg.build()
    client.send(msg)
    time.sleep(1)

  for x in range(100):
    msg = osc_message_builder.OscMessageBuilder(address = "/debug")
    msg.add_arg("scottgreenwald.jpg" if x % 2 == 1 else "brandynwhite.jpg")
    msg = msg.build()
    client.send(msg)
    time.sleep(0.1)
  