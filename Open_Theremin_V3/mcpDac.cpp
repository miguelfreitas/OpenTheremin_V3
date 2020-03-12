#include "Arduino.h"

#include "mcpDac.h"

#if USE_HW_SPI
uint8_t _mcpDacLSB;
bool _mcpHasDacLSB;

/* Performance notes (measuring ihandlers.cpp:INT1_vect):
 * Total function time:
 * 18.2us software SPI (original)
 * 15.9us USE_HW_SPI @ fck/4 = 4 MHz
 * 15.8us USE_HW_SPI + USE_SPI_INTERRUPT
 * 11.2us USE_HW_SPI + USE_SPI_INTERRUPT + USE_SPI_FAKE_INT (hackish!!)
 * 
 * The interrupt overhead basically kills the asynchronous USE_SPI_INTERRUPT time savings.
 * But the (hackish) USE_SPI_FAKE_INT is quite efficient.
 */


/*
 * SPI interrupt vector is called by transfer complete and is used to send the
 * remaining 8 bits of DAC value. SPI by interrupt is only used by audio DAC.
 * The calibration DAC (CS2) is not time-critical and we will do it synchronously (no interrupts).
  */
ISR (SPI_STC_vect) {
  // this ISR takes 0.6us (hasDacLSB) + 0.8us (!hasDacLSB)
  if( _mcpHasDacLSB ) {
    // must transmit the remaining 8 bits.
    // another interrupt is produced when done.
    _mcpHasDacLSB = 0;
    mcpSpiFirstInt(); //SPDR = _mcpDacLSB;
  } else {
    // all 16 bits sent. CS goes high now.
    mcpSpiSecondInt(); //mcpDacCsHigh();
    // mask SPI interrupts. we don't need it anymore.
    SPCR &= ~(1<<SPIE);
  }
}
#endif // USE_HW_SPI
