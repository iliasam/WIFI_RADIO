#ifndef __WIFI_UART
#define __WIFI_UART

#include <stdint.h>

#define QUICK_CMD   (uint16_t)200
#define MID_CMD     (uint16_t)300
#define LONG_CMD    (uint16_t)1000
#define LONG2_CMD    (uint16_t)10000

#define  USART2_DR_Address 0x40004404

#define UART_BUF1_SIZE 200//размер буфера, в который будут записываться команды и данные от UART

typedef enum
{
  ABUF_P1_COMPLETE = 0,//заполнена 1 половина буфера
  ABUF_P2_COMPLETE,//заполнена 2 половина буфера
  ABUF_NONE,
} abuf_status_type;

typedef enum
{
  ABUF_STOP,//заполнение буфера остановлено
  ABUF_RUN,//разрешено заполнение буфера
} abuf_run_type;


void wifi_send_data_com(uint8_t* data, uint8_t length);
void wifi_send_str_com(uint8_t* data, uint8_t length);
void wifi_dma_config1(uint16_t length);
void wifi_dma_config2(uint16_t length);
void wifi_dma_disable(void);
void rx_handler(void);

void command_cmd_rx_handler(void);
void command_data_rx_handler(void);
void command_status_rx_handler(void);
void command_open_time_rx_handler(void);
void command_open_server_rx_handler(void);
void command_get_server_rx_handler(void);
void command_close_rx_handler(void);

#endif

