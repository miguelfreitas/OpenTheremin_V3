/*
 * Debugging functions for simulavr console output
 * Add parameter "-W 0x20,-" to simulavr command line
 */

#define ENABLE_SIMULAVR_DEBUG 0

#if ENABLE_SIMULAVR_DEBUG
/* This port correponds to the "-W 0x20,-" command line option. */
#define SPECIAL_OUTPUT_PORT (*((volatile char *)0x20))

void simulavr_puts(const char *str) {
 const char *c;
 for(c = str; *c; c++)
   SPECIAL_OUTPUT_PORT = *c;
}

void simulavr_printhex(int32_t i, uint8_t bits, const char *prefix, const char *sufix) {
  int8_t shift;
  static const char digits[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
  simulavr_puts(prefix);
  for( shift = bits-4; shift >= 0; shift -= 4) {
    uint8_t c = (i >> shift) & 0xf;
    SPECIAL_OUTPUT_PORT = digits[c];   
  }
  simulavr_puts(sufix);
}

#else

#define simulavr_puts(s)
#define simulavr_printhex(i,bits,prefix,sufix)

#endif
