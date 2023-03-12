=== API ===

receive:

port 0: request update (no data)
port 1: byte bus voltage
port 2: byte led (0 - off, 1 - on)

send:

port 0: byte selector mode
port 1: uint16_t transponder (0000 - 7777)
port 2: button pressed (no data)