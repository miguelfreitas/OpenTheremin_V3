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
static const int8_t chordtables[][4] = {
  //n1(d):n2:n3:n4
   { 1, 1, 1, 1 }, // 0
   { 1, 1, 1, 1 }, // 1
   { 1, 1, 1, 1 }, // 2
   { 1, 1, 1, 1 }, // 3
   //{ 8, 10, 12, 15 }, // 4 CMaj7 = intervals 0,4,7,11 = 1/1, 5/4, 3/2, 15/8 = 8:10:12:15
   //{ 40, 48, 60, 75}, // 5 CmMaj7 = intervals 0,3,7,11 = 1/1, 6/5, 3/2, 15/8 = 40:48:60:75
   //{ 4, 5, 6, 7}, // 5 C7 = intervals 0,3,7,11 = 1/1, 5/4, 3/2, 7/4 = 4:5:6:7
   //{ 3, 5, 9, 15}, // "anomalous saturated suspension" 
   //{ 12, 16, 17, 18}, // dream chord
   {6, 9, 12, 18}, // tibetan bowl n=2,3,4,6 = 6:9:12:18
   { 1, 1, 1, 1 }, // 6
   { 1, 1, 1, 1 },  // 7
   //{ 1, 1, 1, 1 },  // 7
};

#endif // APPLICATION_CPP

#endif // WAVETABLES_H
