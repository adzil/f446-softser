#include <fec.h>
#include <memory.h>

int main(void) {
  uint8_t In[] = "Ini coba CC doang.";
  uint8_t *OutRS;
  uint8_t *OutCC;
  uint8_t *DecCC;
  uint8_t *DecRS;
  uint16_t Len;
  uint16_t LenRS;
  uint16_t LenCC;

  Len = sizeof(In) - 1;
  LenRS = FEC_RS_BUFFER_LEN(Len);
  LenCC = FEC_CC_BUFFER_LEN(LenRS);
  // Initialze
  OutRS = MEM_Alloc(LenRS);
  OutCC = MEM_Alloc(LenCC);
  DecCC = MEM_Alloc(LenRS);
  DecRS = MEM_Alloc(Len);

  FEC_Init();

  // Encode Data
  FEC_RS_Encode(OutRS, In, Len);
  FEC_CC_Encode(OutCC, OutRS, LenRS);
  // Initialize decoder
  FEC_CC_DecodeInit(DecCC, LenRS);

  while(!FEC_CC_DecodeComplete()) {
    FEC_CC_DecodeInput(*(OutCC++));
  }
  // DecodeRS
  FEC_RS_Decode(DecRS, DecCC, Len);

  return 0;
}