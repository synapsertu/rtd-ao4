# Synapse RTD-AO4 Tool

 Dependancies :

```# apt-get install sqlite3 libsqlite3-dev libmodbus5 libmodbus-dev```

Compile as below or use 'make all'

```gcc log.c -o log -lsqlite3 -lmodbus```

```
Synapse RTU-AO4 Reader - v1.0

./rtu-ao4 [-h|j|c] [-a] [-b] [-p] [-o] [-s] [-m] [-w] [-d]

Syntax :

-h = Human readable output (default)
-j = JSON readable output
-c = Comma delimited minimal output

-a = Set Modbus device address (1-255)  default=1
-b = Set serial baud rate (9600/14400/19200/38400/57600)  default=19200
-p = Set serial interface to use e.g. /dev/ttyS0  default=/dev/ttyUSB0

-o = Output channel to set (1|2|3|4)
-s = mA setting for channel e.g. 1080 - 10.80mA

-m = Set value for RTU Baud Rate register (1=9600/2=14400/3=19200/4=38400/5=57600)

-w = Confirm writing configured setting registers to RTU NVRAM

-d = debug mode

Examples :
Set output channel 3 to 10.80mA            :  ./rtu-ao4 -a 1 -b 19200 -p /dev/ttyS0 -o 3 -s 1080
Set output channel 3 to off                :  ./rtu-ao4 -a 1 -b 19200 -p /dev/ttyS0 -o 3 -s 0

Reconfigure RTU baud rate to 38400         :  ./rtu-ao4 -a 3 -p /dev/ttyS0 -m 4 -w

```
