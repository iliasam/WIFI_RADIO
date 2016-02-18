#ifndef __INIT_PERIPH
#define __INIT_PERIPH

#include <stdint.h>

void init_all_periph(void);
void init_clk(void);
void init_gpio(void);
void init_uart_nvic(void);
void init_dma_nvic(void);

void init_dma_wifi(void);
void init_usart2(void);

void eneble_uart_int(void);
void disable_uart_int(void);

//void uart_start_rx(void);
//void uart_stop_rx(void);

void eneble_uart_int(void);
void disable_uart_int(void);

void eneble_uart_req(void);
void disable_uart_req(void);


void delay_ms(uint32_t ms);


//#define PIN_HSY   GPIO_Pin_4
//#define PIN_VSY   GPIO_Pin_5
//#define SYNC_GPIO GPIOC

#endif
