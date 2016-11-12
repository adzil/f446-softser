#ifndef __MAC_PIB_H__
#define __MAC_PIB_H__

#include <inttypes.h>

typedef enum {
  MAC_PIB_VPAN_DEVICE = 0,
  MAC_PIB_VPAN_COORDINATOR = 1
} MAC_PibVpanCoordinator;

typedef enum {
  MAC_PIB_ASSOCIATED_RESET = 0,
  MAC_PIB_ASSOCIATED_SET = 1
} MAC_PibAssociatedCoord;

typedef struct {
  MAC_PibAssociatedCoord AssociatedCoord;
  MAC_PibVpanCoordinator VpanCoordinator;
  uint32_t CoordExtendedAdr;
  uint16_t CoordShortAdr;
  uint16_t ShortAdr;
  uint8_t DSN;
} MAC_Pib;

#endif // __MAC_PIB_H__
