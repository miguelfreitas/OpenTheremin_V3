/* Arduino WaveHC Library
 * Copyright (C) 2009 by William Greiman
 *  
 * This file is part of the Arduino WaveHC Library
 *  
 * This Library is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with the Arduino WaveHC Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 *  Macros and inline functions for MCP4921 DAC
 */   
#ifndef mcpDac_h
#define mcpDac_h

#include <avr/io.h>
#include "OTPinDefs.h"

// use atmel hardware SPI instead of software bitbanging
#define USE_HW_SPI

//------------------------------------------------------------------------------
#define mcpDacCsLow() MCP_DAC_CS_PORT &= ~_BV(MCP_DAC_CS_BIT)
#define mcpDacCsHigh() MCP_DAC_CS_PORT |= _BV(MCP_DAC_CS_BIT)

#define mcpDac2CsLow() MCP_DAC_CS_PORT &= ~_BV(MCP_DAC2_CS_BIT)
#define mcpDac2CsHigh() MCP_DAC_CS_PORT |= _BV(MCP_DAC2_CS_BIT)

#ifndef USE_HW_SPI
  #define mcpDacSckLow() MCP_DAC_SCK_PORT &= ~_BV(MCP_DAC_SCK_BIT)
  #define mcpDacSckHigh() MCP_DAC_SCK_PORT |= _BV(MCP_DAC_SCK_BIT)
  #define mcpDacSckPulse() {mcpDacSckHigh();mcpDacSckLow();}

  #define mcpDacSdiLow() MCP_DAC_SDI_PORT &= ~_BV(MCP_DAC_SDI_BIT)
  #define mcpDacSdiHigh() MCP_DAC_SDI_PORT |= _BV(MCP_DAC_SDI_BIT)
  #define mcpDacSdiSet(v) if(v){mcpDacSdiHigh();}else{mcpDacSdiLow();}

  // send bit b of d
  #define mcpDacSendBit(d, b) {mcpDacSdiSet(d&_BV(b));mcpDacSckPulse();}
#endif

#ifdef USE_MCP_DAC_LDAC
  #define mcpDacLatchLow() MCP_DAC_LDAC_PORT &= ~_BV(MCP_DAC_LDAC_BIT)
  #define mcpDacLatchHigh() MCP_DAC_LDAC_PORT |= _BV(MCP_DAC_LDAC_BIT)
#else
  #define mcpDacLatchLow()
  #define mcpDacLatchHigh()
#endif
#define mcpDacLatchPulse() {mcpDacLatchLow();mcpDacLatchHigh();}

//------------------------------------------------------------------------------
// init dac I/O ports
inline void mcpDacInit(void) {
  // set all to output mode
  MCP_DAC_CS_DDR |= _BV(MCP_DAC_CS_BIT);
  MCP_DAC2_CS_DDR |= _BV(MCP_DAC2_CS_BIT);

  MCP_DAC_SCK_DDR |= _BV(MCP_DAC_SCK_BIT);
  MCP_DAC_SDI_DDR |= _BV(MCP_DAC_SDI_BIT);
  // chip select high
  mcpDacCsHigh();
  mcpDac2CsHigh();

#if USE_MCP_DAC_LDAC
  // LDAC low always - use unbuffered mode
  MCP_DAC_LDAC_DDR |= _BV(MCP_DAC_LDAC_BIT);
  MCP_DAC_LDAC_PORT &= ~_BV(MCP_DAC_LDAC_BIT);
#endif // USE_MCP_DAC_LDAC

#ifdef USE_HW_SPI
  /* Enable SPI, Master, set clock rate fck/4 */
  SPCR = (1<<SPE)|(1<<MSTR);
#endif
}

#ifdef USE_HW_SPI
inline void mcpSpiWaitDone(void) {
  while(!(SPSR & (1<<SPIF)))
    ;
}
//extern uint8_t _spi_tmp_8bits;
#endif

//------------------------------------------------------------------------------
// send 12 bits to dac
// trusted compiler to optimize and it does 
// csLow to csHigh takes 8 - 9 usec on a 16 MHz Arduino
inline void mcpDacSend(uint16_t data) {
  mcpDacCsLow();

#ifndef USE_HW_SPI
  // send DAC config bits
  mcpDacSdiLow();
  mcpDacSckPulse();  // DAC A
  mcpDacSdiHigh();
  mcpDacSckPulse();  // buffered REF

  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  // send 12 data bits
  mcpDacSendBit(data, 11);
  mcpDacSendBit(data, 10);
  mcpDacSendBit(data,  9);
  mcpDacSendBit(data,  8);
  mcpDacSendBit(data,  7);
  mcpDacSendBit(data,  6);
  mcpDacSendBit(data,  5);
  mcpDacSendBit(data,  4);
  mcpDacSendBit(data,  3);
  mcpDacSendBit(data,  2);
  mcpDacSendBit(data,  1);
  mcpDacSendBit(data,  0);
  mcpDacCsHigh();
#else
  // keep 4 bits (11 to 8) of the MSB
  // set buffered REF (14), 1x gain (13), no SHDN (12)
  SPDR = ((data >> 8) & 0x0f) | 0x70;
  //_spi_tmp_8bits = (data & 0xff);

  mcpSpiWaitDone();
  SPDR = (data & 0xff);
  mcpSpiWaitDone();
  mcpDacCsHigh();
#endif
}

inline void mcpDac2ASend(uint16_t data) {
  mcpDac2CsLow();

#ifndef USE_HW_SPI
  // send DAC config bits
  mcpDacSdiLow();
  mcpDacSckPulse();  // DAC A
  mcpDacSdiHigh();
  mcpDacSckPulse();  // buffered REF

  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  // send 12 data bits
  mcpDacSendBit(data, 11);
  mcpDacSendBit(data, 10);
  mcpDacSendBit(data,  9);
  mcpDacSendBit(data,  8);
  mcpDacSendBit(data,  7);
  mcpDacSendBit(data,  6);
  mcpDacSendBit(data,  5);
  mcpDacSendBit(data,  4);
  mcpDacSendBit(data,  3);
  mcpDacSendBit(data,  2);
  mcpDacSendBit(data,  1);
  mcpDacSendBit(data,  0);
#else
  // keep 4 bits (11 to 8) of the MSB
  // set buffered DAC=0 (15), REF (14), 1x gain (13), no SHDN (12)
  SPDR = ((data >> 8) & 0x0f) | 0x70;
  mcpSpiWaitDone();
  SPDR = (data & 0xff);
  mcpSpiWaitDone();
#endif

  mcpDac2CsHigh();
}

inline void mcpDac2BSend(uint16_t data) {
  mcpDac2CsLow();

#ifndef USE_HW_SPI
  // send DAC config bits
  mcpDacSdiHigh();
  mcpDacSckPulse();  // DAC B
  mcpDacSdiHigh();
  mcpDacSckPulse();  // buffered REF

  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  // send 12 data bits
  mcpDacSendBit(data, 11);
  mcpDacSendBit(data, 10);
  mcpDacSendBit(data,  9);
  mcpDacSendBit(data,  8);
  mcpDacSendBit(data,  7);
  mcpDacSendBit(data,  6);
  mcpDacSendBit(data,  5);
  mcpDacSendBit(data,  4);
  mcpDacSendBit(data,  3);
  mcpDacSendBit(data,  2);
  mcpDacSendBit(data,  1);
  mcpDacSendBit(data,  0);
#else
  // keep 4 bits (11 to 8) of the MSB
  // set buffered DAC=1 (15), REF (14), 1x gain (13), no SHDN (12)
  SPDR = ((data >> 8) & 0x0f) | 0xf0;
  mcpSpiWaitDone();
  SPDR = (data & 0xff);
  mcpSpiWaitDone();
#endif

  mcpDac2CsHigh();
}



#endif //mcpDac_h
