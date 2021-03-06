#!/usr/bin/env python3
import os
import socket
from generated.chess import *

def xxd(b: bytes) -> str:
    s = b.hex()
    return " ".join(s[i:i+2] for i in range(0, len(s), 2))
s = socket.socket()
s.connect(('127.0.0.1', 15873))

message = b'\x01\xc0\x03Jim\x06potato'

print("Sending Login message")
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
reply = s.recv(4096)
print("Login Reply:", xxd(reply))
deserialized = deserialize(reply)

if isinstance(deserialized, ErrorReply):
    print(deserialized.error)

s.close()

assert reply[0] == 25
assert reply[1] == 0x80
assert len(reply) == 18
