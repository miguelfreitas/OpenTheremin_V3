#ifndef WAVETABLES_H
#define WAVETABLES_H

extern const int16_t sine_table[1024] PROGMEM;
extern const int16_t sine_table2[1024] PROGMEM;
extern const int16_t sine_table3[1024] PROGMEM;
extern const int16_t sine_table4[1024] PROGMEM;
extern const int16_t sine_table5[1024] PROGMEM;
extern const int16_t sine_table6[1024] PROGMEM;
extern const int16_t sine_table7[1024] PROGMEM;
extern const int16_t sine_table8[1024] PROGMEM;

#ifdef APPLICATION_CPP

static const int16_t* const wavetables[] = {
  sine_table,
  sine_table2,
  sine_table3,
  sine_table4,
  sine_table5,
  sine_table6,
  sine_table7,
  sine_table8
};

/* chordtable is also selected with wavetables
 * one chord may add up to four pitches.
 * each pitch is specified with numerator/denominator of the base pitch.
 */
static const int8_t chordtables[][8] = {
  //n1,d1, n2,d2, n3,d3, n4,d4
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 0
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 1
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 2
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 3
   { 1, 1,  5, 4,  3, 2, 15, 8}, // 4 CMaj7 = intervals 0,4,7,11 = 1/1, 5/4, 3/2, 15/8
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 5
   { 1, 1,  1, 1,  1, 1,  1, 1}, // 6
   { 1, 1,  1, 1,  1, 1,  1, 1}  // 7
};

#endif // APPLICATION_CPP

#endif // WAVETABLES_H
