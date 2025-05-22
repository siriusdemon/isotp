ip link set can0 down
ip link set can0 type can bitrate 500000 dbitrate 2000000 fd on loopback off
ip link set can0 txqueuelen 1000
ip link set can0 up