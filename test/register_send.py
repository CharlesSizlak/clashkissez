#!/usr/bin/env python3
import os
import socket

def xxd(b: bytes) -> str:
    s = b.hex()
    return " ".join(s[i:i+2] for i in range(0, len(s), 2))

s = socket.socket()
s.connect(('127.0.0.1', 15873))

message = b'\x02\xc0\x03Jim\x06potato'

print("Sending registration message")
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
print("Receiving:")
reply = s.recv(4096)
print(xxd(reply))
assert reply[0] == 26
assert reply[1] == 0x80
assert len(reply) == 18

message = b'\x02\xc0\x03Bob\x06tomato'
s.sendall(len(message).to_bytes(4, 'big'))
s.sendall(message)
print("Receiving:")
reply = s.recv(4096)
print(xxd(reply))
assert reply[0] == 26
assert reply[1] == 0x80
assert len(reply) == 18

s.close()
