/*
        The MIT License (MIT)

        Copyright (c) 2021 Andrew O'Connell

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.

        Dependancies :

        apt-get install libmodbus5 libmodbus-dev

        Compile as below or use 'make all' to allow program to use external system (.so) libraries
        gcc rtu-ao4.c -o rtu-ao4 -lmodbus 


*/

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// External Libs
#include <modbus/modbus.h> //LGPL 2.1

// Types & Variables
#include "Types.h"

// RTU Module Definitions
#include "ao.c"

// Functions
#include "config.c"
#include "modbus.c"
#include "rtudecode.c"

int main(int argc, char *argv[])
{

	int deviceId = 1;			 // there's only one device in the demo setup
	int displayType = HUMANREAD; // default
	int opt;
	int debugMode = 0;
	int configWrite = 0;
	int chanModeSetting[9] = {0};
	int modbusBaudSetting = 0;
	int targetChan = 0;
	int chanmASetting = 0;

	// Load Config, this is
	readConfig();

	// Command line options
	//
	// For reference see : https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
	//
	// The colon after the letter tells getopt to expect an argument after the option
	// To disable the automatic error printing, put a colon as the first character
	while ((opt = getopt(argc, argv, ":hjcda:b:p:o:s:m:w")) != -1)
	{
		switch (opt)
		{
		case 'h': // Human Readable output
			displayType = HUMANREAD;
			break;
		case 'j': // JSON output
			displayType = JSONREAD;
			break;
		case 'c': // comma seperated output
			displayType = CPUREAD;
			break;
		case 'd': // Print Config
			debugMode = 1;
			break;
		case 'a': // Set modbus address for read
			if (atoi(optarg) > 0 && atoi(optarg) < 255)
			{
				dataSource[deviceId].modbusId = atoi(optarg);
			}
			break;
		case 'b': // Set baudrate for read
			if (atoi(optarg) == 9600 || atoi(optarg) == 14400 || atoi(optarg) == 19200 || atoi(optarg) == 38400 || atoi(optarg) == 57600)
			{
				dataSource[deviceId].baudRate = atoi(optarg);
			}
			break;
		case 'p': // Set serial interface for read
			strcpy(dataSource[deviceId].interface, optarg);
			break;
		case 'w': // Invoke write to RTU NVRAM
			displayType = HUMANREAD;
			configWrite = 1;
			break;
		case 'o':
			if (atoi(optarg) < 5 && atoi(optarg) > 0)
			{
				targetChan = atoi(optarg);
			}
			break;
		case 's': //mA setting for channell   1080 = 10.80mA. Range 0-2040
			if (atoi(optarg) < 2041)
			{
				chanmASetting = atoi(optarg);
			}
			break;
		case 'm': // Set value for RTU Baud Rate register  (use in conjunction with -w flag)
			if (atoi(optarg) < 6 && atoi(optarg) > 0)
			{
				modbusBaudSetting = atoi(optarg);
			}
			break;
		case '?':
			printf("Synapse RTU-AO4 Reader - v1.0\n\n");
			printf("%s [-h|j|c] [-a] [-b] [-p] [-o] [-s] [-m] [-w] [-d]\n\n", argv[0]);
			printf("Syntax :\n\n");
			printf("-h = Human readable output (default)\n");
			printf("-j = JSON readable output\n");
			printf("-c = Comma delimited minimal output\n");
			printf("\n");
			printf("-a = Set Modbus device address (1-255)  default=1 \n");
			printf("-b = Set serial baud rate (9600/14400/19200/38400/57600)  default=19200 \n");
			printf("-p = Set serial interface to use e.g. /dev/ttyS0  default=/dev/ttyUSB0 \n");
			printf("\n");
			printf("-o = Output channel to set (1|2|3|4) \n");
			printf("-s = mA setting for channel e.g. 1080 - 10.80mA\n");
			printf("\n");
			printf("-m = Set value for RTU Baud Rate register (1=9600/2=14400/3=19200/4=38400/5=57600)  \n");
			printf("\n");
			printf("-w = Confirm writing configured setting registers to RTU NVRAM\n");
			printf("\n");
			printf("-d = debug mode\n");
			printf("\n");
			printf("Examples :\n");
			printf("Set output channel 3 to 10.80mA            :  %s -a 1 -b 19200 -p /dev/ttyS0 -o 3 -s 1080 \n", argv[0]);
			printf("Set output channel 3 to off                :  %s -a 1 -b 19200 -p /dev/ttyS0 -o 3 -s 0 \n", argv[0]);
			printf("\n");
			printf("Reconfigure RTU baud rate to 38400         :  %s -a 3 -p /dev/ttyS0 -m 4 -w\n", argv[0]);
			printf("\n");
			exit(1);
			break;
		}
	}

	if (displayType == HUMANREAD)
	{
		printf("\nSynapse RTU-AO8 Reader - v1.0\n\n");
	}

	// Write
	if (configWrite == 1)
	{
		reconfigureRTU(deviceId, modbusBaudSetting);
		exit(0);
	}

	if (debugMode == 1)
	{
		printConfig();
	}

	// Set ouput channel according to command line
	if (targetChan > 0)
	{
		setModbusValues(targetChan, chanmASetting); // only 1 device configured in demo tool
	}

	// Read in Modbus registers
	if (displayType == HUMANREAD)
	{
		printf("Modbus Reads...\n");
	}
	if (getModbusValues() == 0)
	{
		if (displayType == HUMANREAD)
		{
			printf("Modbus reads OK\n\n");
		}
	}
	else
	{
		printf("..Fatal Error : Error Reading Modbus device(s) \n\n");
		exit(1);
	}

	// Print output in desired format
	for (deviceId = 1; deviceId < (config.dsTotal + 1); deviceId++)
	{
		if (displayType == JSONREAD)
		{
			if (deviceId == 1)
				printf("{\n   \"device 1\" : {\n");
			else
				printf("   \"device %i\" : {\n", deviceId);
		}

		displayAoValues(deviceId, displayType);

		if (displayType == JSONREAD)
		{
			if (deviceId < (config.dsTotal))
				printf("   },\n");
			else
				printf("   }\n}\n", deviceId);
		}
	}
}