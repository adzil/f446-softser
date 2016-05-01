#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include <rs.h>
#include <nibble.h>
#include <macros.h>

int main(void) {
  // Initialize data
  uint8_t Data[] = {0x43, 0x22, 0xfa, 0xdd, 0x90, 0x78, 0x80, 0x67, 0x80};
  uint8_t *Encoded;
  uint8_t *Decoded;
  int Length, EncLength;

  // Initialize RS
  INIT_RS(4, 0x13, 1, 1, 8);
  // Calculate length
  Length = sizeof(Data);
  EncLength = __CEIL_DIV(RS_ENCODE_LEN(Length), 2);
  // Initialize encoded memory
  Encoded = calloc(EncLength, sizeof(uint8_t));
  Decoded = calloc(Length, sizeof(uint8_t));
  // Do RS encode
  RS_Encode(Encoded, Data, Length);
  RS_Decode(Decoded, Encoded, EncLength);

  return 0;
}
