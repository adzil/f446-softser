#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <rs.h>

int main(void) {
  // Initialize data
  uint8_t Data[] = "Fadhli Dzil Ikram";
  uint8_t *Encoded;
  uint8_t *Decoded;
  int Length, EncLength, i;

  // Initialize RS
  INIT_RS(4, 0x13, 1, 1, 8);
  // Calculate length
  Length = sizeof(Data) - 1;
  EncLength = RS_ENCODE_BUFF(Length);
  // Initialize encoded memory
  Encoded = calloc(EncLength, sizeof(uint8_t));
  Decoded = calloc(Length, sizeof(uint8_t));
  // Do RS encode
  RS_Encode(Encoded, Data, Length);
  for (i = 0; i < 10; i++) {
    Encoded[i+6] ^= 0xff;
  }
  RS_Decode(Decoded, Encoded, Length);

  return 0;
}
