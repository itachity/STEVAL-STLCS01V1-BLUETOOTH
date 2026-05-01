/* This file used to be a build-disabled stub when CubeMX's USB stack and
 * BLE_SampleApp's USB stack were both being copied in. CubeMX is now the
 * single owner. We forward to the real CubeMX-generated header so that any
 * legacy code that still does #include "usbd_conf.h" picks up the right
 * USBD_HandleTypeDef layout, __IO/__PACKED definitions (via stm32l4xx_hal.h),
 * etc. Without this forward, the empty stub shadows the real header that
 * lives under USB_DEVICE/Target and the USB middleware fails to compile.
 */
#ifndef __USBD_CONF_FORWARD_H
#define __USBD_CONF_FORWARD_H

#include "../../USB_DEVICE/Target/usbd_conf.h"

#endif /* __USBD_CONF_FORWARD_H */
