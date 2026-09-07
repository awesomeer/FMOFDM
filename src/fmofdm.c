
#include <math.h>

#include <fmofdm.h>
#include <task.h>
#include <message_buffer.h>

#include <adc.h>
#include <dac.h>
#include <stm32l432xx.h>
#include "arm_math.h"

#define MAX_AMP 0x7000
#define AMP_SPACE 0x2000

// 5 preamble + 1 signal + 16 symbols
#define NUM_PREAMBLE 5
#define NUM_SIGNAL_SYMBOLS 1
#define NUM_DATA_SYMBOLS 16
#define BYTES_PER_SYMBOL 4
#define BITS_PER_POINT 4
#define NUM_SYMBOLS (NUM_PREAMBLE + NUM_SIGNAL_SYMBOLS + NUM_DATA_SYMBOLS)

#define PHASE_RES (360.0f / ((float)NUM_FFT_LEN))


static arm_rfft_instance_q15 recv_ifft_inst;
static arm_rfft_instance_q15 recv_fft_inst;

static q15_t ifft_input[(NUM_BINS+1)*2];
static q15_t fmofdm_burst[NUM_SYMBOLS][NUM_FFT_LEN];

static void convert_polar(q15_t * data, uint32_t length)
{
    for(int i = 0; i < length; i++)
    {
        data[i] += 0x8000;
    }
}

static void fmofdm_create_preamble(q15_t * preamble)
{
    memset(ifft_input, 0, sizeof(ifft_input));
    cq15_bins_t buffer = { .raw = ifft_input };

    buffer.bin[0].real = 0;
    buffer.bin[0].imag = 0;

    buffer.bin[NUM_BINS].real = 0;
    buffer.bin[NUM_BINS].imag = 0;

    // Create Preamble
    for(int i = 1; i < NUM_BINS; i++)
    {
        buffer.bin[i].real = MAX_AMP;
        buffer.bin[i].imag = MAX_AMP;
    }

    arm_rfft_q15(&recv_ifft_inst, buffer.raw, preamble);
}

// This function generates a FMOFDM symbol based on the input data
// QAM4 constellation is used for each bin
// 
// The layout of the symbol is as follows:
// 16 bins for each OFDM symbol
// Bin 0 - DC bin, set to 0
// Bin 1-4 - b0[1:0], b0[3:2], b0[5:4], b0[7:6]
// Bin 5 - Pilot bin
// Bin 6-9 - b1[1:0], b1[3:2], b1[5:4], b1[7:6]
// Bin 10 - Pilot bin
// Bin 11-15 - Unused, set to 0

static void fmofdm_create_symbol(uint8_t * data, q15_t * output)
{
    memset(ifft_input, 0, sizeof(ifft_input));
    cq15_bins_t buffer = { .raw = ifft_input };

    buffer.bin[0].real = 0;
    buffer.bin[0].imag = 0;

    buffer.bin[NUM_BINS].real = 0;
    buffer.bin[NUM_BINS].imag = 0;

    uint32_t buffer_idx = 1;
    for(int byte = 0; byte < BYTES_PER_SYMBOL; byte++)
    {
        // Pilot Signal
        buffer.bin[buffer_idx].real = MAX_AMP;
        buffer.bin[buffer_idx].imag = MAX_AMP;
        buffer_idx++;

        for(int q = 0; q < 8; q+=BITS_PER_POINT) // Iterate through each bit pair in the byte
        {
            uint8_t bit_pair = (data[byte] >> q) & 0x03;
            int16_t bit_pair_mag = (data[byte] >> (q+2)) & 0x03;
            switch(bit_pair)
            {
                case 0b00:  // 45 degrees
                    buffer.bin[buffer_idx].real = (AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1);
                    buffer.bin[buffer_idx].imag = (AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1);
                    break;
                case 0b01: // 135 degrees
                    buffer.bin[buffer_idx].real = -((AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1));
                    buffer.bin[buffer_idx].imag = (AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1);
                    break;
                case 0b10: // 225 degrees
                    buffer.bin[buffer_idx].real = -((AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1));
                    buffer.bin[buffer_idx].imag = -((AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1));
                    break;
                case 0b11: // 315 degrees
                    buffer.bin[buffer_idx].real = (AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1);
                    buffer.bin[buffer_idx].imag = -((AMP_SPACE*bit_pair_mag) + (AMP_SPACE>>1));
                    break;
            }
            buffer_idx++;
        }
    }

    arm_rfft_q15(&recv_ifft_inst, buffer.raw, output);
}

static void fmofdm_create_burst(uint8_t * data, uint32_t length, q15_t * burst_output)
{
    // Create Preamble
    fmofdm_create_preamble(burst_output);
    for(int i = 1; i < NUM_PREAMBLE; i++)
    {
        memcpy(&burst_output[i*NUM_FFT_LEN], burst_output, sizeof(q15_t)*NUM_FFT_LEN);
    }

    // Create Signal Symbol
    fmofdm_create_symbol((uint8_t *)&length, &burst_output[NUM_PREAMBLE*NUM_FFT_LEN]);

    // Create OFDM symbols based on the input data
    for(int byte = 0; byte < length; byte+=BYTES_PER_SYMBOL)
    {
        uint32_t burst_output_idx = NUM_PREAMBLE + NUM_SIGNAL_SYMBOLS + (byte/BYTES_PER_SYMBOL);
        burst_output_idx *= NUM_FFT_LEN;
        fmofdm_create_symbol(&data[byte], &burst_output[burst_output_idx]);
    }

}

void fmofdm_send_data(uint8_t * data, uint16_t length)
{
    fmofdm_create_burst(data, length, (q15_t *)fmofdm_burst);

    if(length % BYTES_PER_SYMBOL)
    {
        length += BYTES_PER_SYMBOL - (length % BYTES_PER_SYMBOL);
    }

    for(int j = 0; j < (NUM_PREAMBLE + NUM_SIGNAL_SYMBOLS + length/BYTES_PER_SYMBOL); j++)
    {
        convert_polar(fmofdm_burst[j], NUM_FFT_LEN);
    }
    dac_transmit((uint16_t *)fmofdm_burst[0], NUM_FFT_LEN*(NUM_PREAMBLE + NUM_SIGNAL_SYMBOLS + length/BYTES_PER_SYMBOL)+1);
}

/*
    RX side functions and state machine
*/

#define RECV_MESSAGE_BUFFER_SIZE 1500
static uint8_t fmofdm_recv_messageBuffer_array[RECV_MESSAGE_BUFFER_SIZE];
static StaticMessageBuffer_t fmofdmMessageBufferStruct;
static MessageBufferHandle_t fmofdmMessageBuffer;

uint32_t fmofdm_recv_data(uint8_t * data, uint16_t length, TickType_t delay)
{
    return xMessageBufferReceive(fmofdmMessageBuffer, data, length, delay);
}

static uint32_t fmofdm_decode_symbol(cq15_t * fft_bins)
{
    uint32_t decoded_data = 0;
    uint32_t bin_idx = 1;
    for(int byte = 0; byte < BYTES_PER_SYMBOL; byte++)
    {
        bin_idx++;
        uint8_t dataByte = 0;
        for(int q = 0; q < 8; q+=BITS_PER_POINT) // Iterate through each bit pair in the byte
        {
            uint8_t bits = 0;
            if(fft_bins[bin_idx].real > 0 && fft_bins[bin_idx].imag > 0)
            {
                bits = 0b00;
            }
            else if(fft_bins[bin_idx].real < 0 && fft_bins[bin_idx].imag > 0)
            {
                bits = 0b01;
            }
            else if(fft_bins[bin_idx].real < 0 && fft_bins[bin_idx].imag < 0)
            {
                bits = 0b10;
            }
            else
            {
                bits = 0b11;
            }

            q15_t bin_mag;
            arm_cmplx_mag_q15((q15_t *) &fft_bins[bin_idx], &bin_mag, 1);
            
            if(bin_mag > 500)
            {
                bits |= 0b11 << 2;
            }
            else if(bin_mag > 300)
            {
                bits |= 0b10 << 2;
            }
            else if(bin_mag > 150)
            {
                bits |= 0b01 << 2;
            }

            dataByte |= bits << q;
            
            bin_idx++;
        }
        decoded_data |= (dataByte << (byte*8));
    }
    return decoded_data;
}

typedef enum{
    INIT,
    PREAMBLE,
    RECV_SIGNAL,
    RECV_DATA,
    NUM_STATES
} FMOFDM_RX_t;
static FMOFDM_RX_t recvState = INIT;
static FMOFDM_RX_t prev_recvState = INIT;

static char recv_bytes[NUM_DATA_SYMBOLS*BYTES_PER_SYMBOL];
static char recv_bytes_idx = 0;
void fmofdmTask(void * parameters)
{
    (void) parameters;
    static q15_t recvData[2*NUM_FFT_LEN];
    static q15_t recvBins[2*NUM_FFT_LEN];
    static q15_t recvTemp[NUM_FFT_LEN];

    dac_init();
    adc_init();

    arm_rfft_init_q15(&recv_ifft_inst, NUM_FFT_LEN, 1, 1);
    arm_rfft_init_q15(&recv_fft_inst, NUM_FFT_LEN, 0, 1);

    fmofdmMessageBuffer = xMessageBufferCreateStatic( sizeof( fmofdm_recv_messageBuffer_array ),
                                                            fmofdm_recv_messageBuffer_array,
                                                            &fmofdmMessageBufferStruct);

    // Configure and Enable TIM6 for DMA triggering
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
    TIM6->PSC = SystemCoreClock / 1000000 - 1;  // Prescaler for 1 MHz
    TIM6->ARR = 999;                            // Auto-reload for 1 KHz
    TIM6->DIER |= TIM_DIER_UDE | TIM_DIER_UIE;  // Enable update interrupt
    TIM6->CR2 = 0b010 << 4;                     // Select Update Event as TRGO
    TIM6->CR1 |= TIM_CR1_CEN;                   // Enable TIM6

    adc_start();

    uint32_t recv_num_bytes = 0;
    uint32_t preamble_retry = 0;
    float angle_rcvd = 0.0f;
    char recv_bytes_idx = 0;
    while(1)
    {

        if(prev_recvState != INIT && recvState == INIT)
        {
            adc_flush();
        }
        prev_recvState = recvState;

        switch(recvState)
        {
            case INIT:
            {
                adc_readData(recvData, NUM_FFT_LEN);

                q15_t sig_rms;
                arm_rms_q15(recvData, NUM_FFT_LEN, &sig_rms);
                if(sig_rms >= 2000)   // Detected a signal
                {
                    // Read next chunk of samples to fix an edge case where the signal starts in the middle
                    adc_readData(recvData, NUM_FFT_LEN);
                    preamble_retry = 0;
                    recvState = PREAMBLE;
                }

                break;
            }
            case PREAMBLE:
            {
                // If we stay in PREAMBLE too long, then just reset back to INIT
                if(preamble_retry++ > NUM_PREAMBLE)
                {
                    recvState = INIT;
                    break;
                }

                cq15_t * recvBins_cq = (cq15_t *)recvBins;
                memcpy(recvTemp, recvData, sizeof(q15_t)*NUM_FFT_LEN);

                arm_rfft_q15(&recv_fft_inst, recvData, recvBins);

                angle_rcvd = atan2f( ((float)recvBins_cq[1].imag) / (float)(1 << 15), ((float)recvBins_cq[1].real) / (float)(1 << 15));
                angle_rcvd *= 180.0f / (float)M_PI; // Convert to degrees
                angle_rcvd -= 45.0f;
                if(angle_rcvd < 0)
                {
                    angle_rcvd += 360.0f;
                }

                int8_t phase_offset = (int8_t)((angle_rcvd + PHASE_RES/2) / PHASE_RES);
                phase_offset = phase_offset % NUM_FFT_LEN;
                if(phase_offset != 0)
                {
                    memcpy(recvData, recvTemp, sizeof(q15_t)*NUM_FFT_LEN);
                    memmove(&recvData[0], &recvData[NUM_FFT_LEN-phase_offset], sizeof(q15_t)*phase_offset);
                    adc_readData(&recvData[phase_offset], NUM_FFT_LEN-phase_offset);
                }
                else
                {
                    // Throw away samples until we reach a DATA SYMBOL
                    if(recvBins_cq[15].real > 300)
                    {
                        adc_readData(recvData, NUM_FFT_LEN);
                    }
                    else
                    {
                        memcpy(recvData, recvTemp, sizeof(q15_t)*NUM_FFT_LEN);
                        recvState = RECV_SIGNAL;
                    }
                }

                break;
            }
            case RECV_SIGNAL:
            {
                recvState = RECV_SIGNAL;

                recv_bytes_idx = 0;
                memset((void *)recv_bytes, 0, sizeof(recv_bytes));

                arm_rfft_q15(&recv_fft_inst, recvData, recvBins);
                recv_num_bytes = fmofdm_decode_symbol((cq15_t *)recvBins);
                if(recv_num_bytes > NUM_DATA_SYMBOLS*BYTES_PER_SYMBOL)
                {
                    recvState = INIT;
                }
                else
                {
                    recvState = RECV_DATA;
                }

                break;
            }
            case RECV_DATA:
            {
                recvState = RECV_DATA;

                adc_readData(recvData, NUM_FFT_LEN);

                q15_t sig_rms;
                arm_rms_q15(recvData, NUM_FFT_LEN, &sig_rms);
                if(sig_rms < 2000)   // Signal Power is too low, ignore data
                {
                    recvState = INIT;
                    break;
                }
                
                arm_rfft_q15(&recv_fft_inst, recvData, recvBins);

                uint32_t symbol_data = fmofdm_decode_symbol((cq15_t *)recvBins);
                for(int byte = 0; byte < BYTES_PER_SYMBOL; byte++)
                {
                    recv_bytes[recv_bytes_idx++] = (symbol_data >> (byte*8)) & 0xFF;
                    recv_num_bytes--;
                    if(recv_num_bytes == 0)
                    {
                        xMessageBufferSend(fmofdmMessageBuffer,
                                            recv_bytes,
                                            recv_bytes_idx,
                                            0);
                        recvState = INIT;
                        break;
                    }
                }

                break;
            }
            default:
            {
                while(1) __NOP(); // FW done goofed
            }
        }
    }
}