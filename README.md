# Ardomino - server mode

Sketch for the UNO+WiFly-based Ardomino, acting as a server over TCP.


## Compiling

- Install the arduino IDE
- Install ino http://inotool.org/
- Copy ``src/settings.example.h`` -> ``src/settings.h``
- From the repo root, run ``ino build``
- Run ``ino upload`` to upload to the arduino


## Message format

Messages are sent on the wire as the ``ARDOMINO`` header (literal string)
followed by a variable number of little-endian packed floats (currently 2).

This thing obviously needs some improvements:

- A "variable number of packed floats" is just bad. We need
  some better way to define fields (add a "count" field at the beginning?).
- The standard is to use Big Endian. But the arduino uses little
  endian internally.


## Server mode

We need to make the ardomino act as a server..
