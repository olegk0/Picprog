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

/*

  Hexfile follows the Microchip hex file format for pic16c84:

  - program memory at locations 0-0x3ff
  - id at locations 0x2000-0x2003
  - fuses at location 0x2007
  - data memory at locations 0x2100-0x213f

 */


#ifndef H_HEXFILE
#define H_HEXFILE

#include <fstream>
using namespace std;

#include "picport.h"

class hexfile {

  // pic16 family: program size is counted in words.
  // pic18 family: program memory size is counted in bytes.
  short *pgm;
  short *data;
  short conf [16];
  short ids  [8];

  // Return code used in program_location () to indicate that the
  // location already was programmed to specified value.
  static const int NOT_PROGRAMMED = -3;

  void reset_code_protection (picport& pic);
  int program_location (picport& pic, unsigned long addr, short word, bool isdata) const;
  bool verify18 (picport& pic, const short *pgmp, unsigned long addr, unsigned long len, unsigned long panel_size, bool verbose) const;
  int program18 (picport& pic, const short *pgmp, unsigned long addr, unsigned long len, unsigned long panel_size) const;

public:
  enum formats { unknown, ihx8m, ihx16, ihx32 };
  enum memtypes { flash, flash2, flash3, flash4, flash5,
		  flash18,
		  flash30,
		  eeprom, eprom, eprom18, prom, rom};

private:
  int dev;
  int addr_max; // Used in inc_addr command for 12f only

  void save_line (ofstream& f, const short *pgmp, unsigned long begin, unsigned long len, enum formats format) const;
  int save_region (ofstream& f, const short *pgmp, unsigned long addr0, unsigned long len0, enum formats format, bool skip_ones, unsigned long &addr32) const;
  int read_code (picport &pic, short *pgmp, unsigned long addr, unsigned long len);

  struct devinf {
    const char *name;
    unsigned long prog_size;

    // How many words are reserved at the end of program memory
    // for for example oscillator calibration?
    unsigned long prog_preserved;

    // Bits that must be preserved of configuration word 0x2007.
    int config_mask;

    unsigned conf_size;

    // Either 12 or 14 for now.  Only 14 bit devices tested
    int prog_bits;

    // Original 18f parts used multipanel writes.  0 disables this.
    int panel_size;

    // How many bytes/words to write at one programming command (18f).
    // 0 means unknown for 14 bit series, 1 byte writes work.
    // In the future maybe for 14 bit series multiword programming
    // will also be implemented.  In that case, fill this field
    // for them too.
    int write_size;

    enum memtypes prog_type;
    unsigned data_size;
    enum memtypes data_type;

    // Device id stored at 0x2006.  -1==no id on this device.
    int device_id;
  };
  static const struct devinf deviceinfo [];

public:

  hexfile () : pgm(0), data(0), dev(-1), addr_max(0) {};
  ~hexfile () {
    if (pgm)
      delete [] pgm;
    if (data)
      delete [] data;
  }

  int setdevice (picport &pic, int& d);

  int load (const char *name);
  int save (const char *name, enum formats format, bool skip_ones) const;

  int program (picport &pic, bool erase, bool nopreserve);
  int read (picport &pic);


  // statics

  static int find_device (const char *name);
  static void print_devices ();
};

#endif // H_HEXFILE
