#!/usr/bin/env python3
import os
import socket

s = socket.socket()
s.connect(('127.0.0.1', 15873))

s.recv(4096)

s.close()
