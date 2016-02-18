/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIMEBASE_H
#define __TIMEBASE_H

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
void SysTick_Handler(void);
bool TB_Counter_Rx_Elapsed(void);
bool TB_Counter_Status_Elapsed(void);
bool TB_Counter_Rx_End_Elapsed(void);

void Set_TB_Counter_Rx(uint16_t value);
void Set_TB_Counter_Status(uint16_t value);
void Set_TB_Counter_Rx_End(uint16_t value);
#endif
