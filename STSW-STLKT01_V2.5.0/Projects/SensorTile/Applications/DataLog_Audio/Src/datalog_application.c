/**
  ******************************************************************************
  * @file    datalog_application.c
  * @author  SRA
  * @version v2.5.0
  * @date    23-Apr-2021
  * @brief   This file provides a set of functions to handle the datalog
  *          application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "datalog_application.h"
#include "main.h"
#include "string.h"
#include "SensorTile.h"
#include <math.h>
    
/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"


FRESULT res;                                          /* FatFs function common result code */
uint32_t byteswritten, bytesread;                     /* File write/read counts */
FATFS SDFatFs;  /* File system object for SD card logical drive */
FIL MyFile;     /* File object */
char SDPath[4]; /* SD card logical drive path */
    
volatile uint8_t SD_Log_Enabled = 0;

char newLine[] = "\r\n";

static BSP_MOTION_SENSOR_Event_Status_t MotionStatus;

extern uint16_t Audio_OUT_Buff[SIZE_BUFF];
extern  volatile int  index_buff;

volatile uint8_t pHeader[44];
static uint32_t WavProcess_HeaderInit(void);
static uint32_t WavProcess_HeaderUpdate(uint32_t len);
static void MX_DataLogTerminal_Init(void);  


/**
  * @brief  Initialize the wave header file
  * @param  pHeader: Header Buffer to be filled
  * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
  * @retval 0 if passed, !0 if failed.
  */
static uint32_t WavProcess_HeaderInit(void)
{
  uint16_t   BitPerSample=16;
  uint16_t   NbrChannels=AUDIO_CHANNELS;
  uint32_t   ByteRate=AUDIO_SAMPLING_FREQUENCY*(BitPerSample/8);
   
  uint32_t   SampleRate=AUDIO_SAMPLING_FREQUENCY;  
  uint16_t   BlockAlign= NbrChannels * (BitPerSample/8);
  
  /* Write chunkID, must be 'RIFF'  ------------------------------------------*/
  pHeader[0] = 'R';
  pHeader[1] = 'I';
  pHeader[2] = 'F';
  pHeader[3] = 'F';
  
  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the 
     recording opearation.  Example: 661500 Btyes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pHeader[4] = 0x00;
  pHeader[5] = 0x4C;
  pHeader[6] = 0x1D;
  pHeader[7] = 0x00;
  
  /* Write the file format, must be 'WAVE' -----------------------------------*/
  pHeader[8]  = 'W';
  pHeader[9]  = 'A';
  pHeader[10] = 'V';
  pHeader[11] = 'E';
  
  /* Write the format chunk, must be'fmt ' -----------------------------------*/
  pHeader[12]  = 'f';
  pHeader[13]  = 'm';
  pHeader[14]  = 't';
  pHeader[15]  = ' ';
  
  /* Write the length of the 'fmt' data, must be 0x10 ------------------------*/
  pHeader[16]  = 0x10;
  pHeader[17]  = 0x00;
  pHeader[18]  = 0x00;
  pHeader[19]  = 0x00;
  
  /* Write the audio format, must be 0x01 (PCM) ------------------------------*/
  pHeader[20]  = 0x01;
  pHeader[21]  = 0x00;
  
  /* Write the number of channels, ie. 0x01 (Mono) ---------------------------*/
  pHeader[22]  = NbrChannels;
  pHeader[23]  = 0x00;
  
  /* Write the Sample Rate in Hz ---------------------------------------------*/
  /* Write Little Endian ie. 8000 = 0x00001F40 => byte[24]=0x40, byte[27]=0x00*/
  pHeader[24]  = (uint8_t)((SampleRate & 0xFF));
  pHeader[25]  = (uint8_t)((SampleRate >> 8) & 0xFF);
  pHeader[26]  = (uint8_t)((SampleRate >> 16) & 0xFF);
  pHeader[27]  = (uint8_t)((SampleRate >> 24) & 0xFF);
  
  /* Write the Byte Rate -----------------------------------------------------*/
  pHeader[28]  = (uint8_t)(( ByteRate & 0xFF));                   
  pHeader[29]  = (uint8_t)(( ByteRate >> 8) & 0xFF);
  pHeader[30]  = (uint8_t)(( ByteRate >> 16) & 0xFF);
  pHeader[31]  = (uint8_t)(( ByteRate >> 24) & 0xFF);
  
  /* Write the block alignment -----------------------------------------------*/
  pHeader[32]  = BlockAlign;
  pHeader[33]  = 0x00;
  
  /* Write the number of bits per sample -------------------------------------*/
  pHeader[34]  = BitPerSample;
  pHeader[35]  = 0x00;
  
  /* Write the Data chunk, must be 'data' ------------------------------------*/
  pHeader[36]  = 'd';
  pHeader[37]  = 'a';
  pHeader[38]  = 't';
  pHeader[39]  = 'a';
  
  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  pHeader[40]  = 0x00;
  pHeader[41]  = 0x4C;
  pHeader[42]  = 0x1D;
  pHeader[43]  = 0x00;
  
  /* Return 0 if all operations are OK */
  return 0;
}


/**
  * @brief  Initialize the wave header file
  * @param  pHeader: Header Buffer to be filled
  * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
  * @retval 0 if passed, !0 if failed.
  */
static uint32_t WavProcess_HeaderUpdate(uint32_t len)
{
  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the 
     recording opearation.  Example: 661500 Btyes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pHeader[4] = (uint8_t)(len);
  pHeader[5] = (uint8_t)(len >> 8);
  pHeader[6] = (uint8_t)(len >> 16);
  pHeader[7] = (uint8_t)(len >> 24);
  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  len -=44;
  pHeader[40] = (uint8_t)(len); 
  pHeader[41] = (uint8_t)(len >> 8);
  pHeader[42] = (uint8_t)(len >> 16);
  pHeader[43] = (uint8_t)(len >> 24); 
  /* Return 0 if all operations are OK */
  return 0;
}

/**
* @brief Highest priority interrupt handler routine
* @param None
* @retval None
*/
void write_ms_on_sd(void)
{ 
  static uint32_t Audio_OUT_Buff_Index_out = 0;
  volatile FRESULT s;
  uint32_t byteswritten; /* File write/read counts */
   
  s=f_write(&MyFile,  &(((uint8_t *)Audio_OUT_Buff)[Audio_OUT_Buff_Index_out]), PCM_BYTES_X_MS, (void *)&byteswritten);
  Audio_OUT_Buff_Index_out = (Audio_OUT_Buff_Index_out+PCM_BYTES_X_MS)%(SIZE_BUFF*2);
  
  if(s != FR_OK)
  {
    BSP_LED_On(LED1);
    while(1);
  }
}

void write_on_sd(void)
{ 
  volatile FRESULT s;
  uint32_t byteswritten; /* File write/read counts */
  
  s=f_write(&MyFile,  &(((uint8_t *)Audio_OUT_Buff)[index_buff]), SIZE_BUFF, (void *)&byteswritten);
  if(s != FR_OK)
  {
    BSP_LED_On(LED1);
    while(1);
  }
}


/**
  * @brief  Start SD-Card demo
  * @param  None
  * @retval None
  */
void DATALOG_SD_Init(void)
{
  if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
  {
    /* Register the file system object to the FatFs module */
    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
    {
      /* FatFs Initialization Error */
      while(1)
      {
        BSP_LED_On(LED1);
        HAL_Delay(500);
        BSP_LED_Off(LED1);
        HAL_Delay(100);
      }
    }
  }
}
  

void DATALOG_SD_DeInit(void)
{
  FATFS_UnLinkDriver(SDPath);
}


/**
  * @brief  Start SD-Card demo
  * @param  None
  * @retval None
  */
uint8_t DATALOG_SD_Log_Enable(void)
{
  static uint16_t sdcard_file_counter = 0;
  //char header[] = " ";
  uint32_t byteswritten; /* written byte count */
  char file_name[30] = {0};
  static uint32_t delay,d=0;
  WavProcess_HeaderInit();

  /* SD SPI CS Config */
  SD_IO_CS_Init();
  
  do
  {
    sprintf(file_name, "%s%.3d%s", "SensorTile_Log_N", sdcard_file_counter, ".wav");
    sdcard_file_counter++;
  }
  while (f_stat(file_name,NULL) != FR_NO_FILE);
  
  delay = HAL_GetTick();  
  d=HAL_GetTick();
  do
  {
    d=HAL_GetTick();
  }while(d - delay < 100 );
  //   HAL_Delay(100);

  if(f_open(&MyFile, (char const*)file_name, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
  {
    return 0;
  }
  
  if(f_write(&MyFile, (uint8_t*) pHeader, sizeof(pHeader), (void *)&byteswritten) != FR_OK)
  {
    return 0;
  }
  return 1;
}

/**
  * @brief  Disable SDCard Log
  * @param  None
  * @retval None
  */
void DATALOG_SD_Log_Disable(void)
{
  uint32_t len;
  uint32_t byteswritten;                     /* File write/read counts */

  
  len = f_size(&MyFile);
  WavProcess_HeaderUpdate(len);

    /* Update the data length in the header of the recorded Wave */    
  f_lseek(&MyFile, 0);
  
  /* Parse the wav file header and extract required information */
  
  f_write(&MyFile, (uint8_t*)pHeader,  sizeof(pHeader), (void*)&byteswritten);
  
  /* Close file and unmount MyFilesystem */
    //f_sync(&MyFile);
  f_close(&MyFile);
  
  /* SD SPI Config */
  SD_IO_CS_DeInit();;
}

/**
  * @brief  Write New Line to file
  * @param  None
  * @retval None
  */
void DATALOG_SD_NewLine(void)
{
  uint32_t byteswritten; /* written byte count */
  f_write(&MyFile, (const void*)&newLine, 2, (void *)&byteswritten);
}


/**
* @brief  Splits a float into two integer values.
* @param  in the float value as input
* @param  out_int the pointer to the integer part as output
* @param  out_dec the pointer to the decimal part as output
* @param  dec_prec the decimal precision to be used
* @retval None
*/
void floatToInt( float in, int32_t *out_int, int32_t *out_dec, int32_t dec_prec )
{
  *out_int = (int32_t)in;
  if(in >= 0.0f)
  {
    in = in - (float)(*out_int);
  }
  else
  {
    in = (float)(*out_int) - in;
  }
  *out_dec = (int32_t)trunc(in * pow(10, dec_prec));
}


int32_t DoubleTap(void)
{
  int32_t ret = 0;
  BSP_MOTION_SENSOR_Get_Event_Status(LSM6DSM_0, &MotionStatus);
  if (MotionStatus.DoubleTapStatus==1)
  {
    ret = 1;
  }
  
  return ret;
}


void MX_X_CUBE_MEMS1_Init(void)
{
  MX_DataLogTerminal_Init();
}


/**
  * @brief  Initialize the DataLogTerminal application
  * @retval None
  */
void MX_DataLogTerminal_Init(void)
{  
  BSP_MOTION_SENSOR_Init(LSM6DSM_0, MOTION_ACCELERO | MOTION_GYRO);
  BSP_MOTION_SENSOR_SetOutputDataRate(LSM6DSM_0, MOTION_ACCELERO, LSM6DSM_ACC_ODR);
  BSP_MOTION_SENSOR_SetFullScale(LSM6DSM_0, MOTION_ACCELERO, LSM6DSM_ACC_FS);
  BSP_MOTION_SENSOR_SetOutputDataRate(LSM6DSM_0, MOTION_GYRO, LSM6DSM_GYRO_ODR);
  BSP_MOTION_SENSOR_SetFullScale(LSM6DSM_0, MOTION_GYRO, LSM6DSM_GYRO_FS);
            
  BSP_MOTION_SENSOR_Enable_Double_Tap_Detection(LSM6DSM_0, BSP_MOTION_SENSOR_INT2_PIN);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
