/**
 ******************************************************************************
 * @file    main.c
 * @author  SRA
 * @version v2.5.0
 * @date    23-Apr-2021
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include <string.h> /* strlen */
#include <stdio.h>  /* sprintf */
#include <math.h>   /* trunc */
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define AUDIO_IN_BUF_LEN   (AUDIO_CHANNELS*AUDIO_SAMPLING_FREQUENCY/1000)
#define AUDIO_OUT_BUF_LEN  (AUDIO_IN_BUF_LEN*8)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
BSP_AUDIO_Init_t MicParams;
BSP_AUDIO_Init_t OutParams;
uint16_t PCM_Buffer[AUDIO_IN_BUF_LEN];
volatile int16_t audio_out_buffer[AUDIO_OUT_BUF_LEN];

USBD_HandleTypeDef hUSBDDevice;
extern USBD_AUDIO_ItfTypeDef  USBD_AUDIO_fops;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main( void ) 
{
  /* STM32F4xx HAL library initialization:
  - Configure the Flash prefetch, instruction and Data caches
  - Configure the Systick to generate an interrupt each 1 msec
  - Set NVIC Group Priority to 4
  - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();
  
    /* enable USB power on Pwrctrl CR2 register */
  HAL_PWREx_EnableVddUSB();   
  /*Init USB*/
  /* Initialize USB descriptor basing on channels number and sampling frequency */
  USBD_AUDIO_Init_Microphone_Descriptor(&hUSBDDevice, AUDIO_SAMPLING_FREQUENCY, AUDIO_CHANNELS);
  /* Init Device Library */
  USBD_Init(&hUSBDDevice, &AUDIO_Desc, 0);
  /* Add Supported Class */
  USBD_RegisterClass(&hUSBDDevice, &USBD_AUDIO);
  /* Add Interface callbacks for AUDIO Class */  
  USBD_AUDIO_RegisterInterface(&hUSBDDevice, &USBD_AUDIO_fops);
  /* Start Device Process */
  USBD_Start(&hUSBDDevice);  
  
  /* Configure Audio Input peripheral - DFSDM */  
  MicParams.BitsPerSample = 16;
  MicParams.ChannelsNbr = AUDIO_CHANNELS;
  MicParams.Device = AUDIO_IN_DIGITAL_MIC;
  MicParams.SampleRate = AUDIO_SAMPLING_FREQUENCY;
  MicParams.Volume = AUDIO_VOLUME_INPUT;
  
  BSP_AUDIO_IN_Init(BSP_AUDIO_IN_INSTANCE, &MicParams); 
  
  /* Start Microphone acquisition */
  BSP_AUDIO_IN_Record(BSP_AUDIO_IN_INSTANCE, (uint8_t *) PCM_Buffer, AUDIO_IN_BUFFER_SIZE);
    
  /* Configure Audio Output peripheral (SAI) and external DAC */
  OutParams.BitsPerSample = 16;
  OutParams.ChannelsNbr = AUDIO_CHANNELS;
  OutParams.Device = 0;
  OutParams.SampleRate = AUDIO_SAMPLING_FREQUENCY;
  OutParams.Volume = AUDIO_VOLUME_OUTPUT;
  
  BSP_AUDIO_OUT_Init(BSP_AUDIO_OUT_INSTANCE, &OutParams);
  BSP_AUDIO_OUT_SetVolume(BSP_AUDIO_OUT_INSTANCE, OutParams.Volume);
  
  while (1)
  {
    /* Go to Sleep, everything is done in the interrupt service routines */
    __WFI();
  }
}


/**
* @brief  User function that is called when 1 ms of PDM data is available.
* 		  In this application only PDM to PCM conversion and USB streaming
*                  is performed.
* 		  User can add his own code here to perform some DSP or audio analysis.
* @param  none
* @retval None
*/
void AudioProcess(void)
{
  /*for L4 PDM to PCM conversion is performed in hardware by DFSDM peripheral*/
  static uint32_t IndexOut = 0;
  static uint32_t AudioOutActive = 0;
  uint32_t indexIn;
  
  Send_Audio_to_USB((int16_t *)PCM_Buffer, AUDIO_SAMPLING_FREQUENCY/1000*AUDIO_CHANNELS);  
  
  for(indexIn=0;indexIn<AUDIO_IN_BUF_LEN;indexIn++)
  {
    audio_out_buffer[IndexOut++] = PCM_Buffer[indexIn];
    audio_out_buffer[IndexOut++] = PCM_Buffer[indexIn];
  }
  
  if(!AudioOutActive && IndexOut==AUDIO_OUT_BUF_LEN/2)
  {
    BSP_AUDIO_OUT_Play(BSP_AUDIO_OUT_INSTANCE,(uint8_t*)audio_out_buffer, AUDIO_OUT_BUF_LEN);
    AudioOutActive=1;
  }
  if(IndexOut==AUDIO_OUT_BUF_LEN)
  {
    IndexOut=0;
  }
  
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance)
{  
  AudioProcess();
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance)
{  
  AudioProcess();
}

/**
 * @brief  This function is executed in case of error occurrence
 * @param  None
 * @retval None
 */
void Error_Handler( void )
{

  while (1)
  {}
}



#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed( uint8_t *file, uint32_t line )
{

  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  while (1)
  {}
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
