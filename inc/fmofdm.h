#pragma once

#include <stdint.h>

#include <FreeRTOS.h>
#include <arm_math.h>

#define NUM_BINS 16
#define NUM_FFT_LEN (NUM_BINS*2)

typedef struct {
    q15_t real;
    q15_t imag;
} cq15_t;

typedef union {
    cq15_t * bin;
    q15_t  * raw;
} cq15_bins_t;

/*
 * Get Data from the FMOFDM interface
 */
void fmofdm_send_data(uint8_t * data, uint16_t length);

/*
 * Send Data out FMOFDM interface
 */
uint32_t fmofdm_recv_data(uint8_t * data, uint16_t length, TickType_t delay);

/*
 * Task that contains a State Machine to receive and decode FMOFDM transmissions
 */
void fmofdmTask(void * parameters);