/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
#include <rs.h>

#define	min(a,b)	((a) < (b) ? (a) : (b))

#ifndef NULL
#define NULL ((void *)0)
#endif

#define NDATA 15

struct rs RS;
DTYPE rs_alpha_to[NDATA+1];
DTYPE rs_index_of[NDATA+1];
DTYPE rs_genpoly[MAX_ARRAY];
int rs_modnn_table[2<<((sizeof(unsigned char))*8)];

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits (1-8)
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 */
void INIT_RS(unsigned int symsize,unsigned int gfpoly,unsigned fcr,unsigned prim, unsigned int nroots){
  struct rs *rs = &RS;
  int sr,root,iprim;
  unsigned int i, j;

  if(symsize > 8*sizeof(DTYPE))
    return; /* Need version with ints rather than chars */

  if(fcr >= (1u<<symsize))
    return;
  if(prim == 0 || prim >= (1u<<symsize))
    return;
  if(nroots >= (1u<<symsize))
    return; /* Can't have more roots than symbol values! */

  //rs = (struct rs *)calloc(1,sizeof(struct rs));
  rs->mm = symsize;
  rs->nn = (1<<symsize)-1;

  rs->alpha_to = rs_alpha_to;
  rs->index_of = rs_index_of;

  /* Generate Galois field lookup tables */
  rs->index_of[0] = A0; /* log(zero) = -inf */
  rs->alpha_to[A0] = 0; /* alpha**-inf = 0 */
  sr = 1;
  for(i=0;i<rs->nn;i++){
    rs->index_of[sr] = i;
    rs->alpha_to[i] = sr;
    sr <<= 1;
    if(sr & (1<<symsize))
      sr ^= gfpoly;
    sr &= rs->nn;
  }
  if(sr != 1){
    /* field generator polynomial is not primitive! */
    return;
  }

  /* Form RS code generator polynomial from its roots */
  rs->genpoly = rs_genpoly;
  if(rs->genpoly == NULL){
    return;
  }
  rs->fcr = fcr;
  rs->prim = prim;
  rs->nroots = nroots;

  /* Find prim-th root of 1, used in decoding */
  for(iprim=1;(iprim % prim) != 0;iprim += rs->nn)
    ;
  rs->iprim = iprim / prim;

  rs->genpoly[0] = 1;
  for (i = 0,root=fcr*prim; i < nroots; i++,root += prim) {
    rs->genpoly[i+1] = 1;

    /* Multiply rs->genpoly[] by  @**(root + x) */
    for (j = i; j > 0; j--){
      if (rs->genpoly[j] != 0)
        rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[j]] + root)];
      else
        rs->genpoly[j] = rs->genpoly[j-1];
    }
    /* rs->genpoly[0] can never be zero */
    rs->genpoly[0] = rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[0]] + root)];
  }
  /* convert rs->genpoly[] to index form for quicker encoding */
  for (i = 0; i <= nroots; i++)
    rs->genpoly[i] = rs->index_of[rs->genpoly[i]];

  /* Form modnn lookup table */
  rs->modnn_table = rs_modnn_table;
  for(i = 0; i < (2<<((sizeof(unsigned char))*8)); i++){
    j = i;
    rs->modnn_table[i] = modnn(rs,j);
  }
}

void ENCODE_RS(DTYPE *data, DTYPE *bb){
  struct rs *rs = &RS;
  unsigned int i, j;
  DTYPE feedback;

  memset(bb,0,NROOTS*sizeof(DTYPE));

  for(i=0;i<NN-NROOTS;i++){
    feedback = INDEX_OF[data[i] ^ bb[0]];
    if(feedback != A0){      /* feedback term is non-zero */
      for(j=1;j<NROOTS;j++)
        bb[j] ^= ALPHA_TO[rs->modnn_table[feedback + GENPOLY[NROOTS-j]]];
    }
    /* Shift */
    memmove(&bb[0],&bb[1],sizeof(DTYPE)*(NROOTS-1));
    if(feedback != A0)
      bb[NROOTS-1] = ALPHA_TO[rs->modnn_table[feedback + GENPOLY[0]]];
    else
      bb[NROOTS-1] = 0;
  }
}

int DECODE_RS(DTYPE *data, int *eras_pos, int no_eras){
  struct rs *rs = &RS;

  int deg_lambda, el, deg_omega;
  int i, j, r, k;

  DTYPE u,q,tmp,num1,num2,den,discr_r;
  DTYPE lambda[MAX_ARRAY], s[MAX_ARRAY];	/* Err+Eras Locator poly
                                             * and syndrome poly */
  DTYPE b[MAX_ARRAY], t[MAX_ARRAY], omega[MAX_ARRAY];
  DTYPE root[MAX_ARRAY], reg[MAX_ARRAY], loc[MAX_ARRAY];

  int syn_error, count;

  /* form the syndromes; i.e., evaluate data(x) at roots of g(x) */
  for(i=0;(unsigned int)i<NROOTS;i++)
    s[i] = data[0];

  for(j=1;(unsigned int)j<NN;j++){
    for(i=0;(unsigned int)i<NROOTS;i++){
      if(s[i] == 0){
        s[i] = data[j];
      } else {
        s[i] = data[j] ^ ALPHA_TO[MODNN(INDEX_OF[s[i]] + (FCR+i)*PRIM)];
      }
    }
  }

  /* Convert syndromes to index form, checking for nonzero condition */
  syn_error = 0;
  for(i=0;(unsigned int)i<NROOTS;i++){
    syn_error |= s[i];
    s[i] = INDEX_OF[s[i]];
  }

  if (!syn_error) {
    /* if syndrome is zero, data[] is a codeword and there are no
     * errors to correct. So return data[] unmodified
     */
    count = 0;
    goto finish;
  }
  memset(&lambda[1],0,NROOTS*sizeof(lambda[0]));
  lambda[0] = 1;

  if (no_eras > 0) {
    /* Init lambda to be the erasure locator polynomial */
    lambda[1] = ALPHA_TO[MODNN(PRIM*(NN-1-eras_pos[0]))];
    for (i = 1; i < no_eras; i++) {
      u = MODNN(PRIM*(NN-1-eras_pos[i]));
      for (j = i+1; j > 0; j--) {
        tmp = INDEX_OF[lambda[j - 1]];
        if(tmp != A0)
          lambda[j] ^= ALPHA_TO[MODNN(u + tmp)];
      }
    }
  }
  for(i=0;(unsigned int)i<NROOTS+1;i++)
    b[i] = INDEX_OF[lambda[i]];

  /*
   * Begin Berlekamp-Massey algorithm to determine error+erasure
   * locator polynomial
   */
  r = no_eras;
  el = no_eras;
  while ((unsigned int)(++r) <= NROOTS) {	/* r is the step number */
    /* Compute discrepancy at the r-th step in poly-form */
    discr_r = 0;
    for (i = 0; i < r; i++){
      if ((lambda[i] != 0) && (s[r-i-1] != A0)) {
        discr_r ^= ALPHA_TO[MODNN(INDEX_OF[lambda[i]] + s[r-i-1])];
      }
    }
    discr_r = INDEX_OF[discr_r];	/* Index form */
    if (discr_r == A0) {
      /* 2 lines below: B(x) <-- x*B(x) */
      memmove(&b[1],b,NROOTS*sizeof(b[0]));
      b[0] = A0;
    } else {
      /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
      t[0] = lambda[0];
      for (i = 0 ; (unsigned int)i < NROOTS; i++) {
        if(b[i] != A0)
          t[i+1] = lambda[i+1] ^ ALPHA_TO[MODNN(discr_r + b[i])];
        else
          t[i+1] = lambda[i+1];
      }
      if (2 * el <= r + no_eras - 1) {
        el = r + no_eras - el;
        /*
         * 2 lines below: B(x) <-- inv(discr_r) *
         * lambda(x)
         */
        for (i = 0; (unsigned int)i <= NROOTS; i++)
          b[i] = (lambda[i] == 0) ? A0 : MODNN(INDEX_OF[lambda[i]] - discr_r + NN);
      } else {
        /* 2 lines below: B(x) <-- x*B(x) */
        memmove(&b[1],b,NROOTS*sizeof(b[0]));
        b[0] = A0;
      }
      memcpy(lambda,t,(NROOTS+1)*sizeof(t[0]));
    }
  }

  /* Convert lambda to index form and compute deg(lambda(x)) */
  deg_lambda = 0;
  for(i=0;(unsigned int)i<NROOTS+1;i++){
    lambda[i] = INDEX_OF[lambda[i]];
    if(lambda[i] != A0)
      deg_lambda = i;
  }
  /* Find roots of the error+erasure locator polynomial by Chien search */
  memcpy(&reg[1],&lambda[1],NROOTS*sizeof(reg[0]));
  count = 0;		/* Number of roots of lambda(x) */
  for (i = 1,k=IPRIM-1; (unsigned int)i <= NN; i++,k = MODNN(k+IPRIM)) {
    q = 1; /* lambda[0] is always 0 */
    for (j = deg_lambda; j > 0; j--){
      if (reg[j] != A0) {
        reg[j] = MODNN(reg[j] + j);
        q ^= ALPHA_TO[reg[j]];
      }
    }
    if (q != 0)
      continue; /* Not a root */
    /* store root (index-form) and error location number */
    root[count] = i;
    loc[count] = k;
    /* If we've already found max possible roots,
     * abort the search to save time
     */
    if(++count == deg_lambda)
      break;
  }
  if (deg_lambda != count) {
    /*
     * deg(lambda) unequal to number of roots => uncorrectable
     * error detected
     */
    count = -1;
    goto finish;
  }
  /*
   * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
   * x**NROOTS). in index form. Also find deg(omega).
   */
  deg_omega = 0;
  for (i = 0; (unsigned int)i < NROOTS;i++){
    tmp = 0;
    j = (deg_lambda < i) ? deg_lambda : i;
    for(;j >= 0; j--){
      if ((s[i - j] != A0) && (lambda[j] != A0))
        tmp ^= ALPHA_TO[MODNN(s[i - j] + lambda[j])];
    }
    if(tmp != 0)
      deg_omega = i;
    omega[i] = INDEX_OF[tmp];
  }
  omega[NROOTS] = A0;

  /*
   * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
   * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
   */
  for (j = count-1; j >=0; j--) {
    num1 = 0;
    for (i = deg_omega; i >= 0; i--) {
      if (omega[i] != A0)
        num1  ^= ALPHA_TO[MODNN(omega[i] + i * root[j])];
    }
    num2 = ALPHA_TO[MODNN(root[j] * (FCR - 1) + NN)];
    den = 0;

    /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
    for (i = (int)min((unsigned int)deg_lambda,NROOTS-1) & ~1; i >= 0; i -=2) {
      if(lambda[i+1] != A0)
        den ^= ALPHA_TO[MODNN(lambda[i+1] + i * root[j])];
    }
    if (den == 0) {
      count = -1;
      goto finish;
    }
    /* Apply error to data */
    if (num1 != 0) {
      data[loc[j]] ^= ALPHA_TO[MODNN(INDEX_OF[num1] + INDEX_OF[num2] + NN - INDEX_OF[den])];
    }
  }
  finish:
  if(eras_pos != NULL){
    for(i=0;i<count;i++)
      eras_pos[i] = loc[i];
  }
  return count;
}

// RS Utility with padding and interleaving
void RS_Encode(uint8_t *OutPtr, uint8_t *InPtr, int Length) {
  int id, datacount, rscount, incount, outcount, itercount;

  const int EncodedLen = RS_ENCODE_LEN(Length);
  // Modify length size
  Length *= 2;
  // Constant property
  const int Depth = __CEIL_DIV(Length, 7);
  const int PadDataLen = Length % 7;
  const int PadLen = (PadDataLen + 8) * Depth;

  // Change input and output to nibble type
  NibbleTypeDef *Out = (NibbleTypeDef *) OutPtr;
  NibbleTypeDef *In = (NibbleTypeDef *) InPtr;
  // Buffer for single RS encoding
  uint8_t Data[15];
  // Get RS loop
  rscount = Depth;
  incount = 0;
  itercount = 0;

  while (rscount-- > 0) {
    // Empty the data buffer
    memset(Data, 0, 15);
    // Check for data count
    if (rscount == 0) {
      datacount = PadDataLen;
    } else {
      datacount = 7;
    }
    // Reload the data buffer
    for (id = 0; id < datacount; id++) {
      Data[id] = NibbleRead(In, incount++);
    }
    // Do the RS encoding
    ENCODE_RS(Data, Data + 7);

    // Put the data on the output buffer
    outcount = 0;
    for (id = itercount++; id < EncodedLen;) {
      if (rscount == 0 && outcount == datacount) {
        outcount = 7;
      }
      NibbleWrite(Out, id, Data[outcount++]);
      // Update the id
      if (id < PadLen) {
        id += Depth;
      } else {
        id += Depth - 1;
      }
    }
  }
}

// RS Utility with padding and interleaving
void RS_Decode(uint8_t *OutPtr, uint8_t *InPtr, int Length) {
  int id, datacount, rscount, incount, outcount, itercount;

  // Modify length size
  Length *= 2;
  // Constant property
  const int Depth = __CEIL_DIV(Length, 15);
  const int PadDataLen = Length % 15;
  const int PadLen = PadDataLen * Depth;

  // Change input and output to nibble type
  NibbleTypeDef *Out = (NibbleTypeDef *) OutPtr;
  NibbleTypeDef *In = (NibbleTypeDef *) InPtr;
  // Buffer for single RS encoding
  uint8_t Data[15];
  // Get RS loop
  rscount = Depth;
  outcount = 0;
  itercount = 0;

  while (rscount-- > 0) {
    // Empty the data buffer
    memset(Data, 0, 15);
    // Check for data count
    if (rscount == 0) {
      datacount = PadDataLen - 8;
    } else {
      datacount = 15;
    }
    // Reload the data buffer
    incount = 0;
    for (id = itercount++; id < Length;) {
      // Align zeroes
      if (rscount == 0) {
        if (incount == datacount)
          incount = 7;
        else if (incount >= 15)
          break;
      }
      Data[incount++] = NibbleRead(In, id);
      // Update the id
      if (id < PadLen) {
        id += Depth;
      } else {
        id += Depth - 1;
      }
    }
    // Do the RS encoding
    DECODE_RS(Data, NULL, 0);
    // Put the data on the output buffer
    for (id = 0; id < 7; id++) {
      NibbleWrite(Out, outcount++, Data[id]);
    }
  }
}