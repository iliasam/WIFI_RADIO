
#include "init_periph.h"
#include "wifi_uart.h"
#include "wifi_module.h"
#include "timebase.h"
#include "stm32f4xx.h"
#include "radio.h"

#include "stm32f4xx_conf.h"



uint32_t time = 0;

uint16_t rx_cnt=0;
uint8_t wifi_buffer[UART_BUF1_SIZE];
extern uint8_t main_abuf[];

uint16_t rx_length=0;

extern wifi_status_type 		wifi_status;
extern wifi_command_type 		wifi_command;
extern wifi_net_status_type  	wifi_net_status;
extern rtc_status_type   		rtc_status;
extern server_status_type   	server_status;
extern wifi_open_status_type 	wifi_open_status;
extern uint8_t 					noip_count;
extern main_status_type 	  	main_status;
abuf_status_type				abuf_status = ABUF_NONE;

#define RX_DELAY 50// ms

extern volatile uint8_t j_flag;
extern volatile uint8_t overrun_st;

void wifi_send_data_com(uint8_t* data, uint8_t length)
{
  uint8_t i;
  uint16_t cnt = 0;
  
  for (i=0;i<length;i++)
  {
    USART_SendData(USART2, (uint8_t)*data);
    data++;
    cnt = 0;
    //while((USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) && (cnt < 50000)){asm("nop");cnt++;}
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
  }
  
}

void wifi_send_str_com(uint8_t* data, uint8_t length)
{
  uint8_t i;
  
  for (i=0;i<length;i++)
  {
    USART_SendData(USART2, (uint8_t)*data);
    data++;
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
  }
  USART_SendData(USART2, (uint8_t)13);
  while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
}

//приготовиться к приему небольшого объема данных
void wifi_dma_config1(uint16_t length)
{
   uint8_t i;
   rx_cnt = 0;
   rx_length = length;

   for (i=0;i<30;i++){wifi_buffer[0] = 0;}//очистка буфера

   DMA_Cmd(DMA1_Stream5, DISABLE);
   DMA1->HIFCR = DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5| DMA_HIFCR_CTEIF5;

   //init_dma_wifi();
   DMA1_Stream5->M0AR = (uint32_t)&wifi_buffer[0];
   DMA1_Stream5->NDTR = UART_BUF1_SIZE;
   DMA_Cmd(DMA1_Stream5, ENABLE);
   Set_TB_Counter_Rx(rx_length);
   Set_TB_Counter_Rx_End(RX_DELAY);
}

//приготовиться к приему потока данных
void wifi_dma_config2(uint16_t length)
{
   DMA_InitTypeDef           DMA_InitStructure;

   rx_cnt = 0;
   rx_length = length;
   DMA_Cmd(DMA1_Stream5, DISABLE);

   DMA1->HIFCR = DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5| DMA_HIFCR_CTEIF5;

   DMA_DeInit(DMA1_Stream5);
   DMA_InitStructure.DMA_Channel = DMA_Channel_4;
   DMA_InitStructure.DMA_PeripheralBaseAddr = USART2_DR_Address;
   DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&main_abuf[0];
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory ;
   DMA_InitStructure.DMA_BufferSize = MAIN_ABUF_SIZE;

   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
   DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
   DMA_InitStructure.DMA_Priority = DMA_Priority_High;

   DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
   DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
   DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
   DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

   DMA_Init(DMA1_Stream5, &DMA_InitStructure);
   DMA_ITConfig(DMA1_Stream5,DMA_IT_TC | DMA_IT_HT,ENABLE);
   init_dma_nvic();

   DMA_Cmd(DMA1_Stream5, ENABLE);
   Set_TB_Counter_Rx(rx_length);
}




//отключить dma wifi
void wifi_dma_disable(void)
{
   DMA_Cmd(DMA1_Stream5, DISABLE);
}




void rx_handler(void)
{
  asm("nop");
  //if(TB_Counter_Rx_Elapsed() && (wifi_command != WIFI_CMD_NO))
  if ((TB_Counter_Rx_End_Elapsed() && (wifi_command != WIFI_CMD_NO) && (rx_cnt > 2)) || TB_Counter_Rx_Elapsed())
  {
     //прием данных завершен
    if (wifi_command != WIFI_CMD_GET_SERVER) {wifi_dma_disable();}//если передается поток, то DMA отключать не надо
    
    switch(wifi_command)
    {
      case(WIFI_CMD_CMD):           {command_cmd_rx_handler(); break;}//была послана команда на переключение в командный режим
      case(WIFI_CMD_DATA):          {command_data_rx_handler(); break;}//была послана команда на переключение в режим данных
      case(WIFI_CMD_STATUS):        {command_status_rx_handler(); break;}//была послана команда на полученние данных о подключении
      case(WIFI_CMD_OPEN_SERVER):  {command_open_server_rx_handler(); break;}//была послана команда для подключения к серверу
      case(WIFI_CMD_CLOSE):         {command_close_rx_handler(); break;}//была послана команда для закрытия подключения к серверу

      
      default: {break;}
    }
    
    if (wifi_command != WIFI_CMD_GET_SERVER) {wifi_command = WIFI_CMD_NO;}//если WIFI_CMD_GET_SERVER, то короткого ответа не будет
  }
}






//прием ответа на команду перехода в режим данных
void command_data_rx_handler(void)
{
  //static uint8_t err_cnt = 0;
  
   if ((wifi_buffer[0] == 69) && (wifi_buffer[3] == 84))//EXIT
   {//принято верно
      wifi_status = WIFI_DATA;
      //err_cnt = 0;
   }
   else
   if ((wifi_buffer[1] == 69) && (wifi_buffer[3] == 82))//$ERR
   {//сообщение об ошибке, выдается только в режиме команд, значит сейчас режим команд
    wifi_status = WIFI_CMD;
    //err_cnt = 0;
   }
   else
   {
      wifi_status = WIFI_UNKNOWN;
      //если модуль wifi уже находится в режиме данных, то он не ответит
      if (rx_cnt == 0) {wifi_status = WIFI_DATA;}
   }
  
}

//была послана команда на переключение в режим команд
void command_cmd_rx_handler(void)
{
  if ((wifi_buffer[0] == 67) && (wifi_buffer[2] == 68))//CMD
  {//принято верно
    wifi_status = WIFI_CMD;
    if (main_status == MAIN_STAT_START) {main_status = MAIN_STAT_CMD;}
  }
  else
  if ((wifi_buffer[1] == 69) && (wifi_buffer[3] == 82))//$ERR
  {//сообщение об ошибке, выдается только в режиме команд, значит сейчас режим команд
    wifi_status = WIFI_CMD;
  }
  else
  {
    
    wifi_status = WIFI_UNKNOWN;
  }
}

void command_status_rx_handler(void)
{
  if (wifi_buffer[0] == 56)//8
  {//принято верно
    //if ((wifi_buffer[3] == 48) || (wifi_buffer[3] == 49)) {wifi_net_status = WIFI_CONNECTED;} else {wifi_net_status = WIFI_DISCONNECTED;}
    if (wifi_buffer[3] == 48)//"0" idle
    {
      wifi_net_status = WIFI_CONNECTED;
      wifi_open_status = WIFI_CLOSED;
      noip_count = 0;
    }
    else
    if (wifi_buffer[3] == 49)// "1" connected
    {
      wifi_net_status = WIFI_CONNECTED;
      wifi_open_status = WIFI_OPENED;
      noip_count = 0;
    }
    else
    if (wifi_buffer[3] == 51) //"3" NOIP
    {
    	noip_count++;
    	if (noip_count > 3)
    	{
    		wifi_status = WIFI_FAIL;//модуль не смог подключится к сети
    		wifi_open_status = WIFI_CLOSED;
    	}

    }
    else {wifi_net_status = WIFI_DISCONNECTED;}
  }
  else {wifi_status = WIFI_UNKNOWN;}//ошибка приема
  
}



void command_open_server_rx_handler(void)
{
  uint8_t i = 0;

  while ((wifi_buffer[i] != 42) && (i < 10)){i++;}//поиск символа *
  if ((wifi_buffer[i+1] == 79) && (wifi_buffer[i+4] == 78))//$*OPEN*
  {//принято верно
   wifi_status = WIFI_DATA;
   server_status = SERVER_OPEN;//соединение с сервером установлено
   wifi_open_status = WIFI_OPENED;
   STM_EVAL_LEDOn(LED4);
  }
  else
  if ((wifi_buffer[1] == 67) && (wifi_buffer[9] == 70))//$Connect Failed
  {
    wifi_status = WIFI_CMD;
    wifi_open_status = WIFI_CLOSED;
    STM_EVAL_LEDOff(LED4);
  }
  
  else
  {
    asm("nop");//ошибка
  }
  
}



void command_close_rx_handler(void)
{
  if ((wifi_buffer[1] == 67) && (wifi_buffer[4] == 83))//*CLOS*
  {
    wifi_open_status = WIFI_CLOSED;
  }
  else
  {
	if ((wifi_buffer[0] == 69) && (wifi_buffer[5] == 67))//$ERR: CMD fail
	{
	  wifi_status = WIFI_CMD;
	  wifi_open_status = WIFI_CLOSED;
	}
  }
}

//ПРЕРЫВАНИЯ *******************************************************************************************

//прерывания по приему байта от модуля(используется при передаче малых объемов данных)
void USART2_IRQHandler(void)
{
  if ( USART_GetITStatus(USART2, USART_IT_RXNE) ) {USART_ClearITPendingBit(USART2, USART_IT_RXNE);}
    rx_cnt++;
    Set_TB_Counter_Rx(rx_length);
    Set_TB_Counter_Rx_End(RX_DELAY);


    if (rx_cnt > UART_BUF1_SIZE)
    {
     asm("nop");//СБОЙ!!!!!!!!!!!!!!!!!!
     //uart_stop_rx();
     disable_uart_int();
     wifi_reboot_cmd();
     init_usart2();//попытка заново переинициализировать UART
     rx_cnt = 0;
    }
}



//DMA работает с UART
void DMA1_Stream5_IRQHandler(void)//вызывается только если есть соединение с аудиосервером
{
  if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5))
  {
	  DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);//2 половина
	  abuf_status = ABUF_P2_COMPLETE;
	  Set_TB_Counter_Rx(TIMEOUT);

	  if (j_flag == 1)
	    {
	      j_flag = 0;
	      overrun_st = 0;//вроде нормально
	    }
	  else
	    {
	      overrun_st = 1;//ошибка
	      j_flag = 0;
	    }


  }
  else
  if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_HTIF5))// 1 половина
  {
	  DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_HTIF5);
	  if (main_status == MAIN_STAT_DATA) {disable_uart_req();}//для того, чтобы было время на последующую обработку данных
	  abuf_status = ABUF_P1_COMPLETE;
	  Set_TB_Counter_Rx(TIMEOUT);
  }
}
