#!/usr/bin/env python3
import os
import socket
from generated.chess import *

def xxd(b: bytes) -> str:
    s = b.hex()
    return " ".join(s[i:i+2] for i in range(0, len(s), 2))

s = socket.socket()
s.connect(('127.0.0.1', 15873))

# Login
message = b'\x01\xc0\x03Jim\x06potato'

print("Sending login message")
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
print("Receiving:")
reply = s.recv(4096)
print(xxd(reply))

# Send a game invite


message = GameInviteRequest(
    session_token=reply[2:], 
    username="Bob",
    time_control_sender=60,
    time_increment_sender=1,
    time_control_receiver=120,
    time_increment_receiver=5,
    color=Color.RANDOM
).serialize()

print("Sending game invite")
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
print("Receiving:")
reply = s.recv(4096)
print(xxd(reply))

s.close()
