#pragma once

#include <arm_math.h>

void adc_init();

void adc_start();
void adc_readData(q15_t * buffer, uint32_t length);
void adc_flush(void);
//void adc_readData(int16_t * buffer, int32_t length);