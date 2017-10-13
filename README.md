# P2P485
Peer to peer communications for RS485 networks

As a by-product of another project (that I'll post when done) I've scrounged together this comm net out of need. In essence it's pretty simple. Byte-wide addressing allows, in theory, up to 255 connected nodes. Reality, less.

Some things that may make it a bit different are:
  - all messages are packets, limited by the size of your buffer
  - packets start with STX, destination address, source address, message body, ETX then crc.
  - you can switch crc on/off as a transmitter
  - receiver will automatically detect/check crc if there is one or not otherwise
  - ACK/NAK returned to sender (can also be switched off)
  - General Call to address 0 will be picked up by all, acknowledged by none.
  - source address of last received packet retatained and automatically used as destination address for a response if the packet message begins with a "/".
  
In the bigger picture, my project has interrupt conflicts with SoftwareSerial (bogarting all the PCINTs) so I hacked a (three) version of it to only work on one port. This code deals with pins on PORTC so I've included SSerialC which is simply the hacked version. If you have no conflicts, you can use any serial port you want as long as it's referenced as ntwk or change the name.

The other is called functions.h and exists only because they work and were in the way so I dumped them out to a separate file.

This will continue to be work-in-progress as it is targetted as an open source front-end (the bigger picture) to a commercial bit of hardware, so stay tuned.
