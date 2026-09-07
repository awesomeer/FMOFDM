#pragma once

#include <FreeRTOS.h>
#include <stm32l432xx.h>

void usart2_init( void );
void usart2_write(const uint8_t * c, uint32_t len );
uint32_t usart2_read(uint8_t *buffer, uint32_t len, TickType_t delay);