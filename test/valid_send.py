#!/usr/bin/env python3
import os
import socket

s = socket.socket()
s.connect(('127.0.0.1', 15873))

message = b'\x01\xc0\x03bob\x04asdf'

s.send(len(message).to_bytes(4, 'big'))
s.send(message)
s.recv(4096)

s.close()
