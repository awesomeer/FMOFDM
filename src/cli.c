#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <usart2.h>
#include <cli.h>
#include <dac.h>
#include <fmofdm.h>


#define CONSOLE_GREETING "FreeRTOS$ "

#define BACKSPACE "\b"
#define RIGHT_ARROW "\E[C"

uint8_t console_input[32];

/*
 * Get Time Command
 * Returns FreeRTOS Tick time
 */

static BaseType_t getTime_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t getTime_cmd =
{
	.pcCommand = "time",
	.pcHelpString = "time: Print FreeRTOS Tick Time\r\n",
	.pxCommandInterpreter = getTime_func,
	.cExpectedNumberOfParameters = 0
};

static BaseType_t getTime_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	TickType_t ticks = xTaskGetTickCount();
	snprintf(pcWriteBuffer, xWriteBufferLen, "Time Passed: %d ticks\r\n", ticks);
	return pdFALSE;
}

/*
 * Get Run Time Stats Command
 * Returns FreeRTOS Run Time Stats
 */

static BaseType_t getRunTimeStats_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t getRunTimeStats_cmd =
{
	.pcCommand = "runTimeStats",
	.pcHelpString = "runTimeStats: Print FreeRTOS Run Time Stats\r\n",
	.pxCommandInterpreter = getRunTimeStats_func,
	.cExpectedNumberOfParameters = 0
};

static BaseType_t getRunTimeStats_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	vTaskGetRunTimeStats(pcWriteBuffer);
	return pdFALSE;
}

/*
 * Generate Frequency Command
 * Generate sine wave with specified frequency
 */

static BaseType_t genFreq_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t genFreq_cmd =
{
	.pcCommand = "genFreq",
	.pcHelpString = "genFreq: Generate sine wave with specified frequency\r\n",
	.pxCommandInterpreter = genFreq_func,
	.cExpectedNumberOfParameters = 1
};

static int16_t cosine[100];
static void generateSineWave(uint32_t freq, int16_t *buffer, size_t length)
{

	float y[3] = {0, 0, 0};
	float x[2] = {0.99, 0};

	// y[n] = x[n] - y[n-2] + 2*cos(w0)*y[n-1] - cos(w0)*x[n-1]
	// y[n] = x[n] - y[n-2] + cos(w0)*(2*y[n-1] - x[n-1])
	float cw0 = cosf(2*M_PI*((float)freq)/100.0f); // Assuming a sample rate of 100 Hz
	for (size_t i = 0; i < length; i++)
	{ 
		y[0] = x[0] - y[2] + cw0 * (2 * y[1] - x[1]);
		buffer[i] = y[0] * (1 << 15);
		buffer[i] += 0x8000;

		x[1] = x[0];
		x[0] = 0;

		y[2] = y[1];
		y[1] = y[0];
	}
}

static BaseType_t genFreq_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterStringLength;
	const char *pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	uint32_t freq = (uint32_t)atoi(pcParameter);

	generateSineWave(freq, cosine, 100);
	dac_transmit(cosine, 100);

	snprintf(pcWriteBuffer, xWriteBufferLen, "Cosine wave generated with frequency: %d Hz\r\n", freq);
	return pdFALSE;
}

/*
 * Create OFDM Symbol Command
 * Create an OFDM symbol based on the input data (2 bytes)
 */

static BaseType_t genOFDM_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t genOFDMSymbol_cmd =
{
	.pcCommand = "genOFDM",
	.pcHelpString = "genOFDM: Create an OFDM symbol based on the input data (2 bytes)\r\n",
	.pxCommandInterpreter = genOFDM_func,
	.cExpectedNumberOfParameters = 1
};

static char fmofdm_recv_string[64];
static BaseType_t genOFDM_func(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	BaseType_t xParameterStringLength;
	// Get Symbol or Burst parameter
	const char *pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);

	// Send the data through FMOFDM and wait for response, timeout after 10 ticks
	fmofdm_send_data((uint8_t *)pcParameter, xParameterStringLength);
	uint8_t recvLen = fmofdm_recv_data(fmofdm_recv_string, 64, 10);
	if(recvLen != 0)
	{
		fmofdm_recv_string[recvLen] = '\0';
	}
	else
	{
		strcpy(fmofdm_recv_string, "Error Occured!!!");
	}

	snprintf(pcWriteBuffer, xWriteBufferLen, "Sent the string: %s\r\nRecv the string: %s\r\n", pcParameter, fmofdm_recv_string);
	return pdFALSE;
}

void cliTask(void * parameters)
{
	(void) parameters;
	uint8_t input;
	uint32_t input_idx;

	FreeRTOS_CLIRegisterCommand(&getTime_cmd);
	FreeRTOS_CLIRegisterCommand(&getRunTimeStats_cmd);
	FreeRTOS_CLIRegisterCommand(&genFreq_cmd);
	FreeRTOS_CLIRegisterCommand(&genOFDMSymbol_cmd);

	while(pdTRUE)
	{
		usart2_write(CONSOLE_GREETING, strlen(CONSOLE_GREETING));

		input_idx = 0;
		do
		{
			usart2_read(&input, sizeof(input), portMAX_DELAY);
			if(input == '\b')
			{
				if(input_idx > 0)
				{
					input_idx--;
					usart2_write(" \b", strlen(" \b"));
				}
				else
				{
					usart2_write(RIGHT_ARROW, strlen(RIGHT_ARROW));
				}
				continue;
			}
			console_input[input_idx++] = input;
		} while(input != '\r' && input_idx < sizeof(console_input));
		console_input[input_idx-1] = 0;

		usart2_write("\r\n", strlen("\r\n"));
		while(FreeRTOS_CLIProcessCommand(console_input, FreeRTOS_CLIGetOutputBuffer(), configCOMMAND_INT_MAX_OUTPUT_SIZE))
		{
			usart2_write((uint8_t *)FreeRTOS_CLIGetOutputBuffer(), strlen(FreeRTOS_CLIGetOutputBuffer()));
		}
		usart2_write((uint8_t *)FreeRTOS_CLIGetOutputBuffer(), strlen(FreeRTOS_CLIGetOutputBuffer()));
	}
}
