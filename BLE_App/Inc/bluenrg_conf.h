/**
 ******************************************************************************
 * @file    bluenrg_conf.h
 * @brief   BlueNRG-MS middleware configuration
 ******************************************************************************
 */

#ifndef __BLUENRG_CONF_H
#define __BLUENRG_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include <string.h>
#include "hci_tl_interface.h"

#define DEBUG                     0
#define PRINT_CSV_FORMAT          0
#define HCI_READ_PACKET_SIZE      128
#define HCI_MAX_PAYLOAD_SIZE      128
#define HCI_DEFAULT_TIMEOUT_MS    1000

#define BLUENRG_memcpy            memcpy
#define BLUENRG_memset            memset

#if (DEBUG == 1)
#include <stdio.h>
#define PRINTF(...)               printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if PRINT_CSV_FORMAT
#include <stdio.h>
#define PRINT_CSV(...)            printf(__VA_ARGS__)
void print_csv_time(void);
#else
#define PRINT_CSV(...)
#endif

#ifdef __cplusplus
}
#endif
#endif /* __BLUENRG_CONF_H */
