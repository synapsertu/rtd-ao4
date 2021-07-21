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
*/

#define REG_UI16   1
#define REG_SI16   2
#define REG_UI32B  3
#define REG_UI32L  4
#define REG_SI32B  5
#define REG_SI32L  6
#define REG_F32B   7
#define REG_F32L   8

#define RTU_AI8   1
#define RTU_DI8   2
#define RTU_RTD4  3
#define RTU_AO4   4
#define RTU_DO8   5

#define HUMANREAD 1
#define CPUREAD   2
#define JSONREAD  3

#define MAX_DATASOURCES 20 		        // Devices
#define MAX_DATAPOINTS  150		        // Max registers per device 

// TODO : Not sure why we've left this as a struct, probably historcial, let's convert it back
typedef struct 
{
	int dsTotal; 				// Total number of devices
}CONFIG;	
		
typedef struct 
{
	int deviceType;		 		//  1 = RTU-AI8  2=RTU-DI8  3=RTU-RTU4   4=RTU-AO8 
	int modbusId;        			//  Modbus address of device
	int dataBits;        			//  8 or 7
	int baudRate;        			//  9600/19200/38400/57600
	int stopBit;         			//  1 or 0
	int timeout;         			//  TODO : this needs configuring
	char parity[6];      			//  none, odd, even
	char interface[20]; 			//  /dev/ttyS0 /dev/ttyAMA0 /dev/ttyUSB100
	int numRegisters;			//  Total number of registers enabled to scan for
	int ChanMode[9];			//  Per-channel mode, this gives the channel setting or mode e.g.ADC this is the resolution setting for Digin this the channel mode, index 1 = Chan 1
	int regAddress[MAX_DATAPOINTS];         //  addess minus the 40000 multiier, total  max registers per device              	
	int regType[MAX_DATAPOINTS];            //  register type e.g. 32bit integer or unsigned 16bit  
	float value[MAX_DATAPOINTS];	        //  decoded (and then scaled) value for the register			  
}DATASOURCE;		
		
	
CONFIG config;
DATASOURCE dataSource[MAX_DATASOURCES];  // Max devices/data sources



