#include <fec.h>
#include <memory.h>

int main(void) {
  uint8_t In[] = "Nama saya Fadhli Dzil Ikram. Saya tinggal di Jalan Dr. "
      "Saleh No. 4. Asal saya dari Bontang, Kalimantan Timur.";
  uint8_t *OutRS;
  uint8_t *OutCC;
  uint8_t *DecCC;
  uint8_t *DecRS;
  uint16_t Len;
  uint16_t LenRS;
  uint16_t LenCC;
  int i;

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

  // Introduce errors
  for (i = 0; i < 50; i++) {
    OutCC[i+10] ^= 0xff;
  }

  for (i = 0; i < 50; i++) {
    OutCC[i+500] ^= 0xff;
  }

  while(FEC_CC_DecodeComplete() != FEC_OK) {
    FEC_CC_DecodeInput(*(OutCC++));
  }
  // DecodeRS
  FEC_RS_Decode(DecRS, DecCC, Len);

  return 0;
}