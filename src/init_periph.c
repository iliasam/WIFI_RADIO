#include "init_periph.h"
#include "wifi_uart.h"
#include "stm32f4xx_conf.h"

//#define  USART1_DR_Address 0x40013804


extern uint8_t wifi_buffer[];

void init_all_periph(void)
{
  init_clk();
  init_gpio();
  init_uart_nvic();
  init_usart2();
  init_dma_wifi();
}

void init_dma_wifi(void)
{
  DMA_InitTypeDef           DMA_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  DMA_DeInit(DMA1_Stream5);

  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = USART2_DR_Address;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&wifi_buffer[0];
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory ;
  DMA_InitStructure.DMA_BufferSize = UART_BUF1_SIZE;

  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;

  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  
  DMA_Init(DMA1_Stream5, &DMA_InitStructure);
  
  NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  
  //DMA_Cmd(DMA1_Channel5, ENABLE);
}




void init_clk(void)
{

  //Ќастраиваем систему тактировани€
  //автоматически настоена на HSI - 168MHZ

  //RCC_ClocksTypeDef RCC_ClockFreq;
  ErrorStatus HSEStartUpStatus;
  RCC_DeInit();
  RCC_HSEConfig(RCC_HSE_ON);
  // Wait till HSE is ready
  HSEStartUpStatus = RCC_WaitForHSEStartUp();
  if(HSEStartUpStatus == SUCCESS)
  {
	  FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

	  //FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
	  //FLASH_SetLatency(FLASH_Latency_2);

	  /* HCLK = SYSCLK */
	  RCC_HCLKConfig(RCC_SYSCLK_Div1);
	  RCC_PCLK2Config(RCC_HCLK_Div1);
	  RCC_PCLK1Config(RCC_HCLK_Div1);

	  RCC_PLLConfig(RCC_PLLSource_HSE,8,336,2,7);
	  RCC_PLLCmd(ENABLE);
	  /* Wait till PLL is ready */
	  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
	  {;}
	  /* Select PLL as system clock source */
	  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	  /* Wait till PLL is used as system clock source */
	  while(RCC_GetSYSCLKSource() != 0x08)
	  {;}
  }
}

void init_gpio(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC,&GPIO_InitStructure);
}

void init_usart2(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  // Configure USART2 Rx (PA3) & USART2 Tx (PA2)

  /* Configure USART Tx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART Rx as alternate function  */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
      
  USART_DeInit(USART2);
  USART_StructInit(&USART_InitStructure);
  USART_InitStructure.USART_BaudRate = 460800;
  USART_Init(USART2, &USART_InitStructure);
  USART_Cmd(USART2, ENABLE);

  USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE);
  eneble_uart_int();

  USART_Cmd(USART2, ENABLE);
}


void eneble_uart_int(void)
{
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

}

void disable_uart_int(void)
{
  USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
}

//включает запросы от uart >>> DMA
void eneble_uart_req(void)
{
  USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE);
  DMA_Cmd(DMA1_Stream5, ENABLE);
  USART_Cmd(USART2, ENABLE);
}

void disable_uart_req(void)
{
  USART_DMACmd(USART2,USART_DMAReq_Rx,DISABLE);
  DMA_Cmd(DMA1_Stream5, DISABLE);
  USART_Cmd(USART2, DISABLE);
}




void init_uart_nvic(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


void init_dma_nvic(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


void delay_ms(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq (&RCC_Clocks);
  nCount=(RCC_Clocks.HCLK_Frequency/10000)*ms;
  for (; nCount!=0; nCount--);
}
