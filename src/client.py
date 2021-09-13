#!/usr/bin/env python3.9

from numpy.core.fromnumeric import size
from pychess.ErrorReply import ErrorReply
from pychess.RegisterRequest import *
import socket

def recv_packet(sock):
    # Read the first 4 bytes to figure out the size of this message
    buffer = b""
    bytes_received = 0
    while bytes_received < 4:
        buffer_piece = sock.recv(4 - bytes_received)
        buffer += buffer_piece
        bytes_received += len(buffer_piece)
        if len(buffer_piece) == 0:
            raise ConnectionError("Connection closed")
    
    bytes_to_read = int.from_bytes(buffer, "big", signed=False)

    # Sit in a while loop reading bytes bytes until we're finished
    buffer = b""
    bytes_received = 0
    while bytes_received < bytes_to_read:
        buffer_piece = sock.recv(bytes_to_read - bytes_received)
        buffer += buffer_piece
        bytes_received += len(buffer_piece)
        if len(buffer_piece) == 0:
            raise ConnectionError("Connection closed")

    # return to the nice function caller person what we got
    return buffer

def main():
    socky = socket.socket()
    socky.connect(("127.0.0.1", 15873))

    # Make a builder
    builder = flatbuffers.Builder(1024)
    username = builder.CreateString("Bob")
    password = builder.CreateByteVector(b"Password")
    # Make a Login Request
    RegisterRequestStart(builder)
    RegisterRequestAddUsername(builder, username)
    RegisterRequestAddPassword(builder, password)
    request = RegisterRequestEnd(builder)

    # Convert the Login Request to a buffer
    builder.Finish(request)
    packet = builder.Output()

    # Send the buffer
    socky.sendall(len(packet).to_bytes(4, "big", signed=False))
    socky.sendall(packet)

    # Recieve the expected error message
    buffer = recv_packet(socky)
    x = ErrorReply.GetRootAsErrorReply(buffer, 0)
    print(x)

    socky.shutdown(socket.SHUT_RDWR)
    socky.close()

    ErrorReply.GetRootAsErrorReply()


if __name__ == '__main__':
    main()
