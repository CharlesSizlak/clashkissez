#!/usr/bin/env python3
import os
import socket

def xxd(b: bytes) -> str:
    s = b.hex()
    return " ".join(s[i:i+2] for i in range(0, len(s), 2))
s = socket.socket()
s.connect(('127.0.0.1', 15873))

message = b'\x01\xc0\x03asd\x06asdfjk'

print("Sending Login message")
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
reply = s.recv(4096)
s.close()

print("Login Reply:", xxd(reply))
assert reply[0] == 46
assert reply[1] == 0x80
assert reply[2] == 2
