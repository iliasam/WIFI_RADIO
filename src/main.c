//by iliasam(citizen) 07.10.2012
//CoIDE project

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "radio.h"
#include "mad.h"


//IMPORTANT - STACK_SIZE = 0x00001000
//Важно, чтобы STACK_SIZE был равен 0x00001000

//LED4 - соединение с сервером установлено (connected to server)
//LED5 - мигает при воспроизведении звука (blinks at play process)
//LED6 - OVERRUN(поток нестабилен) (flow is not stable)
//LED3 - при ошибках (errors found)

int main(void)
{ 
  /* Initialize LEDS */
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
 
  STM_EVAL_LEDOn(LED3);
  delay_ms(200);
  STM_EVAL_LEDOff(LED3);

  main_cycle();
 while(1);
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
