#include "main.h"
#include "main.h"

uint16_t output_buff1[2*1152];
uint16_t output_buff2[2*1152];

__IO int output_buffnb;

#define dbgprintf(a,...) chprintf(NULL,a,__VA_ARGS__)

__IO uint32_t XferCplt = 0;
__IO uint8_t volume = 65;//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<   ÃÐÎÌÊÎÑÒÜ

extern __IO uint32_t AudioTotalSize;
extern __IO uint32_t AudioRemSize;
extern __IO uint16_t *CurrentPos;

static uint16_t samples_pf_l;

void WavePlayBack(uint32_t AudioFreq,uint16_t buf_size)
{ 

  WavePlayerInit(AudioFreq);// Initialize wave player (Codec, DMA, I2C)
  samples_pf_l = buf_size;
  //EVAL_AUDIO_Play((uint16_t *)output_buff1,2304*2);
  EVAL_AUDIO_Play((uint16_t *)output_buff1,buf_size*2*2);

  output_buffnb=1;
}

uint8_t WaveplayerCtrlVolume(uint8_t vol)
{ 
  EVAL_AUDIO_VolumeCtl(vol);
  return 0;
}

/**
* @brief  Initializes the wave player
* @param  AudioFreq: Audio sampling frequency
* @retval None
*/
int WavePlayerInit(uint32_t AudioFreq)
{ 
  int res;
  /* Initialize I2S interface */  
  EVAL_AUDIO_SetAudioInterface(AUDIO_INTERFACE_I2S);
  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */  
  res=EVAL_AUDIO_Init(OUTPUT_DEVICE_AUTO, volume, AudioFreq );  
  return res;
}

void DisableCodec(void)
{
  EVAL_AUDIO_Stop(CODEC_PDWN_SW);
  EVAL_AUDIO_DeInit();
}

/*--------------------------------
Callbacks implementation:
the callbacks prototypes are defined in the stm324xg_eval_audio_codec.h file
and their implementation should be done in the user code if they are needed.
Below some examples of callback implementations.
--------------------------------------------------------*/
/**
* @brief  Calculates the remaining file size and new position of the pointer.
* @param  None
* @retval None
*/
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{
  STM_EVAL_LEDToggle(LED5);

  if (output_buffnb==1)
  {
   //EVAL_AUDIO_Play((uint16_t *)output_buff2,2304*2);
	  EVAL_AUDIO_Play((uint16_t *)output_buff2,samples_pf_l*2*2);
   output_buffnb=2;
  }
  else
  {
   //EVAL_AUDIO_Play((uint16_t *)output_buff1,2304*2);
	  EVAL_AUDIO_Play((uint16_t *)output_buff1,samples_pf_l*2*2);
   output_buffnb=1;
  }

}

/**
* @brief  Manages the DMA Half Transfer complete interrupt.
* @param  None
* @retval None
*/
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size)
{  
  /* Generally this interrupt routine is used to load the buffer when 
  a streaming scheme is used: When first Half buffer is already transferred load 
  the new data to the first half of buffer while DMA is transferring data from 
  the second half. And when Transfer complete occurs, load the second half of 
  the buffer while the DMA is transferring from the first half ... */
}

/**
* @brief  Manages the DMA FIFO error interrupt.
* @param  None
* @retval None
*/
void EVAL_AUDIO_Error_CallBack(void* pData)
{
  /* Stop the program with an infinite loop */
  while (1)
  {}
  
  /* could also generate a system reset to recover from the error */
  /* .... */
}




#ifndef USE_DEFAULT_TIMEOUT_CALLBACK
/**
  * @brief  Basic management of the timeout situation.
  * @param  None.
  * @retval None.
  */
uint32_t Codec_TIMEOUT_UserCallback(void)
{   
  return (0);
}
#endif /* USE_DEFAULT_TIMEOUT_CALLBACK */
/*----------------------------------------------------------------------------*/

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
