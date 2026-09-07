#pragma once

#include <stdint.h>
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

// void fmofdm_init(void);
// void fmofdm_transmit(uint8_t * data, uint32_t length);
// void fmofdm_create_symbol(uint8_t * data, q15_t * output);
void fmofdm_send_data(uint8_t * data, uint16_t length);
void fmofdmTask(void * parameters);