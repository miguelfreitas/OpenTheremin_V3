#include "Arduino.h"

#include "ihandlers.h"
#include "mcpDac.h"
#include "timer.h"

#include "build.h"
#include "wavetables.h"

#include "simulavr_debug.h"

static const uint32_t MCP_DAC_BASE = 2047;

#define INT0_STATE    (PIND & (1<<PORTD2))
#define PC_STATE      (PINB & (1<<PORTB0))

uint8_t  vScaledVolume = 0;
uint16_t vPointerIncrement1 = 0;
uint16_t vPointerIncrement2 = 0;
uint16_t vPointerIncrement3 = 0;
uint16_t vPointerIncrement4 = 0;

volatile uint16_t pitch = 0;            // Pitch value
volatile uint16_t pitch_counter = 0;    // Pitch counter
volatile uint16_t pitch_counter_l = 0;  // Last value of pitch counter

volatile bool volumeValueAvailable = 0;  // Volume read flag
volatile bool pitchValueAvailable = 0;   // Pitch read flag
volatile bool reenableInt1 = 0;   // reeanble Int1

volatile uint16_t vol;                   // Volume value
volatile uint16_t vol_counter = 0;
volatile uint16_t vol_counter_i = 0;     // Volume counter
volatile uint16_t vol_counter_l;         // Last value of volume counter

volatile uint16_t timer_overflow_counter;         // counter for frequency measurement

uint16_t curWavetableBase = sine_table;

static uint16_t pointer1       = 0;  // Table pointer
static uint16_t pointer2       = 0;  // Table pointer
static uint16_t pointer3       = 0;  // Table pointer
static uint16_t pointer4       = 0;  // Table pointer
static volatile uint8_t  debounce_p, debounce_v  = 0;  // Counters for debouncing

void ihInitialiseTimer() {
  /* Setup Timer 1, 16 bit timer used to measure pitch and volume frequency */
  TCCR1A = 0;                     // Set Timer 1 to Normal port operation (Arduino does activate something here ?)
  TCCR1B = (1<<ICES1)|(1<<CS10);  // Input Capture Positive edge select, Run without prescaling (16 Mhz)
  TIMSK1 = (1<<ICIE1);            // Enable Input Capture Interrupt
  
  TCCR0A = 3; //Arduino Default: Fast PWM
  TCCR0B = 3; //Arduino Default: clk I/O /64 (From prescaler)
  TIMSK0 = 1; //Arduino Default: TOIE0: Timer/Counter0 Overflow Interrupt Enable
  
}

void ihInitialiseInterrupts() {
  /* Setup interrupts for Wave Generator and Volume read */
  EICRA = (1<<ISC00)|(1<<ISC01)|(1<<ISC11)|(1<<ISC10) ; // The rising edges of INT0 and INT1 generate an interrupt request.
  reenableInt1 = true;
  EIMSK = (1<<INT0)|(1<<INT1);                          // Enable External Interrupt INT0 and INT1
}

void ihInitialisePitchMeasurement() //Measurement of variable frequency oscillator on Timer 1
{   reenableInt1 = false;
    EIMSK =  0; // Disable External Interrupts
    TCCR1A = 0;           //Normal port operation Timer 1
    TIMSK1 = (1<<TOIE1);  //Timer/Counter1, Overflow Interrupt Enable
   
  }
  
void ihInitialiseVolumeMeasurement() //Measurement of variable frequency oscillator on Timer 0
{   reenableInt1 = false;
    EIMSK =  0; // Disable External Interrupts
    TIMSK1 = 0; //Timer/Counter1, Overflow Interrupt Disable

    TCCR0A = 0; // Normal port operation, OC0A disconnected. Timer 0
    TIMSK0 = (1<<OCIE0A);  //TOIE0: Timer/Counter0 Overflow Interrupt Enable
    OCR0A = 0xff; // set Output Compare Register0.
    
    TCCR1A = 0;  //Normal port operation Timer 1
    TCCR1B = (1<<CS10)|(1<<CS12); // clk I/O /1024 (From prescaler)
    TCCR1C=0;

    
  }

/* 16 bit by 8 bit multiplication */
static inline uint32_t mul_16_8(uint16_t a, uint8_t b)
{
  uint32_t product;
  asm (
    "mul %A1, %2\n\t"
    "movw %A0, r0\n\t"
    "clr %C0\n\t"
    "clr %D0\n\t"
    "mul %B1, %2\n\t"
    "add %B0, r0\n\t"
    "adc %C0, r1\n\t"
    "clr r1"
    :
    "=&r" (product)
    :
    "r" (a), "r" (b));
  return product;
}

/* Externaly generated 31250 Hz Interrupt for WAVE generator (32us) */
ISR (INT1_vect) {
  // Interrupt takes up a total of max 25 us

  // latch previous word to DAC output
  mcpDacLatchPulse();

  disableInt1(); // Disable External Interrupt INT1 to avoid recursive interrupts
  // Enable Interrupts to allow counter 1 interrupts
  interrupts();

  int16_t waveSample;
  static int16_t scaledSample = 0;

#if CV_ENABLED                                 // Generator for CV output

 mcpDacSend(min(vPointerIncrement, 4095));        //Send result to Digital to Analogue Converter (audio out) (9.6 us)

#else   //Play sound
  //Send (previous) scaledSample to Digital to Analogue Converter (audio out)
  //0.7us USE_HW_SPI=1, USE_SPI_INTERRUPT=1, USE_SPI_FAKE_INT=1
  //5.6us USE_HW_SPI=1, USE_SPI_INTERRUPT=0, USE_SPI_FAKE_INT=0
  //8.0us USE_HW_SPI=0, USE_SPI_INTERRUPT=0, USE_SPI_FAKE_INT=0
  mcpDacSend(scaledSample + MCP_DAC_BASE);
  simulavr_printhex(scaledSample, 16, "0x", "\n");

  // Read next wave table value (2.7us each)
  // Note: (pointer1>>5) & 0x7fe) is 0.3us faster than ((pointer1>>6) * 2 bytes per <uint16 *>)
  //       (pointer1>>5) & 0xfffe) is 0.07us faster than (pointer1>>5) & 0x7fe) => ok as shift right unsigned fills with zeros
  waveSample = (int16_t) pgm_read_word_near(curWavetableBase + ((pointer1>>5) & 0xfffe)); // 2.7us
  waveSample += (int16_t) pgm_read_word_near(curWavetableBase + ((pointer2>>5) & 0xfffe)); // 2.7us

#if USE_SPI_FAKE_INT
  // warning: we have not checks here, must ensure the transfer completed already!
  mcpSpiFirstInt();
#endif

  // Read next wave table value (2.7us each)
  waveSample += (int16_t) pgm_read_word_near(curWavetableBase + ((pointer3>>5) & 0xfffe));
  waveSample += (int16_t) pgm_read_word_near(curWavetableBase + ((pointer4>>5) & 0xfffe));
  waveSample = waveSample / 4; // (0.5us)

  //simulavr_printhex(waveSample, 16, "0x", " * ");
  //simulavr_printhex(vScaledVolume, 8, "0x", " = ");
  if (waveSample > 0) {                   // multiply 16 bit wave number by 8 bit volume value (1.6us / 2.3us)
    scaledSample = (mul_16_8(waveSample, vScaledVolume) >> 8);
  } else {
    scaledSample = -(mul_16_8(-waveSample, vScaledVolume) >> 8);
  }
  //simulavr_printhex(scaledSample, 16, "0x", "\n");

#if USE_SPI_FAKE_INT
  // warning: we have not checks here, must ensure the transfer completed already!
  mcpSpiSecondInt();
#endif

  //simulavr_printhex(vPointerIncrement1, 16, "0x", " ");
  //simulavr_printhex(vPointerIncrement2, 16, "0x", " ");
  //simulavr_printhex(vPointerIncrement3, 16, "0x", " ");
  //simulavr_printhex(vPointerIncrement4, 16, "0x", "\n");
  
  // increment table pointer (1us each)
  pointer1 += vPointerIncrement1;
  pointer2 += vPointerIncrement2;
  pointer3 += vPointerIncrement3;
  pointer4 += vPointerIncrement4;

#endif                          //CV play sound
  incrementTimer();               // update 32us timer

  if (PC_STATE) debounce_p++;
  if (debounce_p == 3) {
    noInterrupts();
    pitch_counter = ICR1;                      // Get Timer-Counter 1 value
    pitch = (pitch_counter - pitch_counter_l); // Counter change since last interrupt -> pitch value
    pitch_counter_l = pitch_counter;           // Set actual value as new last value
  };

  if (debounce_p == 5) {
    pitchValueAvailable = true;
  };

  if (INT0_STATE) debounce_v++;
  if (debounce_v == 3) {
    noInterrupts();
    vol_counter = vol_counter_i;            // Get Timer-Counter 1 value
    vol = (vol_counter - vol_counter_l);    // Counter change since last interrupt
    vol_counter_l = vol_counter;            // Set actual value as new last value
  };

  if (debounce_v == 5) {
    volumeValueAvailable = true;
  };

  noInterrupts();
  enableInt1();
}

/* VOLUME read - interrupt service routine for capturing volume counter value */
ISR (INT0_vect) {
  vol_counter_i = TCNT1;
  debounce_v = 0;
};


/* PITCH read - interrupt service routine for capturing pitch counter value */
ISR (TIMER1_CAPT_vect) {
  debounce_p = 0;
};


/* PITCH read absolute frequency - interrupt service routine for calibration measurement */
ISR(TIMER0_COMPA_vect)
{
  timer_overflow_counter++;
  }

/* VOLUME read absolute frequency - interrupt service routine for calibration measurement */
ISR(TIMER1_OVF_vect)
{
  timer_overflow_counter++;
}
