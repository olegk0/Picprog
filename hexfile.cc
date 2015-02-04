/* -*- c++ -*-

This is Picprog, Microchip PIC programmer software for the serial port device.
Copyright © 1997,2002,2003,2004,2006,2008,2010 Jaakko Hyvätti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/ .

The author may be contacted at:

Email: Jaakko.Hyvatti@iki.fi
URL:   http://www.iki.fi/hyvatti/
Phone: +358 40 5011222

Please send any suggestions, bug reports, success stories etc. to the
Email address above.  Include word 'picprog' in the subject line to
make sure your email passes my spam filtering.

*/

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cassert>

#include <sysexits.h>
#include <unistd.h>

#include "hexfile.h"

using namespace std;

/*
  The exact programmable memory type, prom/eprom/eeprom/flash, does
  not affect the operations of the program.  Therefore I have not
  checked if every chip has the correct type specified.

  If rom type is specified, this program only reads the memory and
  does not atempt to burn it.

  Structure fields:

  - Name, used in command line option to select the device.
  - Program memory size in words.
  - Number of preserved words at the end of program memory, for example
    oscillator calibration word.  I have not checked all datasheets
    for which devices have one, only some.  Please check the datasheet
    before you overwrite these configuration words!  As a precaution,
    please read them first off the chip and write down somewhere!
  - configuration word preserved bits mask
  - number of configuration words. 1 for 14 bit devices, 14 for PIC18
    and 2 for 16f87 and 16f88.
  - Word bit length.  Only 14 bit and 16 bit (18f) ones supported.
  - For 18f series, panel size.  0 means multipanel writes disabled.
  - How many words or bytes to write at one command (18f)
  - Program memory type.
  - Non-volatile data memory size.
  - Non-volatile data memory type.
  - location 0x2006 or 0x3ffffe device id (-1 == device has no known id)
*/

const struct hexfile::devinf hexfile::deviceinfo [] = {

  // 16x8x family

  {"pic16c84", 1024, 0, 0, 1, 14, 0, 0, eeprom, 64, eeprom, -1}, // no OSCCAL
  {"pic16cr83", 512, 0, 0, 1, 14, 0, 0, rom, 64, eeprom, -1},
  {"pic16cr84", 1024, 0, 0, 1, 14, 0, 0, rom, 64, eeprom, -1},
  {"pic16f83", 512, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, -1},
  {"pic16f84", 1024, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, -1}, // no OSCCAL
  {"pic16f84a", 1024, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, 0x0560},
  {"pic16f87", 4096, 0, 0, 2, 14, 0, 0, flash5, 256, eeprom, 0x0720},
  {"pic16f88", 4096, 0, 0, 2, 14, 0, 0, flash5, 256, eeprom, 0x0760},

  // 16c6x family

  {"pic16c61",  1024, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1}, // ?
  {"pic16c62",  2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c62a", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c62b", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c63",  4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c63a", 4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c64",  2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c64a", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c65",  4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c65a", 4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c65b", 4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c66",  8192, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c66a", 8192, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c67",  8192, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16cr62", 2048, 0, 0, 1, 14, 0, 0, rom, 0, rom, -1},
  {"pic16cr63", 4096, 0, 0, 1, 14, 0, 0, rom, 0, rom, -1},
  {"pic16cr64", 2048, 0, 0, 1, 14, 0, 0, rom, 0, rom, -1},
  {"pic16cr65", 4096, 0, 0, 1, 14, 0, 0, rom, 0, rom, -1},

  // 16c62x family

  {"pic16c620", 512, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c620a", 512, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16cr620a", 512, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c621", 1024, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c621a", 1024, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c622", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c622a", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16f627",  1024, 0, 0, 1, 14, 0, 0, flash, 128, eeprom, 0x07a0}, // no OSCCAL
  {"pic16f627a", 1024, 0, 0, 1, 14, 0, 0, flash4, 128, eeprom, 0x1040}, // no OSCCAL
  {"pic16f628",  2048, 0, 0, 1, 14, 0, 0, flash, 128, eeprom, 0x07c0}, // no OSCCAL
  {"pic16f628a", 2048, 0, 0, 1, 14, 0, 0, flash4, 128, eeprom, 0x1060}, // no OSCCAL
  {"pic16f648a", 4096, 0, 0, 1, 14, 0, 0, flash4, 128, eeprom, 0x1100}, // no OSCCAL

  // 16f88x family
  {"pic16f883",  4096, 0, 0, 2, 14, 0, 0, flash4, 256, eeprom, 0x2020},
  {"pic16f884",  4096, 0, 0, 2, 14, 0, 0, flash4, 256, eeprom, 0x2040},
  {"pic16f886",  8192, 0, 0, 2, 14, 0, 0, flash4, 256, eeprom, 0x2060},
  {"pic16f887",  8192, 0, 0, 2, 14, 0, 0, flash4, 256, eeprom, 0x2080},

  // 16ce62x family

  {"pic16ce623", 512, 0, 0, 1, 14, 0, 0, eprom, 128, eeprom, -1},
  {"pic16ce624", 1024, 0, 0, 1, 14, 0, 0, eprom, 128, eeprom, -1},
  {"pic16ce625", 2048, 0, 0, 1, 14, 0, 0, eprom, 128, eeprom, -1},

  // 16c64x, 16c66x families

  {"pic16c641", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c642", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c661", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c662", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},

  // 16c7x, 16c77x families

  {"pic16c71",  1024, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c710", 512, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c711", 1024, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c712", 1024, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c715", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c716", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c717", 2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},

  {"pic16c72",  2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c72a",  2048, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16cr72",  2048, 0, 0, 1, 14, 0, 0, rom, 0, rom, -1},
  {"pic16c73",  4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c73a", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c73b", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c74",  4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c74a", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c74b", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c76", 8192, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c77", 8192, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},

  // 16f7x family

  {"pic16f72", 4096, 0, 0, 1, 14, 0, 0, flash2, 0, rom, 0x00a0},
  {"pic16f73", 4096, 0, 0, 1, 14, 0, 0, flash2, 0, rom, 0x0600},
  {"pic16f74", 4096, 0, 0, 1, 14, 0, 0, flash2, 0, rom, 0x0620},
  {"pic16f76", 8192, 0, 0, 1, 14, 0, 0, flash2, 0, rom, 0x0640},
  {"pic16f77", 8192, 0, 0, 1, 14, 0, 0, flash2, 0, rom, 0x0660},

  // 16f7x7 family

  {"pic16f737", 4096, 0, 0, 2, 14, 0, 0, flash2, 0, rom, 0x0ba0},
  {"pic16f747", 4096, 0, 0, 2, 14, 0, 0, flash2, 0, rom, 0x0be0},
  {"pic16f767", 8192, 0, 0, 2, 14, 0, 0, flash2, 0, rom, 0x0ea0},
  {"pic16f777", 8192, 0, 0, 2, 14, 0, 0, flash2, 0, rom, 0x0de0},

  // 16c43x family
  
  {"pic16c432", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c433", 2048, 1, 0, 1, 14, 0, 0, prom, 0, rom, -1},

  // 16c78x family

  {"pic16c781", 1024, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c782", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},

  // 16c7x5 family

  {"pic16c745", 8192, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},
  {"pic16c765", 8192, 0, 0, 1, 14, 0, 0, eprom, 0, rom, -1},

  // 16c77x family

  {"pic16c770", 2048, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c771", 4096, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c773", 8192, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  {"pic16c774", 8192, 0, 0, 1, 14, 0, 0, prom, 0, rom, -1},
  
  // 16f87x family

  {"pic16f870", 2048, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, 0x0d00},
  {"pic16f871", 2048, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, 0x0d20},
  {"pic16f872", 2048, 0, 0, 1, 14, 0, 0, flash, 64, eeprom, 0x08e0},
  {"pic16f873", 4096, 0, 0, 1, 14, 0, 0, flash, 128, eeprom, 0x0960},
  {"pic16f873a", 4096, 0, 0, 1, 14, 0, 0, flash3, 128, eeprom, 0x0e40},
  {"pic16f874", 4096, 0, 0, 1, 14, 0, 0, flash, 128, eeprom, 0x0920},
  {"pic16f874a", 4096, 0, 0, 1, 14, 0, 0, flash3, 128, eeprom, 0x0e60},
  {"pic16f876", 8192, 0, 0, 1, 14, 0, 0, flash, 256, eeprom, 0x09e0},
  {"pic16f876a", 8192, 0, 0, 1, 14, 0, 0, flash3, 256, eeprom, 0x0e00},
  {"pic16f877", 8192, 0, 0, 1, 14, 0, 0, flash, 256, eeprom, 0x09a0},
  {"pic16f877a", 8192, 0, 0, 1, 14, 0, 0, flash3, 256, eeprom, 0x0e20},

  {"pic16f785", 2048, 0, 0, 1, 14, 0, 0, flash4, 256, eeprom, 0x1200},
  {"pic16hv785", 2048, 0, 0, 1, 14, 0, 0, flash4, 256, eeprom, 0x1220},

  {"pic16f818", 1024, 0, 0, 1, 14, 0, 0, flash5, 128, eeprom, 0x04c0},
  {"pic16f819", 2048, 0, 0, 1, 14, 0, 0, flash5, 128, eeprom, 0x04e0},

  {"pic16c923", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, eprom, -1},
  {"pic16c924", 4096, 0, 0, 1, 14, 0, 0, eprom, 0, eprom, -1},

  {"pic16f630", 1024, 1, 0x3000, 1, 14, 0, 0, flash4, 128, eeprom, 0x10c0},
  {"pic16f676", 1024, 1, 0x3000, 1, 14, 0, 0, flash4, 128, eeprom, 0x10e0},

  // 12

  {"pic12c508",   512, 1, 0, 1, 12, 0, 0, eprom,   0, rom, -1},
  {"pic12c508a",  512, 1, 0, 1, 12, 0, 0, eprom,   0, rom, -1},
  {"pic12f508",   512, 1, 0, 1, 12, 0, 0, flash2,  0, rom, -1},
  {"pic12ce518",  512, 0, 0, 1, 12, 0, 0, eprom,  16, eeprom, -1},
  {"pic12c509",  1024, 1, 0, 1, 12, 0, 0, eprom,   0, rom, -1},
  {"pic12c509a", 1024, 0, 0, 1, 12, 0, 0, eprom,   0, rom, -1},
  {"pic12f509",  1024, 1, 0, 1, 12, 0, 0, flash2,  0, rom, -1},
  {"pic12ce519", 1024, 0, 0, 1, 12, 0, 0, eprom,  16, eeprom, -1},
  {"pic12cr509a",1024, 0, 0, 1, 12, 0, 0, rom,     0, rom, -1},
  {"pic12c671",  1024, 1, 0, 1, 14, 0, 0, eprom,   0, rom, 0x0500},
  {"pic12c672",  2048, 1, 0, 1, 14, 0, 0, eprom,   0, rom, -1},
  {"pic12ce673", 1024, 1, 0, 1, 14, 0, 0, eprom,  16, eeprom, -1},
  {"pic12ce674", 2048, 1, 0, 1, 14, 0, 0, eprom,  16, eeprom, -1},

  {"pic16c505",  1024, 1, 0, 1, 12, 0, 0, eprom,   0, rom, -1},

  {"pic12f629",  1024, 1, 0x3000, 1, 14, 0, 0, flash4, 128, eeprom, 0x0f80},
  {"pic12f675",  1024, 1, 0x3000, 1, 14, 0, 0, flash4, 128, eeprom, 0x0fc0},

  // pic12f635: has 2 calibration words in 2008 and 2009, no osccal
  // Some others below have 1 calibration word in 2008.
  {"pic12f635",  1024, 0, 0,      3, 14, 0, 0, flash4, 128, eeprom, 0x0fa0},
  {"pic12f683",  2048, 0, 0,      3, 14, 0, 0, flash4, 256, eeprom, 0x0460},
  {"pic16f631",  1024, 0, 0,      2, 14, 0, 0, flash4, 128, eeprom, 0x1420},
  {"pic16f636",  2048, 0, 0,      3, 14, 0, 0, flash4, 256, eeprom, 0x10a0}, // ?? same
  {"pic16f639",  2048, 0, 0,      3, 14, 0, 0, flash4, 256, eeprom, 0x10a0}, // ?? same
  {"pic16f677",  2048, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x1440},
  {"pic16f684",  2048, 0, 0,      3, 14, 0, 0, flash4, 256, eeprom, 0x1080},
  {"pic16f685",  4096, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x04a0},
  {"pic16f687",  2048, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x1320},
  {"pic16f688",  4096, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x1180},
  {"pic16f689",  4096, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x1340},
  {"pic16f690",  4096, 0, 0,      2, 14, 0, 0, flash4, 256, eeprom, 0x1400},

  // 18f original series
  // Multi-panel writes
  // Write Buffer Size 8
  {"pic18f242",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0480}, // Same as pic18f2439
  {"pic18f248",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0800},
  {"pic18f252",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0400}, // Same as pic18f2539
  {"pic18f258",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0840},
  {"pic18f442",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x04a0}, // Same as pic18f4439
  {"pic18f448",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0820},
  {"pic18f452",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0420}, // Same as pic18f4539
  {"pic18f458",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0860},

  {"pic18f1220",   4 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x07e0},
  {"pic18f2220",   4 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0580},
  {"pic18f4220",   4 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x05a0},
  {"pic18f1320",   8 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x07c0},
  {"pic18f2320",   8 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0500},
  {"pic18f4320",   8 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0520},

  {"pic18f6520",  32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0b20},
  {"pic18f6620",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0660},
  {"pic18f6720", 128 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0620},
  {"pic18f8520",  32 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0b00},
  {"pic18f8620",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0640},
  {"pic18f8720", 128 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0600},

  {"pic18f6585",  48 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0a60},
  {"pic18f8585",  48 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0a20},
  {"pic18f6680",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0a40},
  {"pic18f8680",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0a00},

  {"pic18f6525",  48 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0ae0},
  {"pic18f6621",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0aa0},
  {"pic18f8525",  48 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0ac0},
  {"pic18f8621",  64 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 1024, eeprom, 0x0a80},

  {"pic18f2439",  12 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0480}, // Same as pic18f242
  {"pic18f2539",  24 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0400}, // Same as pic18f252
  {"pic18f4439",  12 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x04a0}, // Same as pic18f442
  {"pic18f4539",  24 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0420}, // Same as pic18f452

  {"pic18f2331",   8 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x08e0},
  {"pic18f2431",  16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x08c0},
  {"pic18f4331",   8 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x08a0},
  {"pic18f4431",  16 * 1024, 0, 0, 14, 16, 8*1024, 8, flash18, 256, eeprom, 0x0880},

  // PIC18F2xx0/2x21/2xx5/4xx0/4x21/4xx5
  // Works without Multi-panel writes    --> panel_size == 0
  // Write Buffer Size differs 8, 32, 64
  {"pic18f2221",   4 * 1024, 0, 0, 14, 16, 0,  8, flash18,  256, eeprom, 0x2160},
  {"pic18f2321",   8 * 1024, 0, 0, 14, 16, 0,  8, flash18,  256, eeprom, 0x2120},
  {"pic18f2410",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x1160},
  {"pic18f2423",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1150}, // Revision high bit == 1
  {"pic18f2420",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1140}, // Must be listed after pic18f2423
  {"pic18f2450",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x2420},
  {"pic18f2455",  24 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1260},
  {"pic18f2458",  24 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x2a60},
  {"pic18f2480",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1ae0},
  {"pic18f2510",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x1120},
  {"pic18f2515",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18,    0, eeprom, 0x0ce0},
  {"pic18f2523",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1110}, // Revision high bit == 1
  {"pic18f2520",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1100}, // Must be listed after pic18f2523
  {"pic18f2525",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0cc0},
  {"pic18f2550",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1240},
  {"pic18f2553",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x2a40},
  {"pic18f2580",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1ac0},
  {"pic18f2585",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0ee0},
  {"pic18f2610",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18,    0, eeprom, 0x0ca0},
  {"pic18f2620",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0c80},
  {"pic18f2680",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0ec0},
  {"pic18f2682",  80 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x2700},
  {"pic18f2685",  96 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x2720},
  {"pic18f4221",   4 * 1024, 0, 0, 14, 16, 0,  8, flash18,  256, eeprom, 0x2140},
  {"pic18f4321",   8 * 1024, 0, 0, 14, 16, 0,  8, flash18,  256, eeprom, 0x2100},
  {"pic18f4410",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x10e0},
  {"pic18f4423",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x10d0}, // Revision high bit == 1
  {"pic18f4420",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x10c0}, // Must be listed after pic18f4423
  {"pic18f4450",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x2400},
  {"pic18f4455",  24 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1220},
  {"pic18f4458",  24 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x2a20},
  {"pic18f4480",  16 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1aa0},
  {"pic18f4510",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,    0, eeprom, 0x10a0},
  {"pic18f4515",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18,    0, eeprom, 0x0c60},
  {"pic18f4523",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1090}, // Revision high bit == 1
  {"pic18f4520",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1080}, // Must be listed after pic18f4523
  {"pic18f4525",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0c40},
  {"pic18f4550",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1200},
  {"pic18f4553",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x2a00},
  {"pic18f4580",  32 * 1024, 0, 0, 14, 16, 0, 32, flash18,  256, eeprom, 0x1a80},
  {"pic18f4585",  48 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0ea0},
  {"pic18f4610",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18,    0, eeprom, 0x0c20},
  {"pic18f4620",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0c00},
  {"pic18f4680",  64 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x0e80},
  {"pic18f4682",  80 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x2740},
  {"pic18f4685",  96 * 1024, 0, 0, 14, 16, 0, 64, flash18, 1024, eeprom, 0x2760},

  // OTP parts.  ID bits are listed as 0x0002 for all of these,
  // I do not know how to handle that.
  {"pic18c242",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},
  {"pic18c252",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},
  {"pic18c442",   16 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},
  {"pic18c452",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},
  {"pic18c658",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},
  {"pic18c858",   32 * 1024, 0, 0, 14, 16, 8*1024, 8, eprom18, 0, rom, -1},

/*

18f series product pages have these chips listed,
but they are not supported for now.

pic18c601	No programmable memory
pic18c801

pic18f6410	No information available, future products
pic18f6490
pic18f8410
pic18f8490

*/

  // dspic - work has started.

  {"dspic30f2010",  4 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0040},
  {"dspic30f2011",  4 * 3072, 0, 0, 16, 24, 0, 0, flash30,    0,    rom, 0x00c0},
  {"dspic30f2012",  4 * 3072, 0, 0, 16, 24, 0, 0, flash30,    0,    rom, 0x00c2},
  {"dspic30f3010",  8 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, -1    },
  {"dspic30f3011",  8 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, -1    },
  {"dspic30f3012",  8 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x00c1},
  {"dspic30f3013",  8 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x00c3},
  {"dspic30f3014",  8 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0140},
  {"dspic30f4011", 16 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0101},
  {"dspic30f4012", 16 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0100},
  {"dspic30f4013", 16 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0141},
  {"dspic30f5011", 22 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0080},
  {"dspic30f5013", 22 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, 0x0081},
  {"dspic30f5015", 22 * 3072, 0, 0, 16, 24, 0, 0, flash30, 1024, eeprom, -1    },
  {"dspic30f6010", 48 * 3072, 0, 0, 16, 24, 0, 0, flash30, 4096, eeprom, 0x0188},
  {"dspic30f6011", 44 * 3072, 0, 0, 16, 24, 0, 0, flash30, 2048, eeprom, 0x0192},
  {"dspic30f6012", 48 * 3072, 0, 0, 16, 24, 0, 0, flash30, 4096, eeprom, 0x0193},
  {"dspic30f6013", 44 * 3072, 0, 0, 16, 24, 0, 0, flash30, 2048, eeprom, 0x0197},
  {"dspic30f6014", 48 * 3072, 0, 0, 16, 24, 0, 0, flash30, 4096, eeprom, 0x0198},

};

static int got_signal = 0;

void
term_handler (int a)
{
  got_signal = 1;
  signal (a, term_handler);
}

int
hexfile::load (const char *name)
{
  ifstream f(name);
  char buf [128];
  int line = 0;
  unsigned long addr32 = 0;
  enum formats format = unknown;
  unsigned long addr;
  int words, check, sum, word, i;
  int e;

  if (!f) {
    e = errno;
    cerr << name << ":unable to load hexfile:" << strerror (e) << endl;
    return EX_NOINPUT;
  }
  if (dev < 0) {
    cerr << "Internal error: no device defined" << endl;
    return EX_SOFTWARE;
  }
  while (f.get (buf, sizeof (buf))) {
    line++;
    char c;
    if (f.get (c) && '\n' != c) {
      cerr << name << ':' << line << ":long input line" << endl;
      return EX_DATAERR;
    }
    int len = strlen (buf);
    while (len && isspace (buf [len-1]))
      len--;
    buf [len] = '\0';
    if (!strcmp (buf, ":00000001FF")) // eof
      return EX_OK;

    if (15 == len
	&& (unknown == format || ihx32 == format)
	&& !strncmp (buf, ":02000004", 9)) {
      // ihx32 address extension
      format = ihx32;
      check = strtol (buf + 13, 0, 16);
      buf [len - 2] = '\0';
      addr32 = strtol (buf + 9, 0, 16);
      sum = 0x02 + 0x04 + addr32 + (addr32 >> 8);
      if ((sum + check) & 0xff) {
      cerr << name << ':' << line << ":checksum mismatch, checksum is 0x"
	   << hex << setw(2) << setfill('0') << check << ", should be 0x"
	   << setw(2) << (-sum & 0xff) << dec << endl;
	return EX_DATAERR;
      }
      addr32 <<= 16;
      continue;
    }

    if (!len)
      continue; // empty line, I do not want to whine about it

    i = len - 1;
    while (i && isxdigit (buf [i]))
      i--;

    if (i || ':' != buf [0] || 1 != (len & 1) || len < 13
	|| '0' != buf [7] || '0' != buf [8]) {
      cerr << name << ':' << line << ":invalid input line." << endl
	   << "Are you sure this is an 8 or 16 bit intel hex file?" << endl;
      return EX_DATAERR;
    }
    check = strtol (buf + len - 2, 0, 16);
    buf [7] = '\0';
    addr = strtol (buf + 3, 0, 16);
    buf [3] = '\0';
    words = strtol (buf + 1, 0, 16);
    if (unknown == format) {
      if (words * 4 + 11 == len)
	format = ihx16;
      else if (words * 2 + 11 == len)
	format = ihx8m;
      else {
	cerr << name << ':' << line <<
	  ":unknown input format, only ihx8m, ihx16, and ihx32 accepted" << endl;
	return EX_DATAERR;
      }
    }
    if (words * (ihx16 == format ? 4 : 2) + 11 != len) {
      cerr << name << ':' << line << ":line length mismatch:"
	   << (ihx16 == format ? "ihx16 "
	       : (ihx8m == format ? "ihx8m " : "ihx32 "))
	   << words * (ihx16 == format ? 4 : 2) + 11 << " != " << len << endl;
      return EX_DATAERR;
    }
    sum = words + addr + (addr >> 8);
    addr += addr32;

    if (14 == deviceinfo [dev].prog_bits) {
      if (ihx16 != format) {
	if ((words & 1) || (addr & 1)) {
	  cerr << name << ':' << line
	       << ":odd address or number of words." << endl;
	  return EX_DATAERR;
	}
	words /= 2;
	addr /= 2;
      }

      while (words--) {
	buf [13 + words * 4] = '\0';
	word = strtol (buf + 9 + words * 4, 0, 16);
	if (ihx16 != format)
	  word = (word & 0xff) << 8 | word >> 8;
	sum += word + (word >> 8);

	unsigned long a = addr + words;
	if (a >= 0x2007 && a < 0x2007 + deviceinfo [dev].conf_size)
	  conf [a - 0x2007] = word & 0x3fff;
	else if (a >= 0x2100
		 && a < 0x2100 + deviceinfo [dev].data_size)
	  data [a - 0x2100] = word & 0x00ff;
	else if (a >= 0x2000 && a < 0x2004)
	  ids [a - 0x2000] = word & 0x3fff;
	else if (a < deviceinfo [dev].prog_size)
	  pgm [a] = word & 0x3fff;
	else {
	  cerr << name << ':' << line << ":invalid address 0x" << hex
	       << setw(4) << setfill('0') << addr << dec
	       << ", possibly not hex file for correct pic type?"
	       << endl;
	  return EX_DATAERR;
	}
      }
    } else if (12 == deviceinfo [dev].prog_bits) {
      if (ihx16 != format) {
	if ((words & 1) || (addr & 1)) {
	  cerr << name << ':' << line
	       << ":odd address or number of words." << endl;
	  return EX_DATAERR;
	}
	words /= 2;
	addr /= 2;
      }

      while (words--) {
	buf [13 + words * 4] = '\0';
	word = strtol (buf + 9 + words * 4, 0, 16);
	if (ihx16 != format)
	  word = (word & 0xff) << 8 | word >> 8;
	sum += word + (word >> 8);

	unsigned long a = addr + words;
	if (a >= 0xfff && a < 0xfff + deviceinfo [dev].conf_size)
	  conf [a - 0xfff] = word & 0xfff;
	else if (a >= deviceinfo [dev].prog_size && a < deviceinfo [dev].prog_size + 5)
	  // Includes 4 id words and a backup osccal word
	  ids [a - deviceinfo [dev].prog_size] = word & 0xfff;
	else if (a < deviceinfo [dev].prog_size)
	  pgm [a] = word & 0xfff;
	else {
	  cerr << name << ':' << line << ":invalid address 0x" << hex
	       << setw(4) << setfill('0') << addr << dec
	       << ", possibly not hex file for correct pic type?"
	       << endl;
	  return EX_DATAERR;
	}
      }
    } else {
      // pic18, dspic30
      if (ihx16 == format) {
	words *= 2;
	addr *= 2;
      }

      while (words--) {
	buf [11 + words * 2] = '\0';
	word = strtol (buf + 9 + words * 2, 0, 16);
	sum += word;
	unsigned long a = addr + words;
	if (ihx16 == format)
	  a ^= 1;

	if ((flash18 == deviceinfo [dev].prog_type)
	    && a >= 0x300000 && a < 0x300000 + deviceinfo [dev].conf_size)
	  conf [a - 0x300000] = word;
	else if ((flash18 == deviceinfo [dev].prog_type)
		 && a >= 0xf00000
		 && a < 0xf00000 + deviceinfo [dev].data_size)
	  data [a - 0xf00000] = word;
	else if ((flash18 == deviceinfo [dev].prog_type)
		 && a >= 0x200000 && a < 0x200008)
	  ids [a - 0x200000] = word;
	else if (flash30 == deviceinfo [dev].prog_type
	    && a >= 0xf80000 && a < 0xf80000 + deviceinfo [dev].conf_size)
	  conf [a - 0xf80000] = word;
	else if (flash30 == deviceinfo [dev].prog_type
		 && a >= 0x800000 - deviceinfo [dev].data_size
		 && a < 0x800000)
	  data [a - (0x800000 - deviceinfo [dev].data_size)] = word;
	else if (a < deviceinfo [dev].prog_size)
	  pgm [a] = word;
	else {
	  cerr << name << ':' << line << ":invalid address 0x" << hex
	       << setw(6) << setfill('0') << a << dec
	       << ", possibly not hex file for correct pic type?"
	       << endl;
	  return EX_DATAERR;
	}
      }
    }

    if ((sum + check) & 0xff) {
      cerr << name << ':' << line << ":checksum mismatch, checksum is 0x"
	   << hex << setw(2) << setfill('0') << check << ", should be 0x"
	   << setw(2) << (-sum & 0xff) << dec << endl;
      return EX_DATAERR;
    }
  }
  e = errno;
  if (!f.eof ()) {
    cerr << name << ':' << line << ':' << strerror (e) << ":" << endl;
    return EX_IOERR;
  }
  cerr << name << ':' << line << ":warning:unexpected eof" << endl;
  return EX_OK;
}

void
hexfile::save_line (ofstream& f, const short *pgmp, unsigned long begin, unsigned long len, enum hexfile::formats format) const
{
  unsigned long p_begin, p_len, i;
  int sum;
  // f << "#saving line " << setw(6) << begin << " len " << setw(2) << len << endl;

  if (14 == deviceinfo [dev].prog_bits) {
    if (ihx16 == format) {
      p_begin = begin;
      p_len = len;
    } else {
      p_begin = begin * 2;
      p_len = len * 2;
    }

    f << ':' << setw (2) << p_len << setw (4) << (p_begin & 0xffff) << "00";

    sum = p_len + p_begin + (p_begin >> 8);
    for (i = 0; i != len; i++) {
      int word = pgmp [i];
      if (ihx16 != format)
	word = (word & 0xff) << 8 | (word & 0xff00) >> 8;
      f << setw (4) << word;
      sum += word + (word >> 8);
    }

    f << setw (2) << (-sum & 0xff) << endl;
  } else {
    // pic18, dspic30

    f << ':' << setw (2) << len << setw (4) << (begin & 0xffff) << "00";

    sum = len + begin + (begin >> 8);
    for (i = 0; i != len; i++) {
      int word = pgmp [i];
      f << setw (2) << word;
      sum += word;
    }

    f << setw (2) << (-sum & 0xff) << endl;
  }
}

int
hexfile::save_region (ofstream& f, const short *pgmp, unsigned long addr0, unsigned long len0, enum hexfile::formats format, bool skip_ones, unsigned long &addr32) const
{
  unsigned long len;
  unsigned long addr = addr0;
  short skip_value;
  unsigned long rowlen;

  if (!skip_ones)
    skip_value = -1;
  else if (16 <= deviceinfo [dev].prog_bits
	   || (addr0 >= 0x2100 && addr0 < 0x2100 + deviceinfo [dev].data_size))
    skip_value = 0xff;
  else if (12 == deviceinfo [dev].prog_bits)
    skip_value = 0xfff;
  else
    skip_value = 0x3fff;
  if (16 <= deviceinfo [dev].prog_bits)
    rowlen = 16;
  else
    rowlen = 8;

  while (addr < addr0 + len0) {

    // Skip undefined locations or ignored values
    if (skip_value == pgmp [addr-addr0]
	|| -1 == pgmp [addr-addr0]) {
      addr++;
      continue;
    }

    len = 1;
    while (len < rowlen
	   && (addr + len) % rowlen
	   && addr + len < addr0 + len0
	   && skip_value != pgmp [addr - addr0 + len]
	   && -1 != pgmp [addr - addr0 + len])
      len++;
    if (ihx32 == format) {
      if (addr32 != (addr & 0xffff0000)) {
	addr32 = addr & 0xffff0000;
	int sum = 0x02 + 0x04 + (addr32 >> 16) + (addr32 >> 24);
	f << ":02000004"
	  << setw(4) << (addr32 >> 16)
	  << setw(2) << (-sum & 0xff) << endl;
      }
    } else {
      // not ihx32
      if (addr > 0xffff) {
	cerr << "ihx32 format needed for this data" << endl;
	return EX_USAGE;
      }
    }
    save_line (f, pgmp + (addr - addr0), addr, len, format);
    addr += len;
  }
  return EX_OK;
}

int
hexfile::save (const char *name, enum hexfile::formats format, bool skip_ones) const
{
  ofstream f (name);
  int e;
  unsigned long addr32 = 1; // flag that addr32 line must be output on first line

  if (unknown == format) {
    if (16 <= deviceinfo [dev].prog_bits)
      format = ihx32;
    else
      format = ihx16;
  }

  if (!f) {
    e = errno;
    cerr << name << ":unable to open save file:" << strerror (e) << endl;
    return EX_IOERR;
  }
  f << hex << setfill ('0') << setiosflags (ios::uppercase);

  e = save_region (f, pgm, 0, deviceinfo [dev].prog_size, format, skip_ones, addr32);
  if (EX_OK != e)
    return e;
  if (24 == deviceinfo [dev].prog_bits)
    // No ids for dspic30
    e = EX_OK;
  else if (16 == deviceinfo [dev].prog_bits)
    e = save_region (f, ids, 0x200000, 8, format, skip_ones, addr32);
  else if (12 == deviceinfo [dev].prog_bits)
    // Save the backup osccal word to the ids too!
    e = save_region (f, ids, deviceinfo [dev].prog_size, 5, format, skip_ones, addr32);
  else
    e = save_region (f, ids, 0x2000, 4, format, skip_ones, addr32);
  if (EX_OK != e)
    return e;
  if (24 == deviceinfo [dev].prog_bits)
    e = save_region (f, conf, 0xf80000, deviceinfo [dev].conf_size, format, skip_ones, addr32);
  else if (16 == deviceinfo [dev].prog_bits)
    e = save_region (f, conf, 0x300000, deviceinfo [dev].conf_size, format, skip_ones, addr32);
  else if (12 == deviceinfo [dev].prog_bits)
    e = save_region (f, conf, 0xfff, deviceinfo [dev].conf_size, format, skip_ones, addr32);
  else
    e = save_region (f, conf, 0x2007, deviceinfo [dev].conf_size, format, skip_ones, addr32);
  if (EX_OK != e)
    return e;
  if (24 == deviceinfo [dev].prog_bits)
    e = save_region (f, data, 0x800000 - deviceinfo [dev].data_size, deviceinfo [dev].data_size, format, skip_ones, addr32);
  else if (16 == deviceinfo [dev].prog_bits)
    e = save_region (f, data, 0xf00000, deviceinfo [dev].data_size, format, skip_ones, addr32);
  else if (12 == deviceinfo [dev].prog_bits)
    e = EX_OK; // No 12 bit data areas implemented
  else
    e = save_region (f, data, 0x2100, deviceinfo [dev].data_size, format, skip_ones, addr32);
  if (EX_OK != e)
    return e;
  f << ":00000001FF" << endl;
  return EX_OK;
}

// Verify 8 bytes at the given address in all panels
// available in the device.
bool
hexfile::verify18 (picport& pic, const short *pgmp, unsigned long addr, unsigned long len, unsigned long panel_size, bool verbose) const
{
  // If we are not handling program memory, below 0x200000, we obviously
  // loop only once.  Id and config memory must be written in single
  // panel mode.
  for (unsigned long panel = 0;
       addr < 0x200000 ?
	 panel + addr < deviceinfo [dev].prog_size
	 : 0 == panel;
       panel += panel_size) {
    unsigned long i, j;
    for (i = 0; i < len; ++i)
      if (-1 != pgmp [panel + i])
	break;
    if (len == i)
      continue;

    // Find the last byte to verify
    for (j = len - 1; -1 == pgmp [panel + j]; --j)
      ;
    pic.setaddress (panel + addr + i);
    for (; i <= j; ++i) {
      int value = pic.command18 (picport::tread_inc);
      if (value != pgmp [panel + i]
	  && -1 != pgmp [panel + i]) {
	if (verbose) {
	  cerr << pic.port() << ":" << "0x" << hex << setfill('0') << setw(6)
	       << panel + addr + i
	       << ": panel " << (panel >> 13)
	       << ", block " << setw(4) << addr
	       << ", byte " << i << ": verification failed, read 0x"
	       << setw(2) << value << ", should be 0x"
	       << setw(2) << pgmp [panel + i] << dec << endl;
	}
	return false;
      }
    }
  } // for panels
  return true;
}

// PIC18 parts code programming.  Do multi-panel programming.  It is
// assumed that correct mode is set before calling this method.  Also
// address parameter must be < panel size, ie. 8kB for original 18f parts.
// Call with panel_size == prog_size if multipanel writes not used.
int
hexfile::program18 (picport& pic, const short *pgmp, unsigned long addr, unsigned long len, unsigned long panel_size) const
{
  if (verify18 (pic, pgmp, addr, len, panel_size, false))
    return 0;

  unsigned long count = 0;
  // Loop only once for single panel writes
  for (unsigned long panel = 0;
       addr < 0x200000 ?
	 panel + addr < deviceinfo [dev].prog_size
	 : 0 == panel;
       panel += panel_size) {
    pic.setaddress (panel + addr);
    for (unsigned long i = 0; i < len; i += 2) {
      picport::commands18 comm;
      if (i+2 < len)
	// Normal bytes of 8 byte block
	comm = picport::twrite_inc2;
      else if (addr < 0x200000
	       && panel + panel_size + addr < deviceinfo [dev].prog_size)
	// Last bytes do not need post increment
	comm = picport::twrite;
      else
	// Last bytes of last write need programming command
	comm = picport::twrite_prog;

      // For locations that do not need programming, pgm[] has value
      // -1.  With &0xff operation, that becomes 0xff.  This is fine,
      // as it is the erased state of flash memory, and no bits are
      // programmed to 0 state.
      pic.command18 (comm,
		     (0xff & pgmp [panel + i])
		     | ((0xff & pgmp [panel + i + 1]) << 8));
      count += 2;
    } // for bytes (words)
  } // for panels
  // The programming command was issued above, and now we need the NOP
  // command with programming delay.
  pic.command18 (picport::nop_prog, 0);

  if (!verify18 (pic, pgmp, addr, len, panel_size, true))
    return -EX_IOERR;
  return count;
}

/*
Locations are verified before and after programming, so unnecessary
programming is avoided and errors are detected.

PIC18 parts can only be programmed 8 locations at a time.

*/

// This is the 14 bit programming

int
hexfile::program_location (picport& pic, unsigned long addr, short word, bool isdata) const
{
  int retval;


  if (-1 == word
      || (retval = pic.command (isdata ? picport::data_from_data	: picport::data_from_prog ,0,1)) == word)
    return NOT_PROGRAMMED;

  if (-1 == retval) {
    cerr << pic.port() << ':' << hex << setfill ('0') << setw (4) << addr
	 << dec << ":unable to read pic while programming" << endl;
    return EX_IOERR;
  }
  pic.command (isdata ? picport::data_for_data : picport::data_for_prog,
	       word);

  switch (deviceinfo [dev].prog_type) {
  case flash2: // pic16f77
    pic.command (picport::beg_prog);
    pic.delay (1000); // tprog = 1ms
    pic.command (picport::end_prog);
    break;
  case prom: // pic16c
  case eprom:
    pic.command (picport::beg_prog);
    pic.delay(10000); // Maximum needed programming time = 100 * 100 µs
    pic.command (picport::end_prog);
    break;
  case flash3: // pic16f876a
    if (0x2007 == addr) {
      pic.command (picport::beg_prog);
      pic.delay (10000); // tprog2 = 8ms
    } else {
      pic.command (picport::beg_prog_only);
      pic.delay (1000); // tprog = 1ms
      pic.command (picport::end_prog_only);
    }
    break;
  case flash5: // pic16f88
    pic.command (picport::beg_prog_only);
    pic.delay (1000);
    pic.command (picport::end_prog_only);
    break;
  case flash4:
    pic.command (picport::beg_prog);
    pic.delay (6*1000);//tprog max = 6ms
    break;
  case flash:
  case eeprom:
    pic.command (picport::beg_prog);
    pic.delay (10000);
    break;
  default:
    cerr << "Internal error: unknown memory type: "
	 << int(deviceinfo [dev].prog_type) << endl;
    return EX_SOFTWARE;
  }

  // verify, but do not verify fuses if Code Protect bit is cleared!

  int read_val = pic.command (isdata ? picport::data_from_data : picport::data_from_prog,0,1);
  if (word != read_val)
  if (word != (read_val = pic.command (isdata ? picport::data_from_data : picport::data_from_prog,0,1) )) {
    cerr << pic.port() << ':' << hex << setw (4) << setfill ('0') << addr
	 << ": programmed=" << setw (4) << setfill ('0') << word
	 << ", read=" << setw (4) << setfill ('0') << read_val
	 << dec << ":unable to verify pic while programming." << endl;
    if (pic.address () != 0x2007) {
      cerr << "Is code protection enabled, or does the chip need to be " << endl
	   << "erased completely before programming?" << endl
	   << "Use --erase option to disable code protection." << endl;
      return EX_IOERR;
    } else {
      cerr << "This is the configuration word, which often has hardwired" << endl
	   << "bits and therefore does not verify.  It also has the code" << endl
	   << "protection bits, and if they were programmed to enabled" << endl
	   << "state, verification fails.  Therefore this error is ignored." << endl;
    }
  }
  return EX_OK;
}

typedef void (*sig_type)(int);

void hexfile::reset_code_protection (picport& pic)
{
  switch (deviceinfo [dev].prog_type) {
  case flash30: // dspic30f
    // Step 1
    pic.command30 (picport::SIX, 0); // NOP
    pic.command30 (picport::SIX, 0); // NOP
    pic.command30 (picport::SIX, 0x040100); // GOTO 0x100
    pic.command30 (picport::SIX, 0); // NOP
    // Steps 2-7 only concern dspic30f601[0-4] mask 0 versions
    // What does that mean??
    if (0) {
      // Step 2
      pic.command30 (picport::SIX, 0x24008A); // 
      pic.command30 (picport::SIX, 0x883B0A); // 
      // Step 3
      pic.command30 (picport::SIX, 0x200F80); // 
      pic.command30 (picport::SIX, 0x880190); // 
      pic.command30 (picport::SIX, 0x200067); // 
      pic.command30 (picport::SIX, 0xEB0300); // 
      // Step 4
      pic.command30 (picport::SIX, 0x231010); // 
      // Step 5
      pic.command30 (picport::SIX, 0xBB0B96); // 
      // Step 6
      pic.command30 (picport::SIX, 0x200558); // MOV #0x55, W8
      pic.command30 (picport::SIX, 0x883B38); // MOV W8, NVMKEY
      pic.command30 (picport::SIX, 0x200AA9); // MOV #0xAA, W9
      pic.command30 (picport::SIX, 0x883B39); // MOV W9, NVMKEY
      // Step 7
      pic.command30 (picport::SIX, 0xA8E761); // BSET NVMCON, #WR
      pic.command30 (picport::SIX, 0); // NOP
      pic.command30 (picport::SIX, 0); // NOP
      pic.delay (2000);
      pic.command30 (picport::SIX, 0xA9E761); // BCLR NVMCON, #WR
      pic.command30 (picport::SIX, 0); // NOP
      pic.command30 (picport::SIX, 0); // NOP
    }
    // Step 8
    pic.command30 (picport::SIX, 0x2407FA); // MOV #0x407F, W10
    pic.command30 (picport::SIX, 0x883B0A); // MOV W10, NVMCON
    // Step 9
    pic.command30 (picport::SIX, 0x200558); // MOV #0x55, W8
    pic.command30 (picport::SIX, 0x883B38); // MOV W8, NVMKEY
    pic.command30 (picport::SIX, 0x200AA9); // MOV #0xAA, W9
    pic.command30 (picport::SIX, 0x883B39); // MOV W9, NVMKEY
    // Step 10
    pic.command30 (picport::SIX, 0xA8E761); // BSET NVMCON, #WR
    pic.command30 (picport::SIX, 0); // NOP
    pic.command30 (picport::SIX, 0); // NOP
    pic.delay (2000);
    pic.command30 (picport::SIX, 0xA9E761); // BCLR NVMCON, #WR
    pic.command30 (picport::SIX, 0); // NOP
    pic.command30 (picport::SIX, 0); // NOP
    break;
  case flash18: // pic18f
    // new series has different erase algorithm
    if (!deviceinfo [dev].panel_size) {
      pic.setaddress (0x3c0005);
      pic.command18 (picport::twrite, 0x0F0F); // Write 0F0F to 3c0005h
      pic.setaddress (0x3c0004);
      pic.command18 (picport::twrite, 0x8787); // Write 8787 to 3c0004h
      pic.command18 (picport::instr, 0); // NOP
      pic.command18 (picport::nop_erase, 0); // NOP, delay
      break;
    }
    // fallthrough for original pic18f series
  case eprom18: // pic18c - unsupported
    pic.setaddress (0x3c0004);
    pic.command18 (picport::twrite, 0x0080); // Write 80h to 3c0004h
    pic.command18 (picport::instr, 0); // NOP
    pic.command18 (picport::nop_erase, 0); // NOP, delay
    break;
  case flash2: // pic16f77, pic12f
    // pic12f508, pic12f509 do not have load_conf, but full erase
    // needs another magic address thing.  See 41227D.
    if (deviceinfo [dev].prog_bits == 12) {
      while (pic.address () != deviceinfo [dev].prog_size)
	pic.command (picport::inc_addr, addr_max);
    } else
      pic.command (picport::load_conf, 0x3fff);
    pic.command (picport::erase_prog);
    break;
  case flash3: // pic16f876a
  case flash5: // pic16f88
    pic.command (picport::load_conf, 0x3fff);
    pic.command (picport::chip_erase);
    break;
  case flash4: // pic16f628a
    pic.command (picport::load_conf, 0x3fff);
    pic.command (picport::erase_prog);
    pic.delay (50000);
    pic.command (picport::erase_data);
    break;
  default: // eeprom, flash
    pic.command (picport::load_conf, 0x3fff);
    for (int i = 0; i < 7; i++)
      pic.command (picport::inc_addr);
    assert (0x2007 == pic.address ());
    pic.command (picport::command1);
    pic.command (picport::command7);
    pic.command (picport::beg_prog);
    pic.delay (20000);
    pic.command (picport::command1);
    pic.command (picport::command7);

    // On my pic16f628, the above leaves data memory not erased,
    // if the code protection was not on.
    pic.command (picport::data_for_data, 0x3fff);
    pic.command (picport::erase_data);
    pic.command (picport::beg_prog);
  }

  pic.delay (50000);
  pic.reset (deviceinfo [dev].prog_bits == 12 ? 0xfff : 0);
}

int
hexfile::program (picport &pic, bool reset, bool nopreserve)
{
  int retval;

  sig_type save_t, save_q, save_i;
  save_t = signal (SIGTERM, term_handler);
  save_q = signal (SIGQUIT, term_handler);
  save_i = signal (SIGINT, term_handler);

  cout << "Device " << deviceinfo [dev].name
       << ", program memory: " << deviceinfo [dev].prog_size
       << ", data memory: " << deviceinfo [dev].data_size << "."
       << endl;
  if ((rom == deviceinfo [dev].data_type || 0 == deviceinfo [dev].data_size)
      && (rom == deviceinfo [dev].prog_type || 0 == deviceinfo [dev].prog_size)) {
    cerr << "This type of device has no programmable memory." << endl;
    return EX_USAGE;
  }

  // As PIC18 parts never have prog_preserved, it does not have to
  // be tested here.

  if (deviceinfo [dev].prog_preserved && !nopreserve) {
    if (!reset) {
      // Chip is not erased, so we just need to avoid programming
      // the reserved locations.
      for (unsigned long addr = deviceinfo [dev].prog_size - deviceinfo [dev].prog_preserved;
	   addr != deviceinfo [dev].prog_size;
	   ++addr) {
	cout << "Calibration word at "
	     << hex << setfill('0') << setw(4) << addr << dec
	     << " not programmed";
	if (-1 != pgm [addr]) {
	  pgm [addr] = -1;
	  cout << " (value in input file ignored)";
	}
	cout << endl;
      }
      if (deviceinfo [dev].prog_bits == 12) { // 12f508, 12f509
	// Backup osccal word.
	unsigned long addr = deviceinfo [dev].prog_size + 4;
	cout << "Calibration word at "
	     << hex << setfill('0') << setw(4) << addr << dec
	     << " not programmed";
	if (-1 != ids [addr - deviceinfo [dev].prog_size]) {
	  ids [addr - deviceinfo [dev].prog_size] = -1;
	  cout << " (value in input file ignored)";
	}
	cout << endl;
      }
    } else {
      // We are erasing the chip. Read OSCCAL and other reserved
      // words off the device first.

      // Make sure we are in the program memory and below OSCCAL
      // address
      if (pic.address () > deviceinfo [dev].prog_size - deviceinfo [dev].prog_preserved) {
	if (deviceinfo [dev].prog_bits == 12) {
	  // 12f508/509 reset to configuration word.
	  if (pic.address () != 0xfff)
	    pic.reset (0xfff);
	  // We need to step them to address 0.
	  if (pic.address () == 0xfff && pic.address () > deviceinfo [dev].prog_size - deviceinfo [dev].prog_preserved)
	    pic.command (picport::inc_addr, addr_max);
	} else {
	  if (pic.address ())
	    pic.reset (0);
	}
      }

      for (;;) {
	if (pic.address () >= deviceinfo [dev].prog_size - deviceinfo [dev].prog_preserved
	    && pic.address () < deviceinfo [dev].prog_size) {

		int value = pic.command (picport::data_from_prog,0,1);
	  if (-1 == value) {
	    cerr << pic.port() << ':'
		 << hex << setfill ('0') << setw (4) << pic.address () << dec
		 << ":unable to read pic calibration words" << endl;
	    return EX_IOERR;
	  }
	  cout << "Calibration word at 0x"
	       << hex << setfill('0') << setw(4) << pic.address()
	       << " preserved as 0x"
	       << setfill('0') << setw(4) << value << dec;
	  if (-1 != pgm [pic.address ()])
	    cout << " (value in input file ignored)";
	  cout << endl;
	  pgm [pic.address ()] = value;
	}
	if (pic.address () + 1 == deviceinfo [dev].prog_size)
	  break;
	pic.command (picport::inc_addr, addr_max);
      } // for
      if (deviceinfo [dev].prog_bits == 12) { // 12f508, 12f509
	// Backup osccal word.
	for (;;) {
	  if (pic.address () == deviceinfo [dev].prog_size + 4) {

		  int value = pic.command (picport::data_from_prog,0,1);
	    if (-1 == value) {
	      cerr << pic.port() << ':'
		   << hex << setfill ('0') << setw (4) << pic.address () << dec
		   << ":unable to read pic calibration words" << endl;
	      return EX_IOERR;
	    }
	    cout << "Calibration word at 0x"
		 << hex << setfill('0') << setw(4) << pic.address()
		 << " preserved as 0x"
		 << setfill('0') << setw(4) << value << dec;
	    if (-1 != ids [pic.address () - deviceinfo [dev].prog_size])
	      cout << " (value in input file ignored)";
	    cout << endl;
	    ids [pic.address () - deviceinfo [dev].prog_size] = value;
	    break; // Only 1 backup word
	  }
	  pic.command (picport::inc_addr, addr_max);
	}
      } // if 12f508 12f509
    }
  }

  // As PIC18 parts never have config_mask, it does not have to be
  // tested here.

  if (deviceinfo [dev].config_mask && !nopreserve) {
    if (reset || -1 != conf [0]) {
      pic.command (picport::load_conf, 0);
      for (int i = 0; i < 7; ++i)
	pic.command (picport::inc_addr);
      assert (0x2007 == pic.address());
      int value;

      if (-1 == (value = pic.command (picport::data_from_prog,0,1))) {
	cerr << pic.port() << ':' << hex << setfill ('0') << setw (4) << pic.address () << dec
	     << ":unable to read pic calibration bits" << endl;
	return EX_IOERR;
      }
      value &= deviceinfo [dev].config_mask;
      cout << "Calibration bits 0x"
	   << hex << setfill('0') << setw(4) << deviceinfo [dev].config_mask
	   << " in configuration word preserved as 0x"
	   << setfill('0') << setw(4) << value << dec << endl;
      if (-1 == conf [0])
	conf [0] = 0x3fff;
      conf [0] = (conf [0] & ~deviceinfo [dev].config_mask) | value;
    } else {
      cout << "Calibration bits 0x"
	   << hex << setfill('0') << setw(4) << deviceinfo [dev].config_mask
	   << dec << " in configuration word not programmed" << endl;
    }
  }

  if (reset) {
    if (rom == deviceinfo [dev].prog_type
	|| prom == deviceinfo [dev].prog_type
	|| eprom == deviceinfo [dev].prog_type) {
      cerr << "I do not know how to erase this device." << endl;
      return EX_UNAVAILABLE;
    }
    reset_code_protection (pic);
    cout << "Erased and removed code protection." << endl;
  }
  if (deviceinfo [dev].prog_bits == 12) {
    // 12f508/509 reset to configuration word.
    if (pic.address () != 0xfff && pic.address () != 0)
      pic.reset (0xfff);
    // We need to step them to address 0.
    if (pic.address () == 0xfff)
      pic.command (picport::inc_addr, addr_max);
  } else {
    if (pic.address ())
      pic.reset (0);
  }

  int count;
  if (rom == deviceinfo [dev].prog_type || 0 == deviceinfo [dev].prog_size) {
    cout << "Skipped burning program memory," << endl;
  } else {
    cout << "Burning program memory," << flush;
    count = 0;

    if (16 == deviceinfo [dev].prog_bits) {
      // Enable access to config memory
      pic.command18 (picport::instr, 0x8ea6);
      pic.command18 (picport::instr, 0x8ca6);
      pic.command18 (picport::instr, 0x86A6);
      if (deviceinfo [dev].panel_size) {
	//  Configure device for multi-panel writes.
	pic.setaddress (0x3c0006);
	pic.command18 (picport::twrite, 0x0040);
      } else {
	// Disable multi-panel writes.
	pic.setaddress (0x3c0006);
	pic.command18 (picport::twrite, 0x0000);
      }
      // Enable access to program memory.
      pic.command18 (picport::instr, 0x8ea6);
      pic.command18 (picport::instr, 0x9ca6);
      pic.setaddress (0);
    }
    unsigned long addr = pic.address ();
    unsigned long panel_size = deviceinfo [dev].panel_size;
    if (!panel_size || panel_size > deviceinfo [dev].prog_size)
      panel_size = deviceinfo [dev].prog_size;
    while (addr < panel_size) {
      if (16 == deviceinfo [dev].prog_bits) {
	int len = deviceinfo [dev].write_size;
	retval = program18 (pic, pgm + addr, addr, len, panel_size);
	if (retval >= 0)
	  count += retval;
	else
	  return -retval;
	addr += len;
      } else { // 12 bit and 14 bit
	retval = program_location (pic, addr, pgm [addr], false);
	if (EX_OK == retval)
	  ++count;
	else if (NOT_PROGRAMMED != retval)
	  return retval;
	pic.command (picport::inc_addr, addr_max);
	addr = pic.address ();
      }
      if (got_signal) {
	cerr << "Exiting." << endl;
	return EX_UNAVAILABLE;
      }
    }
    cout << " " << count << " location" << (count != 1 ? "s" : "") << "," << endl;
  }

  if (rom == deviceinfo [dev].data_type || 0 == deviceinfo [dev].data_size) {
    cout << "skipped burning data memory," << endl;
  } else {
    cout << "burning data memory," << flush;
    count = 0;
    if (16 == deviceinfo [dev].prog_bits) {
      // Direct access to data EEPROM.
      pic.command18 (picport::instr, 0x9ea6);
      pic.command18 (picport::instr, 0x9ca6);
    }
    for (unsigned long addr = 0;
	 addr < deviceinfo [dev].data_size;
	 ++addr) {
      if (16 == deviceinfo [dev].prog_bits) {
	if (-1 == data [addr])
	  retval = NOT_PROGRAMMED;
	else {
	  // Set the data EEPROM address pointer.
	  pic.command18 (picport::instr, 0x0e00 | (addr & 0x00ff));
	  pic.command18 (picport::instr, 0x6ea9);
	  pic.command18 (picport::instr, 0x0e00 | ((addr & 0xff00) >> 8));
	  pic.command18 (picport::instr, 0x6eaa);
	  // Initiate a memory read.
	  pic.command18 (picport::instr, 0x80a6);
	  // Load data into the serial data holding register.
	  pic.command18 (picport::instr, 0x50a8);
	  pic.command18 (picport::instr, 0x6ef5);
	  pic.command18 (picport::instr, 0x0000);

	  int word = pic.command18 (picport::shift_out);
	  if (word == data [addr])
	    retval = NOT_PROGRAMMED;
	  else {
	    // Load the data to be written.
	    pic.command18 (picport::instr, 0x0e00 | (data [addr] & 0x00ff));
	    pic.command18 (picport::instr, 0x6ea8);
	    // Enable memory writes.
	    pic.command18 (picport::instr, 0x84a6);
	    // Perform required sequence.
	    pic.command18 (picport::instr, 0x0e55);
	    pic.command18 (picport::instr, 0x6ea7);
	    pic.command18 (picport::instr, 0x0eaa);
	    pic.command18 (picport::instr, 0x6ea7);
	    // Initiate write.
	    pic.command18 (picport::instr, 0x82a6);
	    // Poll EECON1 WR bit, repeat until the bit is clear.
	    do {
	      if (got_signal) {
		cerr << "Exiting." << endl;
		return EX_UNAVAILABLE;
	      }
	      pic.command18 (picport::instr, 0x50a6);
	      pic.command18 (picport::instr, 0x6ef5);
	      pic.command18 (picport::instr, 0x0000);

	      word = pic.command18 (picport::shift_out);
	    } while (word & 2);
	    // Disable writes.
	    pic.command18 (picport::instr, 0x94a6);
	    // Read to verify
	    // Initiate a memory read.
	    pic.command18 (picport::instr, 0x80a6);
	    // Load data into the serial data holding register.
	    pic.command18 (picport::instr, 0x50a8);
	    pic.command18 (picport::instr, 0x6ef5);
	    pic.command18 (picport::instr, 0x0000);

	    word = pic.command18 (picport::shift_out);
	    if (word == data [addr])
	      retval = EX_OK;
	    else {
	      cerr << pic.port() << ':' << hex << setw (6) << setfill ('0') << 0xf00000 + addr
		   << ": programmed=" << setw (2) << setfill ('0') << data [addr]
		   << ", read=" << setw (2) << setfill ('0') << word
		   << dec << ":unable to verify pic data eeprom while programming." << endl;
	      retval = EX_IOERR;
	    }
	  }
	}
      } else if (12 == deviceinfo [dev].prog_bits) {
	cerr << "12 bit microcontroller data memory unimplemented." << endl;
	retval = NOT_PROGRAMMED;
	got_signal = 1;
      } else { // 14 bit
	retval = program_location (pic, 0x2100 + addr, data [addr], true);
	pic.command (picport::inc_addr);
      }
      if (EX_OK == retval)
	++count;
      else if (NOT_PROGRAMMED != retval)
	return retval;
      if (got_signal) {
	cerr << "Exiting." << endl;
	return EX_UNAVAILABLE;
      }
    }
    cout << " " << count << " location" << (count != 1 ? "s" : "") << "," << endl;
  }

  cout << "burning id words," << flush;
  count = 0;
  if (16 == deviceinfo [dev].prog_bits) {
    // Enable access to config memory
    pic.command18 (picport::instr, 0x8ea6);
    pic.command18 (picport::instr, 0x8ca6);
    pic.command18 (picport::instr, 0x86A6);
    // Disable multi-panel writes.
    pic.setaddress (0x3c0006);
    pic.command18 (picport::twrite, 0x0000);
    // Enable access to code memory.
    pic.command18 (picport::instr, 0x8ea6);
    pic.command18 (picport::instr, 0x9ca6);
    retval = program18 (pic, ids, 0x200000, 8, 8);
    if (retval >= 0)
      count += retval;
    else
      return -retval;
  } else if (12 == deviceinfo [dev].prog_bits) {
    while (pic.address () != deviceinfo [dev].prog_size)
      pic.command (picport::inc_addr, addr_max);
    // id words and backup osccal word
    while (pic.address () < deviceinfo [dev].prog_size + 5) {
      retval = program_location (pic, pic.address (), ids [pic.address () - deviceinfo [dev].prog_size], false);
      if (EX_OK == retval)
	++count;
      else if (NOT_PROGRAMMED != retval)
	return retval;
      pic.command (picport::inc_addr, addr_max);
      if (got_signal) {
	cerr << "Exiting." << endl;
	return EX_UNAVAILABLE;
      }
    }
  } else { // 14 bit
    pic.command (picport::load_conf, 0x3fff); // dummy value
    while (pic.address () < 0x2004) {
      retval = program_location (pic, pic.address (), ids [pic.address () - 0x2000], false);
      if (EX_OK == retval)
	++count;
      else if (NOT_PROGRAMMED != retval)
	return retval;
      pic.command (picport::inc_addr);
      if (got_signal) {
	cerr << "Exiting." << endl;
	return EX_UNAVAILABLE;
      }
    }
  } // 14 bit
  cout << " " << count << " location" << (count != 1 ? "s" : "") << "," << endl;

  cout << "burning fuses," << flush;
  count = 0;
  if (16 == deviceinfo [dev].prog_bits) {
    // Enable access to config memory
    pic.command18 (picport::instr, 0x8ea6);
    pic.command18 (picport::instr, 0x8ca6);
    // Position the program counter
    pic.command18 (picport::instr, 0xef00);
    pic.command18 (picport::instr, 0xf800); // GOTO 100000h
    bool printerr = true;
    for (unsigned long addr = 0; addr < deviceinfo [dev].conf_size; ++addr) {
      if (-1 == conf [addr])
	continue;
      pic.setaddress (0x300000 + addr);

      int word = pic.command18 (picport::tread);
      if (word == conf [addr])
	continue;
      word = conf [addr];
      if (addr & 1)
	word <<= 8;
      pic.command18 (picport::twrite_prog, word);
      pic.command18 (picport::nop_prog, 0);

      word = pic.command18 (picport::tread);
      if (word != conf [addr]) {
	cerr << pic.port() << ":" << "0x" << hex << setfill('0') << setw(6)
	     << 0x300000 + addr << ": configuration byte verification failed, read 0x"
	     << setw(2) << word << ", should be 0x"
	     << setw(2) << conf [addr] << dec << endl;
	if (printerr) {
	  printerr = false;
	  cerr << "This is a configuration byte, which often has hardwired" << endl
	       << "bits and therefore does not verify.  It also may have code" << endl
	       << "protection bits, and if they were programmed to enabled" << endl
	       << "state, verification may fail.  Therefore this error is ignored." << endl;
	}
      }
      ++count;
    }
  } else if (12 == deviceinfo [dev].prog_bits) {
    // 12f508/12f509 reset to configuration word,
    // and the only time the config word is accessable,
    // is right after the reset.
    pic.reset (0xfff);
    // Only one config word.
    retval = program_location (pic, pic.address (),
			       conf [pic.address () - 0xfff], false);
    if (EX_OK == retval)
      ++count;
    else if (NOT_PROGRAMMED != retval)
      return retval;
  } else { // 14 bit    
    pic.command (picport::inc_addr);
    pic.command (picport::inc_addr);
    while (pic.address () + 1 < 0x2007 + deviceinfo [dev].conf_size) {
      pic.command (picport::inc_addr);
      retval = program_location (pic, pic.address (),
				 conf [pic.address () - 0x2007], false);
      if (EX_OK == retval)
	++count;
      else if (NOT_PROGRAMMED != retval)
	return retval;
    }
    // pic12f635 and friends need to reset write latches,
    // but as this is the last operation on them, no need
    // to do that.
  } // 14 bit
  cout << " " << count << " location" << (count != 1 ? "s" : "") << "," << endl;
  cout << "done." << endl;

  signal (SIGTERM, save_t);
  signal (SIGQUIT, save_q);
  signal (SIGINT, save_i);
  if (got_signal) {
    cerr << "Exiting." << endl;
    return EX_UNAVAILABLE;
  }

  return EX_OK;
}


int
hexfile::read_code (picport &pic, short *pgmp, unsigned long addr, unsigned long len)
{
	int c=0,j;
	uint16_t *pnt;
  time_t tv1 = time(0);
  // 14 bit and 12 bit parts must make sure the address
  // is correct before calling this function.
  if (16 == deviceinfo [dev].prog_bits)
    pic.setaddress (addr);
  unsigned long i = 0;
  while(1) {
    assert (pic.address () == addr + i);

    time_t tv2 = time(0);
    if (tv2 >= tv1 + 2) {
      tv1 = tv2;
      cerr << "\r" << hex << setfill('0') << setw(4) << pic.address () << dec
	   << "                           \r";
    }

    if (16 == deviceinfo [dev].prog_bits) {
    	pic.command18 (picport::tread_inc,0,0);
//      pgmp [i] = *pic.execute();
    }
    else if (12 == deviceinfo [dev].prog_bits) {
    	pic.command (picport::data_from_prog,0,0);
//      pgmp [i] = *pic.execute() & 0xfff;
      pic.command (picport::inc_addr, addr_max,0);
    } else { // 14 bit
    	pic.command (picport::data_from_prog,0,0);
//     pgmp [i] = *pic.execute();
      pic.command (picport::inc_addr,0,0);
    }
    c++;

    if(c==10 || i == (len-1)){
    	pnt = pic.execute();
    	for(j=i-(c-1);j<=i;j++){
    		if (12 == deviceinfo [dev].prog_bits)
    			pgmp [j] = *pnt & 0xfff;
    		else
    			pgmp [j] = *pnt;
    		pnt++;
    		if (-1 == pgmp [j]) {
    			cerr << hex << setfill ('0') << setw (4) << addr + j << dec	<< ":unable to read pic" << endl;
    			return EX_IOERR;
    		}

    	}
    	c=0;
    }

    if (got_signal) {
      cerr << "Exiting." << endl;
      return EX_UNAVAILABLE;
    }
    i++;
    if(i == len)
    	break;
  }
  return EX_OK;
}


int
hexfile::read (picport &pic)
{

  sig_type save_t, save_q, save_i;
  save_t = signal (SIGTERM, term_handler);
  save_q = signal (SIGQUIT, term_handler);
  save_i = signal (SIGINT, term_handler);

  cout << "Device " << deviceinfo [dev].name
       << ", program memory: " << deviceinfo [dev].prog_size;
  if (rom == deviceinfo [dev].prog_type && deviceinfo [dev].prog_size)
    cout << " (rom)";
  cout << ", data memory: " << deviceinfo [dev].data_size;
  if (rom == deviceinfo [dev].data_type && deviceinfo [dev].data_size)
    cout << " (rom)";
  cout << "." << endl;

  int e;

  if (deviceinfo [dev].prog_bits == 12) {
    // 12f508/509 reset to configuration word.
    if (pic.address () != 0xfff && pic.address () != 0)
      pic.reset (0xfff);
    // We need to step them to address 0.
    if (pic.address () == 0xfff)
      pic.command (picport::inc_addr, addr_max);
  } else {
    if (pic.address ())
      pic.reset (0);
  }
  if (0 == deviceinfo [dev].prog_size) {
    cout << "Skipped reading program memory," << endl;
  } else {
    cout << "Reading program memory," << endl;
    e = read_code (pic, pgm, 0, deviceinfo [dev].prog_size);
    if (EX_OK != e)
      return e;
  }

  if (0 == deviceinfo [dev].data_size) {
    cout << "skipped reading data memory," << endl;
  } else {
    cout << "reading data memory," << endl;
    if (16 == deviceinfo [dev].prog_bits) {
      // Direct access to data EEPROM.
      pic.command18 (picport::instr, 0x9ea6);
      pic.command18 (picport::instr, 0x9ca6);
    }
    for (unsigned long addr = 0;
	 addr < deviceinfo [dev].data_size;
	 ++addr) {
      if (16 == deviceinfo [dev].prog_bits) {
	// Set the data EEPROM address pointer.
	pic.command18 (picport::instr, 0x0e00 | (addr & 0x00ff));
	pic.command18 (picport::instr, 0x6ea9);
	pic.command18 (picport::instr, 0x0e00 | ((addr & 0xff00) >> 8));
	pic.command18 (picport::instr, 0x6eaa);
	// Initiate a memory read.
	pic.command18 (picport::instr, 0x80a6);
	// Load data into the serial data holding register.
	pic.command18 (picport::instr, 0x50a8);
	pic.command18 (picport::instr, 0x6ef5);

	data [addr] = pic.command18 (picport::shift_out);
      } else if (12 == deviceinfo [dev].prog_bits) {
	cerr << "12 bit microcontroller data memory unimplemented." << endl;
	got_signal = 1;
      } else { // 14 bit
	assert (addr == pic.address () % deviceinfo [dev].data_size);

	data [addr] = pic.command (picport::data_from_data);
	pic.command (picport::inc_addr);
      }
      if (-1 == data [addr]) {
	cerr << pic.port() << ':' << hex << setfill ('0') << setw (4)
	     << addr << dec
	     << ":unable to read pic data memory" << endl;
	return EX_IOERR;
      }
      if (got_signal) {
	cerr << "Exiting." << endl;
	return EX_UNAVAILABLE;
      }
    }
  }

  cout << "reading id words," << endl;
  if (24 == deviceinfo [dev].prog_bits) {
    // no ids in dspic30
    e = EX_OK;
  } else if (16 == deviceinfo [dev].prog_bits) {
    e = read_code (pic, ids, 0x200000, 8);
  } else if (12 == deviceinfo [dev].prog_bits) {
    while (pic.address () != deviceinfo [dev].prog_size)
      pic.command (picport::inc_addr, addr_max);
    // 12f508/12f509 backup osccal is included in ids
    e = read_code (pic, ids, deviceinfo [dev].prog_size, 5);
  } else {
    pic.command (picport::load_conf, 0);
    e = read_code (pic, ids, 0x2000, 4);
  }
  if (EX_OK != e)
    return e;

  // fuses
  cout << "reading fuses," << endl;
  if (24 == deviceinfo [dev].prog_bits) {
    e = read_code (pic, conf, 0xf80000, deviceinfo [dev].conf_size);
  } else if (16 == deviceinfo [dev].prog_bits) {
    e = read_code (pic, conf, 0x300000, deviceinfo [dev].conf_size);
  } else if (12 == deviceinfo [dev].prog_bits) {
    // Only reset allows us to access the config word.
    pic.reset (0xfff);
    e = read_code (pic, conf, 0xfff, deviceinfo [dev].conf_size);
  } else {
    pic.command (picport::inc_addr);
    pic.command (picport::inc_addr);
    pic.command (picport::inc_addr);
    e = read_code (pic, conf, 0x2007, deviceinfo [dev].conf_size);
  }
  if (EX_OK != e)
    return e;
  cout << "done." << endl;

  signal (SIGTERM, save_t);
  signal (SIGQUIT, save_q);
  signal (SIGINT, save_i);
  if (got_signal) {
    cerr << "Exiting." << endl;
    return EX_UNAVAILABLE;
  }

  return EX_OK;
}

int
hexfile::setdevice (picport &pic, int& d)
{
  if (-1 == d && -1 != dev)
    d = dev;
  if (-1 == d) {
    // Read version information off the chip at address 0x2006
    pic.command (picport::load_conf, 0);
    for (int i = 0; i < 6; ++i)
      pic.command (picport::inc_addr);

    assert (0x2006 == pic.address());
    int version = 0;

    int value = pic.command (picport::data_from_prog);

    int family = 14;
    if (0x3fff == value) {
      // Either the device is old model and does not have id bits, or
      // it is 18F device and needs another programming algorithm.
      // Let's try that first and only default if it fails.

      // Enable access to program memory.
      pic.command18 (picport::instr, 0x8ea6);
      pic.command18 (picport::instr, 0x9ca6);
      pic.setaddress (0x3ffffe);

      value = pic.command18 (picport::tread_inc, 0);

      int v1 = pic.command18 (picport::tread, 0);
      if (-1 != value && -1 != v1) {
	value |= (v1 << 8);
	if (0xffff != value)
	  family = 16;
      }
      if (16 != family) {
	// If may be dspic30 then
	pic.command30 (picport::SIX, 0); // NOP
	pic.command30 (picport::SIX, 0); // NOP
	pic.command30 (picport::SIX, 0x040100); // GOTO 0x100
	pic.command30 (picport::SIX, 0); // NOP
	pic.setaddress30 (0xff0000);
	// Step 3
	pic.command30 (picport::SIX, 0xEB0380); // CLR W7
	pic.command30 (picport::SIX, 0xBA0BB6); // TBLRDL [W6++], [W7]
	pic.command30 (picport::SIX, 0); // NOP
	pic.command30 (picport::SIX, 0x883C20); // MOV W0, VISI
	pic.command30 (picport::SIX, 0); // NOP
	// Step 4

	value =  pic.command30 (picport::REGOUT);
	pic.command30 (picport::SIX, 0); // NOP
	// Step 5
	pic.command30 (picport::SIX, 0x040100); // GOTO 0x100
	pic.command30 (picport::SIX, 0); // NOP

	// Step 3
	pic.command30 (picport::SIX, 0xEB0380); // 
	pic.command30 (picport::SIX, 0xBA0BB6); // 
	pic.command30 (picport::SIX, 0); // NOP
	pic.command30 (picport::SIX, 0x883C20); // 
	pic.command30 (picport::SIX, 0); // NOP
	// Step 4

	version =  pic.command30 (picport::REGOUT);
	pic.command30 (picport::SIX, 0); // NOP
	// Step 5
	pic.command30 (picport::SIX, 0x040100); // GOTO 0x100
	pic.command30 (picport::SIX, 0); // NOP
	cerr << hex << "value: 0x" << setw(4) << setfill('0') << value
	     << " version: 0x" << setw(4) << setfill('0') << version
	     << dec << endl;
	if (0xffff != value && 0xffff != version)
	  family = 24;
	else
	  value = -1;
      }
    }
    if (-1 == value) {
      cerr << pic.port() << ':' << hex << setfill ('0') << setw (4) << pic.address () << dec
	   << ":unable to read pic device id" << endl;
      return EX_IOERR;
    }
    d = 0;
    if (0xffff == value)
      cout << pic.port() << ": old device does not have id, defaulting to "
	   << deviceinfo [d].name << endl;
    else
      for (;;++d) {
	if (d >= int(sizeof (hexfile::deviceinfo)
		     / sizeof (hexfile::deviceinfo [0]))) {
	  d = 0;
	  cout << pic.port() << ": ";
	  if (16 == family)
	    cout << "pic18 ";
	  else if (24 == family)
	    cout << "dspic30 ";
	  cout << "device id 0x"
	       << hex << setfill ('0') << setw (4) << value;
	  if (24 == family)
	    cout << " revision 0x" << setfill ('0') << setw (4) << version;
	  cout << dec << " unknown, exiting." << endl;
	  return EX_PROTOCOL;
	}
	if (24 == family) {
	  if (-1 != deviceinfo [d].device_id
	      && deviceinfo [d].prog_bits == family
	      && deviceinfo [d].device_id == value) {
	    cout << pic.port()
		 << ": id 0x" << hex << setfill ('0') << setw(4) << value
		 << ": detected " << deviceinfo [d].name
		 << " version 0x" << setw (4) << version << dec << endl;
	    break;
	  }
	} else {
	  // Some devices like 18f2423, 2523, 4423 and 4523 can be detected
	  // from the set revision high bit.
	  int mask = 0xffe0 | (0x0010 & deviceinfo [d].device_id);
	  //if (-1 != deviceinfo [d].device_id && (mask & 0x0010)) {
//	    printf("fam:%X-%X val:%X mask:%X  %s \n",family,deviceinfo [d].prog_bits,value & mask,deviceinfo [d].device_id & mask,deviceinfo[d].name);
	  //}
	  if (-1 != deviceinfo [d].device_id
	      && deviceinfo [d].prog_bits == family
	      && (deviceinfo [d].device_id & mask) == (value & mask)) {
	    cout << pic.port()
		 << ": id 0x" << hex << setfill ('0') << setw(4) << value
		 << ": detected " << deviceinfo [d].name
		 << " version 0x" << setw (2) << (value & 0x1f & ~mask) << dec << endl;
	    break;
	  }
	}
      }
  } else if (d < 0 || d >= int(sizeof (deviceinfo)
			       / sizeof (deviceinfo [0]))) {
    cerr << "Internal error: invalid device id " << d << endl;
    return EX_SOFTWARE;
  };
  dev = d;

  if (pgm)
    delete [] pgm;
  if (data)
    delete [] data;
  pgm = data = 0;
  if (deviceinfo [dev].prog_size) {
    pgm = new short [deviceinfo [dev].prog_size];
    if (!pgm) {
      cerr << "Out of memory, trying to allocate "
	   << deviceinfo [dev].prog_size * sizeof (short)
	   << " bytes" << endl;
      return EX_UNAVAILABLE;
    }
  }
  if (deviceinfo [dev].data_size) {
    data = new short [deviceinfo [dev].data_size];
    if (!data) {
      cerr << "Out of memory, trying to allocate "
	   << deviceinfo [dev].data_size * sizeof (short)
	   << " bytes" << endl;
      return EX_UNAVAILABLE;
    }
  }
  unsigned long i;
  for (i = 0; i < deviceinfo [dev].prog_size; ++i)
    pgm [i] = -1;
  for (i = 0; i < deviceinfo [dev].data_size; ++i)
    data [i] = -1;
  assert (deviceinfo [dev].conf_size <= 16);
  for (i = 0; i < 16; ++i)
    conf [i] = -1;
  for (i = 0; i < 8; ++i)
    ids [i] = -1;
  if (deviceinfo [dev].prog_bits == 12) // 12f508 12f509
    addr_max = deviceinfo [dev].prog_size * 2;
  else
    addr_max = 0;

  return EX_OK;
}

// Couple of statics in hexfile class.

int
hexfile::find_device (const char *name)
{
  if (!name)
    return -1;
  for (unsigned d = 0;
       d < sizeof (deviceinfo) / sizeof (struct hexfile::devinf);
       ++d)
    if (!strcasecmp (hexfile::deviceinfo [d].name, name))
      return d;
  return -1;
}

void
hexfile::print_devices ()
{
  for (unsigned d = 0;
       d < sizeof (hexfile::deviceinfo) / sizeof (struct hexfile::devinf);
       ++d) {
    if (12 == deviceinfo [d].prog_bits)
      continue;
    if (d)
      cerr << ", ";
    cerr << deviceinfo [d].name;
    if (-1 != deviceinfo [d].device_id)
      cerr << "*";
  }
  cerr << endl
       << "* = autodetected" << endl;
}
