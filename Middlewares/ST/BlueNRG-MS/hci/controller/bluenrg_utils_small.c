/* Disabled: bluenrg_utils_small.c is excluded from this build because the
 * BLE_SampleApp uses the full bluenrg_utils.c which defines the same
 * getBlueNRGVersion symbol. Linking both causes a duplicate-symbol error.
 *
 * Wrapping the original contents in #if 0 keeps the file in the source tree
 * (so the existing IDE source folders still resolve) but contributes no
 * symbols at link time.
 */
#if 0

#include "bluenrg_types.h"
#include "bluenrg_def.h"
#include "bluenrg_aci.h"
#include "bluenrg_utils.h"
#include "hci.h"
#include "hci_le.h"
#include "string.h"

uint8_t getBlueNRGVersion(uint8_t *hwVersion, uint16_t *fwVersion)
{
  /* original implementation lives in bluenrg_utils.c */
  (void)hwVersion;
  (void)fwVersion;
  return 0;
}

#endif /* 0 */
