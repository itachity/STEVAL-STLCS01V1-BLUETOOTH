/**
 ******************************************************************************
 * @file    uuid_ble_service.h
 * @brief   UUIDs and little-endian pack macros for the ST BLE Sensor protocol
 ******************************************************************************
 */
#ifndef _UUID_BLE_SERVICE_H_
#define _UUID_BLE_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
{\
  uuid_struct[0 ] = uuid_0 ; uuid_struct[1 ] = uuid_1 ; uuid_struct[2 ] = uuid_2 ; uuid_struct[3 ] = uuid_3 ; \
  uuid_struct[4 ] = uuid_4 ; uuid_struct[5 ] = uuid_5 ; uuid_struct[6 ] = uuid_6 ; uuid_struct[7 ] = uuid_7 ; \
  uuid_struct[8 ] = uuid_8 ; uuid_struct[9 ] = uuid_9 ; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
  uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
}

#define STORE_LE_16(buf, val)  ( ((buf)[0] = (uint8_t)(val)     ) , \
                                 ((buf)[1] = (uint8_t)(val>>8)  ) )

#define STORE_LE_32(buf, val)  ( ((buf)[0] = (uint8_t)(val)     ) , \
                                 ((buf)[1] = (uint8_t)(val>>8)  ) , \
                                 ((buf)[2] = (uint8_t)(val>>16) ) , \
                                 ((buf)[3] = (uint8_t)(val>>24) ) )

/* ---------------- ST BLE Sensor protocol UUIDs ---------------- */

/* Hardware ("Features") service — base UUID */
#define COPY_HW_SENS_W2ST_SERVICE_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

/*
 * Environmental characteristic — the low-order UUID bits carry a feature mask
 *   bit 4 (0x10) : pressure present
 *   bit 3 (0x08) : humidity present
 *   bit 2 (0x04) : 1 temperature sensor
 *   bit 0 (0x05) : 2 temperature sensors (0x04 | 0x01)
 *
 * For a single-temperature payload (counter disguised as temp) the resulting
 * UUID is  00040000-0001-11E1-AC36-0002A5D5C51B .
 */
#define COPY_ENVIRONMENTAL_W2ST_CHAR_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

/* Console service + Stdout/Stdin characteristics (text / debug tab in the app) */
#define COPY_CONSOLE_SERVICE_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x0E,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_TERM_CHAR_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x01,0x00,0x0E,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_STDERR_CHAR_UUID(uuid_struct) \
  COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x02,0x00,0x0E,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

#ifdef __cplusplus
}
#endif

#endif /* _UUID_BLE_SERVICE_H_ */
