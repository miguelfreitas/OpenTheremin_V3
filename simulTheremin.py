#!/usr/bin/python3
import sys
from os.path import splitext, basename

import pysimulavr
from ex_utils import SimulavrAdapter

class XPin(pysimulavr.Pin):
  
  def __init__(self, name, pin):
    pysimulavr.Pin.__init__(self)
    self.name = name
    self.__net = pysimulavr.Net()
    self.__net.Add(self)
    self.__net.Add(pin)
    
  def SetInState(self, pin):
    pysimulavr.Pin.SetInState(self, pin)
    #print("%s set to '%s' (t=%dns)" % (self.name, pin.toChar(), sim.getCurrentTime()))

  def __del__(self):
    del self.__net

if __name__ == "__main__":

  #proc, elffile = sys.argv[1].split(":")
  proc = "atmega328"
  dump0x20file = "dump0x20.raw" # "-"
  elffile = sys.argv[1]
  
  sim = SimulavrAdapter()
  sim.dmanSingleDeviceApplication()
  dev = sim.loadDevice(proc, elffile)
  dev.SetClockFreq(63) # 62.5ns = 16MHz
  rwout = pysimulavr.RWWriteToFile(dev, "FWRITE", dump0x20file)
  dev.ReplaceIoRegister(0x20, rwout)
  
  int1Period = round( (1./(8e6/256)) /2 / 1e-9 )
  
  sigs = ("IRQ.VECTOR9", "PORTA.PORT")
  sigs = ("PORTB.B2-Out",
            # DIN do DAC
            "PORTB.B3-Out",
            # SCK do DAC
            "PORTB.B5-Out",
            # LDAC
            "PORTD.D7-Out",
            "IRQ.VECTOR2",
            "TMRIRQ1.TIFR1",
            "SPI.SPSR",
            "SPI.SPCR",
            )
  sim.setVCDDump(splitext(basename(sys.argv[0]))[0] + ".vcd", sigs)
    
  xpin = XPin("port D.3", dev.GetPin("D3"))

  f_vol_pin = XPin("port D.2", dev.GetPin("D2"))
  f_pitch_pin = XPin("port B.0", dev.GetPin("B0"))
  
  hw_button_pin = XPin("port D.6", dev.GetPin("D6"))
  
  sim.dmanStart()
  
  print("simulation start: (t=%dns)" % sim.getCurrentTime())
  #sim.doRun(15000000)

  f_vol_pin.SetPin("H")
  f_pitch_pin.SetPin("H")
  hw_button_pin.SetPin("L")
  
  #for i in range(10000):
  while sim.getCurrentTime() < (1.5 / 1e-9):
    xpin.SetPin("L")
    sim.doRun(sim.getCurrentTime() + int1Period)
    xpin.SetPin("H")
    sim.doRun(sim.getCurrentTime() + int1Period)
  print("simulation end: (t=%dns)" % sim.getCurrentTime())

  #print("value 'timer2_ticks'=%d" % sim.getWordByName(dev, "timer2_ticks"))
  
  sim.dmanStop()
  del dev
  
# EOF
