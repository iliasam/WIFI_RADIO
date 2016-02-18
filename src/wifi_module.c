#include "init_periph.h"
#include "wifi_uart.h"
#include "wifi_module.h"
#include "timebase.h"
#include "radio.h"

//#define REPEAT_DIS //отклюить запросы при потере пакетов

extern uint16_t rx_cnt;
extern uint8_t wifi_buffer[UART_BUF1_SIZE];

uint8_t noip_count = 0;//счетчик числа ошибок

wifi_status_type      wifi_status = WIFI_DATA;//состояние wifi модуля
wifi_command_type     wifi_command = WIFI_CMD_NO;//последняя команда для модуля
wifi_net_status_type  wifi_net_status = WIFI_DISCONNECTED;//состояние соединения с внешней wifi-точкой доступа
server_status_type    server_status = SERVER_NO;//этап соединения с сервером
wifi_open_status_type wifi_open_status = WIFI_CLOSED;//состояние соединения с сервером
main_status_type 	  main_status = MAIN_STAT_START;//этап работы



//переключится в режим команд
void wifi_switch_cmd(void)
{
  uint8_t data[] = {36,36,36};//$$$
  
  delay_ms(300);
  wifi_dma_config1(MID_CMD);
  wifi_command = WIFI_CMD_CMD;
  wifi_send_data_com(&data[0], 3);
}

//переключение в режим данных
void wifi_switch_data(void)
{
  uint8_t data[] = "exit";
  wifi_dma_config1(QUICK_CMD);
  wifi_command = WIFI_CMD_DATA;
  wifi_send_str_com(&data[0], 4);
}

//команда проверки подключения к сети
void wifi_status_cmd(void)
{
  uint8_t data[15] = "show connection";
  wifi_dma_config1(QUICK_CMD);
  wifi_command = WIFI_CMD_STATUS;
  wifi_send_str_com(&data[0], 15);
}

//команда отключает автоматические повторные запросы пакетов
void wifi_config_ip(void)
{
  delay_ms(100);
  uint8_t data[] = "set ip flags 3";//по умолчанию 7
  wifi_send_str_com(&data[0], 14);
  delay_ms(300);
}

//команда на подключение к серверу
void wifi_open_server_cmd(void)
{
  uint8_t length;

  //uint8_t data[] = "open 206.221.211.4 80@";//64kbit
  //uint8_t data[] = "open 72.29.87.97 8261@";//16kbit
  uint8_t data[] = "open 88.190.24.47 80@";//128kbit French Kiss FM shoutcast

  //uint8_t data[] = "open 188.127.243.169 80@";//192kbit best.fm icecast
  //uint8_t data[] = "open webcast1.emg.fm 55655@";//128kbit retro.fm icecast
  //uint8_t data[] = "open 80.237.210.12 5000@";//128kbit disco

  //uint8_t data[] = "open 217.20.164.163 8010@";//128kbit djfm
  //uint8_t data[] = "open 192.168.0.150 9000@";//local computer

  //uint8_t data[] = "open 46.42.5.110 8000@";//mayak

  length = sizeof(data);
  data[length-2] = 13;
  wifi_dma_config1(LONG_CMD);
  wifi_command = WIFI_CMD_OPEN_SERVER;
  wifi_send_data_com(&data[0], length-1);

}

//команда для сервера на передачу данных
void wifi_get_server_cmd(void)
{
	uint8_t tmp;
	uint8_t length;

	//uint8_t data[] = "GET / HTTP/1.1@#Host: 72.29.87.97@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";//16kbit
	uint8_t data[] = "GET / HTTP/1.1@#Host: 88.190.24.47@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";
	//uint8_t data[] = "GET / HTTP/1.1@#Host: 206.221.211.4@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";

	//uint8_t data[] = "GET /best-192 HTTP/1.1@#Host: 188.127.243.169@#Accept: */*@#Icy-MetaData:0@#@#";//best.fm
	//uint8_t data[] = "GET /retro128.mp3 HTTP/1.1@#Host: webcast1.emg.fm@#Accept: */*@#User-Agent: Mozilla/4.0 (compatible)@#Icy-MetaData:0@#Connection: close@#@#";

	//uint8_t data[] = "GET / HTTP/1.1@#Host: 80.237.210.12@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";//128kbit disco
	//uint8_t data[] = "GET / HTTP/1.1@#Host: 46.42.5.110@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";//96kbit mayak
	//uint8_t data[] = "GET / HTTP/1.1@#Host: 217.20.164.163@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";//128kbit djfm

	//uint8_t data[] = "GET / HTTP/1.1@#Host: 192.168.0.150@#Accept: */*@#Icy-MetaData:0@#Connection: close@#@#";//local

	length = sizeof(data);
	for (tmp = 0; tmp<length;tmp++)
	{
		if (data[tmp] == '@') {data[tmp] = 13;}
		if (data[tmp] == '#') {data[tmp] = 10;}
	}
    
  main_status = MAIN_STAT_DATA;
  wifi_dma_config2(LONG2_CMD);
  wifi_command = WIFI_CMD_GET_SERVER;
  disable_uart_int();//прерывания от UART отключаются, так как ожидается поток данных(DMA работает)
  wifi_send_data_com(&data[0], length-1);

}


//заркыть TCP/IP соединение
void wifi_close_cmd(void)
{
  uint8_t data[5] = "close";
  wifi_dma_config1(QUICK_CMD);
  wifi_command = WIFI_CMD_CLOSE;
  wifi_send_str_com(&data[0], 5);
}

void wifi_reboot_cmd(void)
{

  wifi_switch_cmd();
  delay_ms(300);

  uint8_t data[] = "reboot";
  //wifi_dma_config1(LONG_CMD);
  wifi_send_str_com(&data[0], 6);
  delay_ms(1000);


  wifi_status = WIFI_DATA;
  wifi_command = WIFI_CMD_NO;
  wifi_net_status = WIFI_DISCONNECTED;
  server_status = SERVER_NO;
  wifi_open_status = WIFI_CLOSED;
  wifi_command = WIFI_CMD_NO;
  main_status = MAIN_STAT_START;
  noip_count = 0;
  eneble_uart_int();
}


void wifi_handler(void)
{
  if((wifi_status != WIFI_CMD) && (wifi_command == WIFI_CMD_NO) && (wifi_open_status == WIFI_CLOSED))
  {
    if (wifi_status == WIFI_UNKNOWN)
    {
      wifi_switch_data();//переключится в режим данных
      delay_ms(250);
    }
    else
    {
      wifi_switch_cmd();//Пытается переключить в режим команд, если никакая команда не выполняется
    } 
  }
  
  if ((wifi_status == WIFI_CMD)&& (wifi_command == WIFI_CMD_NO) && (wifi_open_status == WIFI_CLOSED))//все в режиме команд
  {
      if (TB_Counter_Status_Elapsed())
      {
        Set_TB_Counter_Status(2000);//каждые 2 секунды, если нет соединений и команд
        wifi_status_cmd();//команда проверки подключения к сети
      }
      else
      if ((server_status == SERVER_NO)&&(wifi_net_status == WIFI_CONNECTED))
      {
#ifdef REPEAT_DIS//если нужно отключить поторные запросы пакетов в модуле
        wifi_config_ip();
#endif
        wifi_open_server_cmd();//попытаться подключится к серверу shoutcast
      }
  }
  
  // в режиме данных и при наличии соединения
  if ((wifi_status == WIFI_DATA)&& (wifi_command == WIFI_CMD_NO) && (wifi_open_status == WIFI_OPENED))
  {
    if (server_status == SERVER_OPEN)
    {
      delay_ms(200);
      wifi_get_server_cmd();//запрос потока даных(уже есть подключение к скрверу)
    }
  }

  if ((wifi_status == WIFI_ERR_CONNECTED) && (wifi_command == WIFI_CMD_NO))
  {
     wifi_close_cmd();
  }
  
  if ((wifi_status == WIFI_CMD)&& (wifi_command == WIFI_CMD_NO)&&(wifi_open_status == WIFI_OPENED))
  {
    //ошибка
     wifi_close_cmd();
  }

}
