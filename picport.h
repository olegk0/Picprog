/* -*- c++ -*-

This is Picprog, Microchip PIC programmer software for the serial port device.
Copyright © 1997,2002,2003,2004,2008,2010 Jaakko Hyvätti

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


#ifndef H_PICPORT
#define H_PICPORT

#include <ctime>

//#include <termios.h>
#include <sys/ioctl.h>
#include "ser_avrdoper.h"

#define LBUFCMDMAX 250

#define ERR(X)	-X

typedef enum {
	False=0,
	True=1,
} Bool;

#define	IS_CMD	False
#define	IS_DATA	True
#define	AUTOSEND	True

class picport {


public:

  void delay (unsigned int us);

  enum{
	  NO_ERROR=0,
	  ERROR=1,
	  ERROR_NO_DATA=5,
	  ERROR_OVF=10,
	  ERROR_SEND=20,
	  ERROR_RCV=25,
  };

  enum commands {
    load_conf = 0, data_for_prog = 2, data_from_prog = 4,
    inc_addr = 6, beg_prog = 010, data_for_data = 3,
    data_from_data = 5, erase_prog = 011, erase_data = 013,     
    command1 = 1, command7 = 7, end_prog = 016,
    end_prog_only = 027, beg_prog_only = 030, chip_erase = 037
  };

  enum commands18 {
    instr = 000,
    shift_out = 002,
    tread = 010,
    tread_inc = 011,
    tread_dec = 012,
    inc_tread = 013,
    twrite = 014,
    twrite_inc2 = 015,
    twrite_dec2 = 016,
    twrite_prog = 017,

    // flag for command18() method to implement programming delay by
    // holding fourth command clock cycle up.
    nop_prog = 0100,
    // Flag for erase delay
    nop_erase = 0200,
  };

  enum commands30 {
    SIX = 0,
    REGOUT = 1,
  };


  picport ( bool slow);

  ~picport ();

  int command (enum commands comm, int data = 0, Bool exec = True);
  int command18 (enum commands18 comm, int data = 0, Bool exec = True);
  int command30 (enum commands30 comm, int data = 0, Bool exec = True);
  void setaddress (unsigned long a);
  void setaddress30 (unsigned long a);

  unsigned long address () { return addr; }

  void force ();
  void reset (unsigned long reset_address);
  const char *port () { return "Multiprog"; }
  uint16_t *execute();

  void debug (int d) { debug_on = d; }

private:
//  int fd;
//  struct termios saved, termstate;
  unsigned long addr;
  int debug_on;
  int W[16];
  unsigned char inPrgMode;
  unsigned char command_sequence;
  avrdoper HwPort;

  void set_clock_data (int rts, int dtr);
  void set_vpp (int vpp);

//  void p_out (int b);
//  int p_in ();

  int stk500v2_getsync();
  int stk500v2_command(unsigned char * buf,size_t len, size_t maxlen);
  int stk500v2_send(unsigned char * data, size_t len);
  int stk500v2_recv(unsigned char *msg, size_t maxsize);

  int buf_send(void);
  int add_to_buf(unsigned char byte, Bool IsCmd, Bool AutoSend=AUTOSEND);
  int send_n_bits(unsigned char cnt, unsigned int var);
  int read_n_bits(unsigned char mode, Bool exec);

  struct lbuf_s{
	  unsigned char count;
	  unsigned char last_cmd_ind;
	  unsigned char buf[LBUFCMDMAX];
	  unsigned char temp_buf[LBUFCMDMAX];
  };

  struct lbuf_s cmd_buf;

};

#if 0
#define PDEBUGS(...) fprintf(stderr, __VA_ARGS__)
#define	PDEBUG(fmt, args...)	fprintf(stderr,"DBG:%s - " fmt "\n",__func__, ##args)
#define	STK500DEBUG(...)
#else
#define	STK500DEBUG(...)
#define PDEBUG(...)
#define PDEBUGS(...)
#endif


#endif // H_PICPORT
