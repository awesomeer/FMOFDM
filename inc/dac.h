#pragma once


#include <stdint.h>

/*
 * Initialize the DAC
 */
void dac_init(void);

/*
 * Set the DAC output value
 * @param value The value to set the DAC output to (0-4095)
 * Will Disable the DMA and reset DMA Counting Semaphore
 */
void dac_set(uint16_t value);


/*
 * Transmit a buffer of data to the DAC using DMA
 * @param buffer The buffer of data to transmit
 * @param length The length of the buffer
 * Will Enable the DMA and start the DMA transfer.
 */
void dac_transmit(uint16_t *buffer, uint32_t length);

