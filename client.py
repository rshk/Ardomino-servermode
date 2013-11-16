#!/usr/bin/env python

# Ok, right now it's the server, but anyways..'

from __future__ import print_function

from collections import deque
from io import BytesIO
import os
import socket
import struct

# print(struct.unpack("<ffff", line))


class DataReader(object):
    def __init__(self):
        self._lines = []
        self._buf = BytesIO()

    def __iter__(self):
        return self

    def next(self):
        try:
            return self._lines.pop(0)
        except IndexError:
            raise StopIteration

    def feed(self, text):
        lines = text.splitlines()
        if len(lines) < 1:
            return  # nothing to do
        lines[0] = self._buf.getvalue() + lines[0]
        self._buf.seek(0)
        self._buf.truncate()
        if not text.endswith("\n"):
            self._buf.write(lines.pop())
        self._lines.extend(lines)


class Socket(object):
    def __init__(self, host, port):
        self.host = host
        self.port = port

    def __enter__(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        return self.socket

    def __exit__(self, exc_type, exc_value, traceback):
        self.socket.close()


HOST = os.environ.get('HOST', '')
PORT = int(os.environ.get('PORT', 43555))


with Socket(HOST, PORT) as s:
    while True:
        print("Listening on socket...")
        s.listen(1)
        conn, addr = s.accept()
        print("Connected: {0}:{1}".format(*addr))

        while True:
            ## discard all data until we find the "ARDOMINO" header
            dq = deque(maxlen=8)
            while ''.join(dq) != 'ARDOMINO':
                dq.append(conn.recv(1))

            ## Ok, now we can decode stuff
            raw = conn.recv(4 * 2)
            unpacked = struct.unpack('<ff', raw)
            print("Data: T={0} RH={1}".format(*unpacked))

            ## Send an "OK" message
            conn.send("ACK")
