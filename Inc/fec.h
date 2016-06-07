#ifndef __FEC__
#define __FEC__

#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include "rs.h"
#include "macros.h"
#include "nibble.h"

#define FEC_CC_DEPTH 7
#define FEC_CC_MEMORY_LENGTH (1 << (FEC_CC_DEPTH - 1))
#define FEC_CC_DECODE_RESET_COUNT 32
#define FEC_CC_DECODE_CONTINUE_COUNT 8

#define FEC_RS_N 15
#define FEC_RS_K 7
#define FEC_RS_MULT(LEN) (LEN * 2)
#define FEC_RS_DIV(LEN) __CEIL_DIV(LEN, 2)
#define FEC_RS_ENCODE_LEN(LEN) (LEN + (FEC_RS_N - FEC_RS_K) * __CEIL_DIV(LEN, FEC_RS_K))

#define FEC_RS_BUFFER_LEN(LEN) FEC_RS_DIV(FEC_RS_ENCODE_LEN(FEC_RS_MULT(LEN)))
#define FEC_CC_BUFFER_LEN(LEN) (LEN * 4 + 3)

typedef enum {
  FEC_OK = 0x0,
  FEC_ERROR = 0x1
} FEC_Status;

typedef struct {
  uint16_t *Distance;
  uint16_t *LastDistance;
  uint32_t *Data;
  uint32_t *LastData;
  uint8_t *Output;
  uint16_t OutputLength;
  uint8_t MinId;
  uint8_t State;
  uint8_t Counter;
} FEC_CC_HandleTypeDef;

void FEC_CC_Encode(uint8_t *Output, uint8_t *Input, uint16_t Length);
void FEC_Init(void);
void FEC_CC_DecodeInput(uint8_t Input);
void FEC_CC_DecodeInit(uint8_t *Output, uint16_t Length);
FEC_Status FEC_CC_DecodeComplete(void);

void FEC_BYPASS_DecodeInit(uint8_t *Output, uint16_t Length);
void FEC_BYPASS_DecodeInput(uint8_t Input);

void FEC_RS_Encode(uint8_t *OutPtr, uint8_t *InPtr, int Length);
void FEC_RS_Decode(uint8_t *OutPtr, uint8_t *InPtr, int Length);

#endif // __FEC__
