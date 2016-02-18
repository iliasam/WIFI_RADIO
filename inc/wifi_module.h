#ifndef __WIFI_MODULE
#define __WIFI_MODULE



typedef enum
{
  WIFI_DATA = 0,
  WIFI_CMD,
  WIFI_UNKNOWN,
  WIFI_ERR_CONNECTED,
  WIFI_FAIL,
} wifi_status_type;

typedef enum
{
  MAIN_STAT_START = 0,
  MAIN_STAT_CMD,
  MAIN_STAT_CONF,//переконфигурировать
  MAIN_STAT_DATA,//отправлен запрос аудиоданных
  MAIN_STAT_DATAREADY,//первая половина аудиобуфера заполнилась
  MAIN_STAT_PLAY,
} main_status_type;

typedef enum
{
  WIFI_CONNECTED = 0,
  WIFI_DISCONNECTED,
} wifi_net_status_type;

typedef enum
{
  RTC_SYNC = 0,
  RTC_NON_SYNC,
} rtc_status_type;

typedef enum
{
  WIFI_OPENED = 0,
  WIFI_CLOSED,
} wifi_open_status_type;

typedef enum
{
  SERVER_NO = 0,
  SERVER_OPEN,
  SERVER_DONE,
  SERVER_RECEIVED,
  SERVER_FAIL,
} server_status_type;

typedef enum
{
  WIFI_CMD_NO = 0,
  WIFI_CMD_CMD,
  WIFI_CMD_DATA,
  WIFI_CMD_OPEN_SERVER,
  WIFI_CMD_STATUS,
  WIFI_CMD_CLOSE,
  WIFI_CMD_GET_SERVER,
} wifi_command_type;

void wifi_switch_cmd(void);
void wifi_switch_data(void);
void wifi_status_cmd(void);
void wifi_open_server_cmd(void);
void wifi_close_cmd(void);
void wifi_get_server_cmd(void);
void wifi_reboot_cmd(void);

void wifi_handler(void);
void wifi_reconnection(void);
void wifi_config_ip(void);



#endif

