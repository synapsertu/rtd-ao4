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

void displayAoValues(int deviceId, int displayType)
{
	// initialise these to zero or else we'll get nonsense readings
	// shown if the chan/register is not enabled in the config.
	// You could initialise this to NaN or some other recognisable value to indicate "No reading available"
	float chanLve[5] = {0.0};

	int regId;

	for (regId = 1; regId < (dataSource[deviceId].numRegisters + 1); regId++)
	{

		if (dataSource[deviceId].regAddress[regId] == 1)
		{
			chanLve[1] = dataSource[deviceId].value[regId];
		}

		if (dataSource[deviceId].regAddress[regId] == 2)
		{
			chanLve[2] = dataSource[deviceId].value[regId];
		}

		if (dataSource[deviceId].regAddress[regId] == 3)
		{
			chanLve[3] = dataSource[deviceId].value[regId];
		}

		if (dataSource[deviceId].regAddress[regId] == 4)
		{
			chanLve[4] = dataSource[deviceId].value[regId];
		}
	}

	// present the output in the format desired by the command line option
	if (displayType == HUMANREAD)
	{

		printf("_____| AO4 Modbus Address %i |______________________________________________________________________\n", dataSource[deviceId].modbusId);
		printf("Ch\t\t\tOutput setting mA\n");
		printf("1\t\t\t%2.2f\n", (chanLve[1] / 100)); // divide current register value by 100 to get setting in mA
		printf("2\t\t\t%2.2f\n", (chanLve[2] / 100));
		printf("3\t\t\t%2.2f\n", (chanLve[3] / 100));
		printf("4\t\t\t%2.2f\n", (chanLve[4] / 100));
		printf("\n\n");
	}

	if (displayType == JSONREAD)
	{
		printf("      \"name\":\"RTU-AO4\",\n");
		printf("      \"type\": %i,\n", dataSource[deviceId].deviceType);
		printf("      \"deviceId\":\"%i\",\n", deviceId);
		printf("      \"modbusId\":\"%i\",\n", dataSource[deviceId].modbusId);
		printf("      \"channels\": 4,\n");
		printf("      \"channel 1\": {\n");
		printf("         \"live\": %2.2f\n", (chanLve[1] / 100));
		printf("      },\n");
		printf("      \"channel 2\": {\n");
		printf("         \"live\": %2.2f\n", (chanLve[2] / 100));
		printf("      },\n");
		printf("      \"channel 3\": {\n");
		printf("         \"live\": %2.2f\n", (chanLve[3] / 100));
		printf("      },\n");
		printf("      \"channel 4\": {\n");
		printf("         \"live\": %2.2f\n", (chanLve[4] / 100));
		printf("      }\n");
	}

	if (displayType == CPUREAD)
	{

		//deviceId,deviceType,modbusId,chanLve[1],chanLve[2],chanLve[3],chanLve[4]
		printf("%i,%i,%i,4,%2.2f,%2.2f,%2.2f,%2.2f;\n",
			   deviceId, dataSource[deviceId].deviceType, dataSource[deviceId].modbusId,
			   (chanLve[1] / 100), (chanLve[2] / 100), (chanLve[3] / 100), (chanLve[4] / 100));
	}
}

// Uses modbus_write_registers (FC16) to write single config registers
// this is custom for each rtu module so let's keep it in the module specific include even though it's modbus related
int reconfigureRTU(int deviceId, int modbusBaudSetting)
{

	int rc;
	int regId;

	const char *adcBaud[6];
	adcBaud[0] = "19200";
	adcBaud[1] = "9600";
	adcBaud[2] = "14400";
	adcBaud[3] = "19200";
	adcBaud[4] = "38400";
	adcBaud[5] = "57600";

	uint16_t tableRegisters[1]; // 1 element array for use with modbus write command

	// modbus device handle
	modbus_t *mb;

	// Defines storage for returned registers from modbus read, *must* equal or exceed maximum number of registers requested, ask me how I know...
	uint16_t mbdata_UI16[30];

	printf("Processing Config Changes...\n");

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

	// check we can connect (not sure if this is relevant on serial modbus)
	if (modbus_connect(mb) == -1)
	{
		printf("Connect Failed to Modbus ID [%i] on [%s]\n", dataSource[deviceId].modbusId,
			   dataSource[deviceId].interface);
		modbus_close(mb);
		modbus_free(mb);
		return -1;
	}

	if (modbusBaudSetting > 0)
	{
		printf("Changing RTU Baud Rate to %s...\n", adcBaud[modbusBaudSetting]);
		tableRegisters[0] = modbusBaudSetting;
		rc = modbus_write_registers(mb, 14, 1, tableRegisters);
		if (rc == -1)
		{
			printf("Modbus request Fail : Device Address [%i] Start Address [14] For [1] Registers \n", deviceId);
			modbus_close(mb);
			modbus_free(mb);
			exit(1);
		}
	}

	printf("Writing Config Register...\n");
	tableRegisters[0] = 255;
	rc = modbus_write_registers(mb, 15, 1, tableRegisters);

	if (rc == -1)
	{
		printf("Modbus request Fail : Device Address [%i] Start Address [15] For [1] Registers \n", deviceId);
		modbus_close(mb);
		modbus_free(mb);
		exit(1);
	}

	exit(0);
}

// This is for RTU modules which have output capability only
// this is custom for each rtu module so let's keep it in the module specific include even though it's modbus related
int setModbusValues(int targetChan, int chanmASetting)
{
	int rc;
	int regId;
	int deviceId = 1; // There's only one device in the demo code

	// modbus device handle
	modbus_t *mb;

	// Defines storage for returned registers from modbus read, *must* equal or exceed maximum number of registers requested, ask me how I know...
	uint16_t mbdata_UI16[30];

	// 1 element array for use with modbus write command
	uint16_t tableRegisters[1];

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

	// check we can connect (not sure if this is relevant on serial modbus)
	if (modbus_connect(mb) == -1)
	{
		printf("Connect Failed to Modbus ID [%i] on [%s]\n", dataSource[deviceId].modbusId,
			   dataSource[deviceId].interface);
		modbus_close(mb);
		modbus_free(mb);
		return -1;
	}

	if (targetChan > 0)
	{
		tableRegisters[0] = chanmASetting;
		rc = modbus_write_registers(mb, dataSource[deviceId].regAddress[(targetChan - 1)], 1, tableRegisters);
		if (rc == -1)
		{
			printf("Modbus request Fail : Device Address [%i] Start Address [1] For [1] Registers \n", deviceId), (0 + (targetChan - 1));
			modbus_close(mb);
			modbus_free(mb);
			exit(1);
		}
	}

	modbus_close(mb);
	modbus_free(mb);

	return 0;
}

int getChanConfig(modbus_t *mb, int deviceId)
{

	// Nothing to do
	return 0;
}