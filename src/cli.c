#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

#include <stdio.h>
#include <string.h>

#include <usart2.h>
#include <cli.h>


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

void cliTask(void * parameters)
{
	(void) parameters;
	uint8_t input;
	uint32_t input_idx;

	FreeRTOS_CLIRegisterCommand(&getTime_cmd);
	FreeRTOS_CLIRegisterCommand(&getRunTimeStats_cmd);

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
