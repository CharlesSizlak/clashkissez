#!/usr/bin/env python3
import os
import socket

s = socket.socket()
s.connect(('127.0.0.1', 15873))

message = b"asdf"

s.send(len(message).to_bytes(4, 'big'))
s.send(message)
s.recv(4096)

s.close()
