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
void decodeModbusReg(int deviceId, int wStartRegId, int requestedRegisters, uint16_t mbdata_UI16[]);

int getModbusValues()
{
	int wRegId;
	int wStartRegId;
	int wStartReg;
	int wEndReg;
	int requestedRegisters;
	int requestFull = 0;
	int rc;
	int deviceId;
	int regId;

	// modbus device handle
	modbus_t *mb;

	// Defines storage for returned registers from modbus read, *must* equal or exceed maximum number of registers requested
	uint16_t mbdata_UI16[30];

	// Run through each device
	for (deviceId = 1; deviceId < (config.dsTotal + 1); deviceId++)
	{
		//printf("\nMODBUS : START - Connecting to device [%i] \n", deviceId);
		mb = modbus_new_rtu(dataSource[deviceId].interface,
							dataSource[deviceId].baudRate,
							dataSource[deviceId].parity[0],
							dataSource[deviceId].dataBits,
							dataSource[deviceId].stopBit);

		modbus_set_slave(mb, dataSource[deviceId].modbusId);

		// Set per-byte and total timeouts, this format has changed from the older libmodbus version.
		// This could be useful if we've a latent RF-Link
		// TODO : Don't hard code this, allow it to be configurable
		modbus_set_response_timeout(mb, 5, (5 * 1000000));
		modbus_set_byte_timeout(mb, 5, (5 * 1000000));

		// Enable/Disable Modbus debug
		modbus_set_debug(mb, FALSE);

   		// clear the serial port before first use
		modbus_flush(mb);

		// check we can connect (not sure if this is relevant on serial modbus)
		if (modbus_connect(mb) == -1)
		{
			printf("Connect Failed to Modbus ID [%i] on [%s]\n", dataSource[deviceId].modbusId,
			dataSource[deviceId].interface);
			modbus_flush(mb);
			modbus_close(mb);
			modbus_free(mb);
			exit(1);
		}

		// Get RTU Specific channel config by making a read from the RTU module ahead of the main request
		// Defined in ao.c
		getChanConfig(mb, deviceId);

		/*   Optimise modbus register reads, 

			 e.g. here's our map
			
			 40001		
			 40002		*
			 40003
			 40004		*
			 40005
			 40006		*
			 40007
			 40008
			 40009		*
			 40010
			 
			 Even if we only want 4 registers * it's quicker to get registers 2-9 in one pass then to make 4 
			 seperate requests due to the comms overhead, however this means our request section gets more
		     complex as a result.
			 
			 The reuqest building needs to accomodate 16bit and 32bit registers (2 x 16bit registers) and
			 non-sequential register addresses.

			 The output buffer of the RTU unit is 64bytes (5bytes overhead  => 29 x 16bit registers abs max)
			 
			 Here we create a window that's at most 26 x 16bit registers long to stay well within this limit	 			 

			 imagine staking out a requst "window" over a map using two pegs to mark start and end register positions, 
			 with all requested registers inside

			 
			 For more info on modbus request formats see : https://simplymodbus.ca/FC03.htm	 
			 								 
		*/

		// Start with both start & end window register pegs at same place, these
		// define the register map window region we're getting in one transaction
		wStartReg = dataSource[deviceId].regAddress[1];
		wEndReg = dataSource[deviceId].regAddress[1];

		// This keeps track of the starting window register Id.
		// We will also make use this during decode so we can keep track of what the modbus
		// reply register array contains.
		wStartRegId = 1;

		// This is used to point to the next register to process in the dataSource[deviceId].regAddress[x] array
		// We increment this one position at a time as sucessfully increase our modbus address request range
		wRegId = 1;

		// Once we have a full (<=26 registers) modbus request range set this flag
		// so we can request them in one transaction.
		requestFull = 0;

		// Move the end window register peg out by one register at time and check both
		// window length and whether we're out of registers to process
		// Note that the registers addresses may not be an adjacent
		while (wRegId <= (dataSource[deviceId].numRegisters))
		{

			// Are we looking at a 32bit double register type?
			if (dataSource[deviceId].regType[wRegId] > 2)
			{
				// Check if we can fit 2 more registers into our request range
				if (((dataSource[deviceId].regAddress[wRegId] + 1) - wStartReg) < 26)
				{
					// Great, there's room so we can include it in this run
					wEndReg = (dataSource[deviceId].regAddress[wRegId] + 1);

					// We can now move to the next register in the list
					wRegId++;
				}
				else
				{
					// Drat, no room left, oh well flag as ready and we'll pick it up at the start of the next run
					// when we reset and move the window.
					requestFull = 1;
				}
			}
			else // Must be a 16bit single register type then
			{
				// Check if we can fit 1 more register into our request range
				if ((dataSource[deviceId].regAddress[wRegId] - wStartReg) < 26)
				{
					// Great, there's room so we can include it in this run
					wEndReg = dataSource[deviceId].regAddress[wRegId];

					// We can now move to the next register in the list
					wRegId++;
				}
				else
				{
					// Drat, no room left, oh well flag as ready and we'll pick it up at the start of the next run
					// when we reset and move the window.
					requestFull = 1;
				}
			}

			// Make modbus request only if we've either a full request OR we've sucesfully added in last register in the list
			if (requestFull == 1 || wRegId > dataSource[deviceId].numRegisters)
			{
				/*  This is confusing 

					Modbus addesses (for the most part) start at 1 not 0, so in a request address 1 is the 0th register
				    
					e.g.  if we want register values 1-9
				
				       wEndReg-wStartReg = 9-1  = 8 Registers
				
				       1 for 8 registers gets values for registers 2,3,4,5,6,7,8,9	
				       0 for 8 registers gets values for registers 1,2,3,4,5,6,7,8
				       0 for 9 registers gets values for registers 1,2,3,4,5,6,7,8,9
				  
				 	So in our case we ask for the register range defined by From : (wStartReg-1) for (requestedRegisters+1) number of registers
				*/

				requestedRegisters = (wEndReg - wStartReg);

				rc = modbus_read_registers(mb, (wStartReg - 1), (requestedRegisters + 1), mbdata_UI16);

				if (rc == -1)
				{
					printf("Modbus request Fail : Device Address [%i] Start Address [%i] For [%i] Registers \n", dataSource[deviceId].modbusId, (wStartReg - 1), (requestedRegisters + 1));
					modbus_close(mb);
					modbus_free(mb);
					exit(1);
				}

				decodeModbusReg(deviceId, wStartRegId, requestedRegisters, mbdata_UI16);

				// Move start and end window register pegs to the next register id in the queue
				// so we don't re-process registers.
				wStartReg = dataSource[deviceId].regAddress[wRegId];
				wStartRegId = wRegId;
				wEndReg = dataSource[deviceId].regAddress[wRegId];

				//not forgetting to reset the requestFull flag so we don't get a stuck state
				requestFull = 0;
			}
		}
	}

	modbus_flush(mb);
	modbus_close(mb);
	modbus_free(mb);

	return 0;
}

void decodeModbusReg(int deviceId, int windowRegId, int requestedRegisters, uint16_t mbdata_UI16[])
{

	// temp variables for modbus register decoding
	unsigned int temp1;
	int temp2;
	int mbShift;
	int mbTotal;

	// These reference the addresses and Ids of the registers we've received back from the device
	// intalise them to the start address of the requested registers
	int mbReplyId = 0; // mbdata_UI16 reply array starts at 0
	int mbReplyRegAddress = dataSource[deviceId].regAddress[windowRegId];

	// Cycle through the reply registers until we exceed the requested total number of registers requested
	// mbdata_UI16 starts at 0
	while (mbReplyId <= requestedRegisters)
	{

		// Check if the current address in the modbus reply list is one we're interested in
		if (mbReplyRegAddress == dataSource[deviceId].regAddress[windowRegId])
		{

			// We have a match! Decode it depending on the register type
			switch (dataSource[deviceId].regType[windowRegId])
			{
			default:
			case REG_UI16: // [1] - 16bit unsigned Integer
				dataSource[deviceId].value[windowRegId] = mbdata_UI16[mbReplyId];
				break;
			case REG_SI16: // [2] - 16bit Signed Integer
				temp1 = mbdata_UI16[mbReplyId];
				temp2 = (int16_t)temp1;
				dataSource[deviceId].value[windowRegId] = temp2;
				break;
			case REG_UI32B: // [3] - Big Endian 32bit Unsigned Integer
				temp1 = (mbdata_UI16[mbReplyId + 1] + (mbdata_UI16[mbReplyId] * 65536));
				dataSource[deviceId].value[windowRegId] = temp1;
				break;
			case REG_UI32L: // [4] - Little Endian 32bit Unsigned Integer
				temp1 = (mbdata_UI16[mbReplyId] + (mbdata_UI16[mbReplyId + 1] * 65536));
				dataSource[deviceId].value[windowRegId] = temp1;
				break;
			case REG_SI32B: // [5] - Big Endian 32bit Signed Integer
				dataSource[deviceId].value[windowRegId] = (mbdata_UI16[mbReplyId + 1] + (mbdata_UI16[mbReplyId] * 65536));
				break;
			case REG_SI32L: // [6] - Little Endian 32bit Signed Integer
				dataSource[deviceId].value[windowRegId] = (mbdata_UI16[mbReplyId] + (mbdata_UI16[mbReplyId + 1] * 65536));
				break;
			case REG_F32B: // [7] - Big Endian 32bit 754 Float
				mbShift = mbdata_UI16[mbReplyId + 1] << 16;
				mbTotal = mbShift + mbdata_UI16[mbReplyId];
				dataSource[deviceId].value[windowRegId] = *((float *)&mbTotal);
				break;
			case REG_F32L: // [8] - Little Endian 32bit 754 Float
				mbShift = mbdata_UI16[mbReplyId] << 16;
				mbTotal = mbShift + mbdata_UI16[mbReplyId + 1];
				dataSource[deviceId].value[windowRegId] = *((float *)&mbTotal);
				break;
			}
			// Now we've matched & decoded we can move on to next target register
			windowRegId++;
		}

		// Match or no-match, move on to next address in the modbus reply list
		mbReplyId++;
		mbReplyRegAddress++;
	}
}
