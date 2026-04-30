/* Compat shim for legacy BLE_SampleApp code that included this header. */
#ifndef __USBD_CDC_INTERFACE_H_DISABLED
#define __USBD_CDC_INTERFACE_H_DISABLED
#include <stdint.h>
static inline uint8_t CDC_Fill_Buffer(uint8_t *Buf, uint32_t TotalLen) {
  (void)Buf; (void)TotalLen; return 0;
}
#endif
