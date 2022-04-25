=== API ===

Ports 0-3 (in): set gear lights
Arguments:
    * BYTE {0, 1} --> 1 - light on, 0 - light ff

Port 4 (in, out): gear switch
Arguments (out):
    * BYTE {0, 1} --> 1 - gear on, 0 - gear off
Arguments (in)
 n/a

Port 5 (in, out): key switch
Arguments (out):
    * BYTE [0, 4] --> key position 0..4
Arguments (in)
 n/a

