#ifndef __RADIO_H
#define __RADIO_H

#include "mad.h"
#include "main.h"

#ifndef ALLOC_SIZE
#define ALLOC_SIZE 10000
#endif

#define MAIN_ABUF_SIZE 50*1024//размер буфера, в который будут записываться исходные аудиоданные от UART
#define TMP_ABUF_SIZE  2048//размер временного буфера исходных аудиоданных

#define TIMEOUT 10000//время, после которого запускается перезапуск соединения

#define buf1_act 1
#define buf2_act 2


typedef enum
{
  ERR_NO = 0,
  ERR_NO_MODULE,
  ERR_NO_INET,
  ERR_NO_SERVER,
  ERR_TIMEOUT,
  ERR_HEADER,

} main_err_type;

typedef enum
{
  ABUF_P1 = 0,
  ABUF_P2,
} cur_abuf_type;


enum mad_flow input(void *data, struct mad_stream *stream);

void start_decode(void);
void wifi_routine(void);
void main_cycle(void);
uint16_t search_mp3_header(void);

void clear_buffer(void);

void read_data(uint8_t	*dest, uint16_t len);

#endif
