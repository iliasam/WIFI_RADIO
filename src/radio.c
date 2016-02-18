#include "radio.h"
#include "main.h"
#include "mad.h"
#include "string.h"
#include "init_periph.h"
#include "wifi_module.h"
#include "wifi_uart.h"
#include "timebase.h"

//размеры массивов в хидере (sizes of arrays in header)
struct buffer {
  unsigned char *start;
  unsigned long length;
};


extern int alloc_free;
extern unsigned char *alloc_ptr;
extern unsigned char alloc_buffer[ALLOC_SIZE];

extern abuf_status_type	abuf_status;
extern main_status_type main_status;
extern abuf_run_type 	abuf_run;

main_err_type err_type = ERR_NO;

uint8_t MP3Buffer[TMP_ABUF_SIZE];	//Временный буфер, в который помещаются данные для последующего декодирования (tepm buffer storing data to be decoded later)
uint8_t main_abuf[MAIN_ABUF_SIZE]; //основной буфер аудиоданных, постепенно они дописываются сюда из UART (main raw data buffer, data comes here from UART)

uint8_t	*mp3_start = &main_abuf[0];//аудиоданные берутся отсюда (buffer for audio data)

RCC_ClocksTypeDef RCC_Clocks;
uint16_t audio_freq = 0;//текущая частота дискретезации (current audio sample rate)
uint16_t sample_fp = 0;//число семплов на фрейм (number of semples per frame)

volatile uint8_t j_flag = 0;//устанавливается, если указатель считывания перешел на начало буфера (set is read pointer was set to beginning of buffer)
//сбразывается, когда указатель записи перешел на начало (cleard when write cointer was set to beginning of buffer)
volatile  uint8_t overrun_st = 0;

volatile  int32_t delta = 0;



//****************************************************************************************************

//CALLBACK для аудиодекодера (mad.h)
//CALLBACK for audio decoder
enum mad_flow input(void *data, struct mad_stream *stream)	{
	struct buffer *buffer = data;
uint8_t	*destination;

uint16_t	num_get;
uint32_t	rest;

	destination = buffer->start; //сюда будут записываться данные
	rest = (uint32_t)(stream->bufend - stream->next_frame);//(buffer->start + num + rest) - stream->next_frame(число байтов, недопрочитанных декодером)

	if( stream->buffer )
	{
		memmove((void*)stream->buffer,(const void*)stream->next_frame, rest);
		destination = destination + rest;//недопрочитанные байты уже в буфере, пишем после них
	}

	num_get = TMP_ABUF_SIZE - rest;//столько байт осталось дописать в буфер

	if (num_get == 0)//такое возможно только при ошибке
	  {
	    destination = stream->next_frame;//попытка уйти от ошибки
	    num_get = 100;
	  }
	read_data(destination, num_get);//считывает данные из основного аудиобуфера во временный

	mad_stream_buffer( stream, buffer->start, TMP_ABUF_SIZE );//пересчитываются stream->buffer, stream->bufend, stream->next_frame

	if (TB_Counter_Rx_Elapsed() == true)//проверка на timeout
	  {
	    asm("nop");
	    //если вернуть не MAD_FLOW_CONTINUE, то декодирование остановится
	    return MAD_FLOW_STOP;
	  }

return MAD_FLOW_CONTINUE;
} // MP3InputStream





//переписывает len байтов из буфера исходных аудиоданных в dest, а так же управляет загрузкой главного аудиобуфера
//copy "len" bytes from raw audio data buffer to "dest" and controlling uasge of main audio buffer
void read_data(uint8_t	*dest, uint16_t len)
{
	static uint32_t readed = 0;//число уже прочитанных байтов от начала буфера
	uint16_t left;
	if ((readed + len) < MAIN_ABUF_SIZE)
	{
		memmove((void*)dest,(const void*)(mp3_start + readed), len);
		readed += len;
	}
	else
	{//в буфере недостаточно данных,часть данных нужно взять из его начала(там должны быть уже новые данные)
		memmove((void*)dest,(const void*)(mp3_start + readed), (MAIN_ABUF_SIZE - readed));//из конца буфера
		dest+= MAIN_ABUF_SIZE - readed;
		left = len - (MAIN_ABUF_SIZE - readed);
		memmove((void*)dest,(const void*)mp3_start, left);//из начала буфера
		readed = left;
		j_flag = 1;

	}
	if (j_flag == 0)
	  {
	    delta = (int32_t)readed - ((uint32_t)MAIN_ABUF_SIZE - (int32_t)(DMA1_Stream5->NDTR));//NDTR - число оставшихся
	  }
	else
	  {
	    delta = (int32_t)readed + (int32_t)(DMA1_Stream5->NDTR);//NDTR - число оставшихся
	  }

	if ((overrun_st == 1) || (delta < 0))
	  {
	    STM_EVAL_LEDOn(LED6);//слишком много данных(указатель записи перед указателем чтения)
	    if ((overrun_st == 1) && (delta > 2000)) {overrun_st = 0;}
	    disable_uart_req();//остановить запись
	  }
	else
	  {
	    STM_EVAL_LEDOff(LED6);
	    eneble_uart_req();
	  }


}



//производит установку соединения при помощи WIFI
//controlling conection to server using wifi module
void wifi_routine(void)
{
	while(!((main_status == MAIN_STAT_DATA) && ((TB_Counter_Rx_Elapsed() || (abuf_status != ABUF_NONE) )) ))
	{
	    //выполняется до тех пор, пока не придут первые аудиоданные или закончится время
	    rx_handler();
	    wifi_handler();
	}
	if (TB_Counter_Rx_Elapsed() == true){err_type = ERR_TIMEOUT; STM_EVAL_LEDOn(LED3);return;}
	audio_freq = search_mp3_header();//отпределить частоту дискретезеции звука
	if (audio_freq == 0){err_type = ERR_HEADER; STM_EVAL_LEDOn(LED3);return;}
}


void main_cycle(void)
{
	init_all_periph();//инициализация периферии

	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	while(1)
	  {
	    wifi_routine();

	    if (err_type != ERR_NO) //что-то пошло неправильно, перезагрузка
	    {
	        asm("nop");
	        //clear_buffer();
	        //wifi_reboot_cmd();
	        STM_EVAL_LEDOn(LED3);
	        delay_ms(1000);
	        NVIC_SystemReset();

	    }
	    else //все идет правильно, WIFI уже получает данные
	    {
	        main_status = MAIN_STAT_PLAY;
	        eneble_uart_req();

	    	WavePlayBack((uint32_t)audio_freq,sample_fp);//инициализация кодека
	    	start_decode();//запуск декодера mp3
	    	//после запуска декодирования программа не выходит из start_decode, до тех пор пока не появится ошибка
	    	//для передачи потока в декодер звука используется функция mad_flow input (radio.c)

	    	err_type = ERR_TIMEOUT;
	    	STM_EVAL_LEDOn(LED3);
	    	Set_TB_Counter_Rx(TIMEOUT);//чтобы дальше не сработало условие overrun
	    	DisableCodec();
	    	delay_ms(1000);
	    	STM_EVAL_LEDOff(LED3);
	    	err_type = ERR_NO;
	    	//clear_buffer();
	    	//wifi_reboot_cmd();
	    	NVIC_SystemReset();
	    }
	  }






}



void start_decode(void)
{
	  alloc_free=ALLOC_SIZE;
	  alloc_ptr=alloc_buffer;
	  decode(&MP3Buffer[0],MAIN_ABUF_SIZE);
}


//очищает аудиобуфер, для того, чтобы поиск заголовка не прочел старые данные
//cleaniing audio buffer (to protect finding old data during header search)
void clear_buffer(void)
{
  uint16_t i;
  for (i=0;i<4000;i++){main_abuf[i] = 0;}
}

//производит поиск заголовка фрейма mp3 и по нему определяет частоту дискретезеции звука и число семплов на фрейм
//try to find mp3 header and calculate sample rate and nubmer samples per frame
uint16_t search_mp3_header(void)
{
  uint16_t pos = 0;
  uint16_t pos2 = 0;//предполагаемая позиция
  uint8_t a =0;
  uint8_t b =0;
  uint8_t c =0;

  uint8_t b2 =0;//для того, чтобы убедиться, что pos2 - действительно заголовок
  uint8_t c2 =0;
  uint8_t found = 0;// 1- заголовок 1 фрейма, вероятно, найден

  uint16_t freq = 0;
  uint8_t mpeg_m = 0;//текущий номер версии mpeg
  sample_fp = 0;

  while (pos < 4000)//4000 - в таком числе байт обязательно должно быть несколько фреймов
  {
	  a = main_abuf[pos];
	  b = main_abuf[pos+1];
	  c = main_abuf[pos+2];

	  if ((( a & 0xff) == 0xff) && (( b & 0xe2) == 0xe2) && ((b & 0x04) == 0))//сравнение с маской заголовка
	  {
		  if (found == 0)
		  {
			  pos2 = pos;
			  found = 1;//после этого начинаем искать сдедующий заголовок
			  b2 = b;
			  c2 = c;
		  }
		  else
		  {
			 //ранее заголовок уже встречался
			  if ((c2 == c) && (b2 == b))
			  {
				  //pos2 - действительно заголовок

				  if ((b & 16) == 0){mpeg_m = 25;} else//опредеряем вид mpeg
				  {
					  if ((b & 8) == 0){mpeg_m = 2;} else {mpeg_m = 1;}
				  }

				  if (mpeg_m == 1)
				  {
					  sample_fp = 1152;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 44100; break;}
					  	case 1: {freq = 48000; break;}
					  	case 2: {freq = 32000; break;}
					  }
				  }
				  if (mpeg_m == 2)
				  {
					  sample_fp = 576;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 22050; break;}
					  	case 1: {freq = 24000; break;}
					  	case 2: {freq = 16000; break;}
					  }
				  }

				  if (mpeg_m == 25)
				  {
					  sample_fp = 576;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 11025; break;}
					  	case 1: {freq = 12000; break;}
					  	case 2: {freq = 8000; break;}
					  }
				  }

				  for (pos = 0;pos< pos2;pos++) {main_abuf[pos] = 0;}//очистить часть буфера до 1 заголовка
				  return freq;
			  }
			  else
			  {
				  //pos2 - не заголовок
				  pos = pos2+1;
				  found = 0;
			  }
		  }

	  }//end if
	  pos++;

  }//end while

  //если пришли сюда, то заголовок не нашелся, ошибка
  //header not found - error
  return 0;
}

