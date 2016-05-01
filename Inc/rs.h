#ifndef __RS__
#define __RS__

/* Include file to configure the RS codec for character symbols
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */

#include <string.h>
#include <inttypes.h>

#include <nibble.h>
#include <macros.h>

#define PHY_RS_N 15
#define PHY_RS_K 7
#define RS_MULT(LEN) (LEN * 2)
#define RS_ENCODE_LEN(LEN) (PHY_RS_N * __CEIL_DIV(RS_MULT(LEN), PHY_RS_K) - \
    (PHY_RS_K - (RS_MULT(LEN) % PHY_RS_K)))
#define RS_DECODE_LEN(LEN) (PHY_RS_K * __CEIL_DIV(RS_MULT(LEN), PHY_RS_N) - \
    (PHY_RS_N - (RS_MULT(LEN) % PHY_RS_N)))

#define DTYPE unsigned char

/* Reed-Solomon codec control block */
struct rs {
  unsigned int mm;              /* Bits per symbol */
  unsigned int nn;              /* Symbols per block (= (1<<mm)-1) */
  unsigned char *alpha_to;      /* log lookup table */
  unsigned char *index_of;      /* Antilog lookup table */
  unsigned char *genpoly;       /* Generator polynomial */
  unsigned int nroots;     /* Number of generator roots = number of parity symbols */
  unsigned char fcr;        /* First consecutive root, index form */
  unsigned char prim;       /* Primitive element, index form */
  unsigned char iprim;      /* prim-th root of 1, index form */
  int *modnn_table;         /* modnn lookup table, 512 entries */
};

static inline unsigned int modnn(struct rs *rs, unsigned int x){
  while (x >= rs->nn) {
    x -= rs->nn;
    x = (x >> rs->mm) + (x & rs->nn);
  }
  return x;
}
#define MODNN(x) modnn(rs,x)

#define MM (rs->mm)
#define NN (rs->nn)
#define ALPHA_TO (rs->alpha_to)
#define INDEX_OF (rs->index_of)
#define GENPOLY (rs->genpoly)
#define NROOTS (rs->nroots)
#define FCR (rs->fcr)
#define PRIM (rs->prim)
#define IPRIM (rs->iprim)
#define A0 (NN)

#define MAX_ARRAY 9

#define ENCODE_RS encode_rs_char
#define DECODE_RS decode_rs_char
#define INIT_RS init_rs_char
#define FREE_RS free_rs_char

void ENCODE_RS(DTYPE *data,DTYPE *parity);
int DECODE_RS(DTYPE *data,int *eras_pos,int no_eras);
void INIT_RS(unsigned int symsize,unsigned int gfpoly,unsigned int fcr,
             unsigned int prim,unsigned int nroots);

void RS_Encode(uint8_t *OutPtr, uint8_t *InPtr, int Length);
void RS_Decode(uint8_t *OutPtr, uint8_t *InPtr, int Length);

#endif //F446_SOFTSER_RS_H
