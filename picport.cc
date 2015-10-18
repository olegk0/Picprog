/* -*- c++ -*-

This is Picprog, Microchip PIC programmer software for the serial port device.
Copyright © 1997,2002,2003,2004,2006,2007,2008,2010 Jaakko Hyvätti

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
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2005 Erik Walthinsen
 * Copyright (C) 2002-2004 Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2006 David Moore
 * Copyright (C) 2006,2007,2010 Joerg Wunsch <j@uriah.heep.sax.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id: stk500v2.c 1294 2014-03-12 23:03:18Z joerg_wunsch $ */
/* Based on Id: stk500.c,v 1.46 2004/12/22 01:52:45 bdean Exp */

/*
 * avrdude interface for Atmel STK500V2 programmer
 *
 * As the AVRISP mkII device is basically an STK500v2 one that can
 * only talk across USB, and that misses any kind of framing protocol,
 * this is handled here as well.
 *
 * Note: most commands use the "universal command" feature of the
 * programmer in a "pass through" mode, exceptions are "program
 * enable", "paged read", and "paged write".
 *
 */
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <ctime>

#include <sys/ioctl.h>
#include <sys/io.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
//#include <termios.h>
#include <sysexits.h>
#include <sched.h>

#include "ser_avrdoper.h"
#include "hw_defs.h"
#include "stk500v2_private.h"

#include "picport.h"

using namespace std;

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

// 
//static bool disable_interrupts = 1;

// Retry count
#define RETRIES 5

// Timeout (in seconds) for waiting for serial response
#define SERIAL_TIMEOUT 2

int picport::stk500v2_getsync()
{
  int tries = 0;
  unsigned char buf[1], resp[32];
  int status;

retry:
  tries++;

  // send the sync command and see if we can get there
  buf[0] = CMD_SIGN_ON;

  stk500v2_send(buf, 1);

  // try to get the response back and see where we got
  status = stk500v2_recv(resp, sizeof(resp));

  // if we got bytes returned, check to see what came back
  if (status > 0) {
    if ((resp[0] == CMD_SIGN_ON) && (resp[1] == STATUS_CMD_OK) &&
	(status > 3)) {
      // success!
      unsigned int siglen = resp[2];
      if (siglen >= strlen("STK500_2") &&
	  memcmp(resp + 3, "STK500_2", strlen("STK500_2")) == 0) {
    	  fprintf(stderr, "\nstk500v2 found\n");
//	PDATA(pgm)->pgmtype = PGMTYPE_STK500;
      } else {
	resp[siglen + 3] = 0;
//	if (verbose)
	  fprintf(stderr,
		  "stk500v2_getsync(): got response from unknown "
		  "programmer %s, assuming STK500\n", resp + 3);
      }
      return 0;
    } else {
      if (tries > RETRIES) {
        fprintf(stderr,
                "stk500v2_getsync(): can't communicate with device: resp=0x%02x\n", resp[0]);
        return -6;
      } else
        goto retry;
    }

  // or if we got a timeout
  } else if (status == -1) {
    if (tries > RETRIES) {
      fprintf(stderr,"stk500v2_getsync(): timeout communicating with programmer\n");
      return -1;
    } else
      goto retry;

  // or any other error
  } else {
    if (tries > RETRIES) {
      fprintf(stderr,"stk500v2_getsync(): error communicating with programmer: (%d)\n",status);
    } else
      goto retry;
  }

  return 0;
}
//***************************************************************************
int picport::stk500v2_command( unsigned char * buf,size_t len, size_t maxlen)
{
  int i;
  int tries = 0;
  int status;

  PDEBUGS("STK500V2: command(");
  for (i=0;i<len;i++) PDEBUGS("0x%02x ",buf[i]);
  PDEBUGS(", %d)\n",len);

retry:
  tries++;

  // send the command to the programmer
  stk500v2_send(buf,len);
  // attempt to read the status back
  status = stk500v2_recv(buf,maxlen);

  // if we got a successful readback, return
  if (status > 0) {
    PDEBUG(" = %d",status);
    if (status < 2) {
      fprintf(stderr, "stk500v2_command(): short reply\n");
      return ERR(ERROR_RCV);
    }
/*    if (buf[0] == CMD_XPROG_SETMODE || buf[0] == CMD_XPROG) {

//         * Decode XPROG wrapper errors.

        const char *msg;
        int i;


//         * For CMD_XPROG_SETMODE, the status is returned in buf[1].
//         * For CMD_XPROG, buf[1] contains the XPRG_CMD_* command, and
//         * buf[2] contains the status.

        i = buf[0] == CMD_XPROG_SETMODE? 1: 2;

        if (buf[i] != XPRG_ERR_OK) {
            switch (buf[i]) {
            case XPRG_ERR_FAILED:   msg = "Failed"; break;
            case XPRG_ERR_COLLISION: msg = "Collision"; break;
            case XPRG_ERR_TIMEOUT:  msg = "Timeout"; break;
            default:                msg = "Unknown"; break;
            }
            fprintf(stderr, "stk500v2_command(): error in %s: %s\n",
                    (buf[0] == CMD_XPROG_SETMODE? "CMD_XPROG_SETMODE": "CMD_XPROG"),
                    msg);
            return -1;
        }
        return 0;
    } else*/
    {
        /*
         * Decode STK500v2 errors.
         */
        if (buf[1] >= STATUS_CMD_TOUT && buf[1] < 0xa0) {
            const char *msg;
            char msgbuf[30];
            switch (buf[1]) {
            case STATUS_CMD_TOUT:
                msg = "Command timed out";
                break;

            case STATUS_RDY_BSY_TOUT:
                msg = "Sampling of the RDY/nBSY pin timed out";
                break;

            case STATUS_SET_PARAM_MISSING:
                msg = "The `Set Device Parameters' have not been "
                    "executed in advance of this command";

            default:
                sprintf(msgbuf, "unknown, code 0x%02x", buf[1]);
                msg = msgbuf;
                break;
            }
/*            if (quell_progress < 2) {
                fprintf(stderr, "stk500v2_command(): warning: %s\n", msg);
            }
*/        } else if (buf[1] == STATUS_CMD_OK) {
            return status;
        } else if (buf[1] == STATUS_CMD_FAILED) {
            fprintf(stderr,"stk500v2_command(): command failed\n");
        } else if (buf[1] == STATUS_CMD_UNKNOWN) {
            fprintf(stderr,"stk500v2_command(): unknown command:0x%02x\n",buf[2]);
        } else {
            fprintf(stderr, "stk500v2_command(): unknown status 0x%02x\n", buf[1]);
        }
        return ERR(ERROR_RCV);
    }
  }

  // otherwise try to sync up again
  status = stk500v2_getsync();
  if (status != 0) {
    if (tries > RETRIES) {
      fprintf(stderr,"stk500v2_command(): failed miserably to execute command 0x%02x\n",buf[0]);
      return -1;
    } else
      goto retry;
  }

  PDEBUG(" = 0");
  return NO_ERROR;
}
//***************************************************************************
int picport::stk500v2_send( unsigned char * data, size_t len)
{
  unsigned char buf[275 + 6];		// max MESSAGE_BODY of 275 bytes, 6 bytes overhead
  int i;

  buf[0] = MESSAGE_START;
  buf[1] = command_sequence;
  buf[2] = len / 256;
  buf[3] = len % 256;
  buf[4] = TOKEN;
  memcpy(buf+5, data, len);

  // calculate the XOR checksum
  buf[5+len] = 0;
  for (i=0;i<5+len;i++)
    buf[5+len] ^= buf[i];

  PDEBUGS("STK500V2: send(");
  for (i=0;i<len+6;i++) PDEBUGS("0x%02x ",buf[i]);
  PDEBUGS(", %d)\n",len+6);

  if (HwPort.avrdoper_send(buf, len+6) != 0) {
    fprintf(stderr,"stk500_send(): failed to send command to serial port\n");
    exit(1);
  }

  return 0;
}

//***************************************************************************

int picport::stk500v2_recv( unsigned char *msg, size_t maxsize) {
  enum states { sINIT, sSTART, sSEQNUM, sSIZE1, sSIZE2, sTOKEN, sDATA, sCSUM, sDONE }  state = sSTART;
  unsigned int msglen = 0;
  unsigned int curlen = 0;
  int timeout = 0;
  unsigned char c, checksum = 0;

  long timeoutval = SERIAL_TIMEOUT;		// seconds
  struct timeval tv;
  double tstart, tnow;


  PDEBUGS("STK500V2: recv:");

  gettimeofday(&tv, NULL);
  tstart = tv.tv_sec;

  while ( (state != sDONE ) && (!timeout) ) {
    if (HwPort.avrdoper_recv(&c, 1) < 0)
      goto timedout;
    PDEBUGS("0x%02x ",c);
    checksum ^= c;

    switch (state) {
      case sSTART:
    	  STK500DEBUG("hoping for start token...");
    	  if (c == MESSAGE_START) {
    		  STK500DEBUG("got it");
    		  checksum = MESSAGE_START;
    		  state = sSEQNUM;
    	  } else
    		  STK500DEBUG("sorry");
    	  break;
      case sSEQNUM:
    	  STK500DEBUG("hoping for sequence...");
    	  if (c == command_sequence) {
    		  STK500DEBUG("got it, incrementing");
    		  state = sSIZE1;
    		  command_sequence++;
    	  } else {
    		  STK500DEBUG("sorry");
    		  state = sSTART;
    	  }
    	  break;
      case sSIZE1:
    	  STK500DEBUG("hoping for size LSB");
    	  msglen = (unsigned)c * 256;
    	  state = sSIZE2;
    	  break;
      case sSIZE2:
    	  STK500DEBUG("hoping for size MSB...");
    	  msglen += (unsigned)c;
    	  PDEBUGS("\n");
    	  PDEBUG(" msg is %u bytes",msglen);
    	  state = sTOKEN;
    	  break;
      case sTOKEN:
    	  if (c == TOKEN) state = sDATA;
    	  else state = sSTART;
    	  break;
      case sDATA:
    	  if (curlen < maxsize) {
    		  msg[curlen] = c;
    	  } else {
    		  PDEBUGS("\n");
    		  PDEBUG("buffer too small, received %d byte into %u byte buffer\n", curlen,(unsigned int)maxsize);
    		  return -2;
    	  }
    	  if ((curlen == 0) && (msg[0] == ANSWER_CKSUM_ERROR)) {
    		  PDEBUGS("\n");
    		  PDEBUG("previous packet sent with wrong checksum\n");
    		  return -3;
    	  }
    	  curlen++;
    	  if (curlen == msglen) state = sCSUM;
    	  break;
      case sCSUM:
    	  if (checksum == 0) {
    		  state = sDONE;
    	  } else {
    		  state = sSTART;
    		  PDEBUGS("\n");
    		  PDEBUG("checksum error\n");
    		  return -4;
    	  }
    	  break;
      default:
    	  PDEBUGS("\n");
    	  PDEBUG("unknown state\n");
    	  return -5;
     } /* switch */

     gettimeofday(&tv, NULL);
     tnow = tv.tv_sec;
     if (tnow-tstart > timeoutval) {			// wuff - signed/unsigned/overflow
    	 timedout:
		 PDEBUGS("\n");
		 PDEBUG("timeout\n");
		 return -1;
     }

  } /* while */
  PDEBUGS("\n");

  return (int)(msglen+6);
}

//*************************************+++++++++++++++++++++++++++++++++++******************************
picport::picport (bool slow)  : addr (0), debug_on (0)
{
	int ret,i;

  command_sequence = 1;

  for (i = 0; i < 16; ++i)
    W [i] = 0;
//  portname = new char [strlen (tty) + 1];
//  strcpy (portname, tty);

  if ((ret = HwPort.avrdoper_open()) > 0) {
    cerr << "Unable to open HW :" << strerror (ret) << endl;
    exit (EX_IOERR);
  }

  HwPort.avrdoper_drain();

//  cerr << "\n***********Get SYnc*******";
  stk500v2_getsync();

  HwPort.avrdoper_drain();

  /*cmd_buf.buf[0] = STK_CMD_LEAVE_PROGMODE_ISCP;
  ret = stk500v2_command( cmd_buf.buf, 1, sizeof(cmd_buf.buf));
  usleep (500000);*/
  cmd_buf.buf[0] = STK_CMD_PREPARE_PROGMODE_ISCP;
  ret = stk500v2_command( cmd_buf.buf, 1, sizeof(cmd_buf.buf));

//  buf[1] = 190;//lo 100
//  buf[2] = 1;//hi 2

  PDEBUG("Prepare programm mode:%d",ret);
//  sleep(1);
  cmd_buf.count = 0;
  buf_send();//init

  if(ret > 0){//ok
	  inPrgMode = 1;
	  //setup
	  add_to_buf(c_VDDon, IS_CMD);

	  add_to_buf(c_set_param, IS_CMD);
	  add_to_buf(p_param_clock_delay, IS_DATA);
	  add_to_buf(0, IS_DATA);
	  
	  add_to_buf(c_DelayMs, IS_CMD);
	  add_to_buf(250, IS_DATA);

	  add_to_buf(c_HVReset_ENABLE, IS_CMD);
	  add_to_buf(c_HVReset_TO_RESET, IS_CMD);

	  add_to_buf(c_DelayMs, IS_CMD);
	  add_to_buf(10, IS_DATA);

	  add_to_buf(c_enablePGC_D, IS_CMD);
	  add_to_buf(c_PGClow, IS_CMD);
	  add_to_buf(c_PGDlow, IS_CMD);

	  add_to_buf(c_DelayMs, IS_CMD);
	  add_to_buf(250, IS_DATA);

	  add_to_buf(c_HVReset_TO_HV, IS_CMD);

	  add_to_buf(c_DelayMs, IS_CMD);
	  add_to_buf(250, IS_DATA);
	  ret = buf_send();
	  PDEBUG("Config programm mode:%d",ret);
  }
  else
	  inPrgMode = 0;
//  fprintf(stderr,"\n ret=%d\n",ret);

//  sleep(1);
usleep (500000);
//	  exit(0);
 /*   if (reboot) {
      // Power off any microcontroller that may be running a program.
      cerr << "Power off." << endl;
      set_clock_data (1, 1);
      set_vpp (1);
      usleep (500000); // 0.5s delay should discharge Vdd.
    }
*/
    // /MCLR must go down for a while first
/*    set_vpp (0);
    usleep (10);
    // Power up
    set_clock_data (0, 0);
    set_vpp (1);
    // Charge Vdd
    usleep (25000);
*/

}

picport::~picport ()
{
	unsigned char buf[5];
//  set_vpp (0);
//	usleep (1);
//  delete [] portname;
	buf[0] = STK_CMD_LEAVE_PROGMODE_ISCP;
	stk500v2_command(buf, 1, sizeof(buf));
}

int picport::buf_send(void)
{
	int ret = ERR(ERROR_NO_DATA);

	if(cmd_buf.count > 1){
		cmd_buf.buf[cmd_buf.count] = 0;//nop
		cmd_buf.count++;
		ret = stk500v2_command( cmd_buf.buf, cmd_buf.count, sizeof(cmd_buf.buf));

	}

	cmd_buf.last_cmd_ind = 1;
	cmd_buf.count = 1;
	cmd_buf.buf[0] = STK_CMD_RUN_ISCP;

	PDEBUG("count:%d ret:%d",cmd_buf.count,ret);

	return ret;
}

int picport::add_to_buf(unsigned char byte, Bool IsData, Bool AutoSend)
{
	int ret = NO_ERROR, len,i;

	if(cmd_buf.count >= (LBUFCMDMAX - 1)){
		PDEBUG("Buf ovf, byte:0x%02x count:%d AutoSend:%d",byte,cmd_buf.count,AutoSend);
		if(AutoSend){
			if(IsData){
				len = cmd_buf.count - cmd_buf.last_cmd_ind;
				memcpy(&(cmd_buf.temp_buf), &(cmd_buf.buf[cmd_buf.last_cmd_ind]), len);
				cmd_buf.count = cmd_buf.last_cmd_ind;
			}

			ret = buf_send();

			if(IsData){
				memcpy(&(cmd_buf.buf[1]), &(cmd_buf.temp_buf), len);
				cmd_buf.count += len;
			}
		}
		else
			return ERR(ERROR_OVF);
	}

	cmd_buf.buf[cmd_buf.count] = byte;
	if(!IsData){
		cmd_buf.last_cmd_ind = cmd_buf.count;
	}
	cmd_buf.count++;

//	PDEBUG("count:%d  cmd_ind:%d",cmd_buf.count,cmd_buf.last_cmd_ind);

	return ret;
}

void picport::reset (unsigned long reset_address)
{
  set_clock_data (0, 0);
  delay(100); // Make sure we have the power there.
  set_vpp (0);
  delay(50);
  set_vpp (1);
  delay(10);
  addr = reset_address;
  buf_send();
}

int picport::send_n_bits(unsigned char cnt, unsigned int var)
{
//	int ret;

	PDEBUG("--Send %d bits:%X",cnt,var);

	add_to_buf(c_pic_send, IS_CMD);
	add_to_buf(cnt, IS_DATA);
	add_to_buf((unsigned char)(var&0xff), IS_DATA);

	if(cnt>8){// 16;
		add_to_buf((unsigned char)((var>>8)&0xff), IS_DATA);
	}
	if(cnt>16){// 24;
		add_to_buf((unsigned char)((var>>16)&0xff), IS_DATA);
	}
	if(cnt>24){// 32;
		add_to_buf((unsigned char)((var>>24)&0xff), IS_DATA);
	}
	
	return NO_ERROR;//TODO
}

int picport::read_n_bits(unsigned char mode, Bool exec)
{
	int ret=NO_ERROR;
//usleep(100000);
	switch(mode){
	case 8:
		add_to_buf(c_pic_read_byte2, IS_CMD);
		break;
	case 14:
		add_to_buf(c_pic_read_14_bits, IS_CMD);
		break;
	case 16:
		add_to_buf(c_dspic_read_16_bits, IS_CMD);
		break;
	}

	if(exec){
		buf_send();
		ret = cmd_buf.buf[3]<<8 |cmd_buf.buf[2];//(int)((uint16_t *)&lbuf.buf[2]);
	}

	PDEBUG("--Read %d mode, ret=%X",mode,ret);
	return ret;

}

uint16_t *picport::execute() //TODO
{
	int ret;
	ret = buf_send();
	PDEBUG("--Execute, ret=%d",ret);

	return (uint16_t *)&cmd_buf.buf[2];
}

void picport::set_clock_data (int clk, int dt)
{

	PDEBUG("--Set clock data %d:%d bytes",clk,dt);
	if(clk)
		add_to_buf(c_PGChigh, IS_CMD);
	else
		add_to_buf(c_PGClow, IS_CMD);
	if(dt)
		add_to_buf(c_PGDhigh, IS_CMD);
	else
		add_to_buf(c_PGDlow, IS_CMD);

}

void picport::set_vpp (int vpp)
{

	if(vpp == 0)
		add_to_buf(c_HVReset_TO_RESET, IS_CMD);
	else
		add_to_buf(c_HVReset_TO_HV, IS_CMD);

}

void picport::delay (unsigned int us)
{

/*
		usleep(us);

	return;
*/
	if(us < 600){
		if(us<5) us = 5;
		add_to_buf(c_DelayUs, IS_CMD);
		add_to_buf((us+3) / 5, IS_DATA);
	}else{
		add_to_buf(c_DelayMs, IS_CMD);
		if(us <= 1000)
			add_to_buf(1, IS_DATA);
		else if(us < (250*1000))
			add_to_buf((us+500)/1000, IS_DATA);
		else
			add_to_buf(250, IS_DATA);
	}

}

int picport::command18 (enum commands18 comm, int data, Bool exec)
{
	int i, shift = comm;

	if (nop_prog == comm) {
    // A programming command must leave the last bit clock pulse up
/*    p_out (0);
    p_out (0);
    p_out (0);
*/
		send_n_bits(3,0);

		set_clock_data (1, 0); // clock up
		delay (1000); // P9 >1 ms programming time
		set_clock_data (0, 0); // clock down
    // P10 >5 µs high voltage discharge time
    // Later models listed as > 100 µs
		delay (100);
	} else {
//		for (i = 0; i < 4; i++)
//			p_out ((shift >> i) & 1);
		send_n_bits(4,shift);
		//set_clock_data (0, 0); // set data down
		delay (1);
	}

	shift = 0; // default return value

	switch (comm) {
	case nop_erase:
    // Erase cycle has delay between command and data
		delay(10000); // P11 5 ms + P10 5 µs erase time
    // FALLTHROUGH

	case instr:
	case nop_prog:
		if (0x0e00 == (data & 0xff00))
			W[0] = data & 0x00ff;
		else if (0x6ef8 == data)
			addr = (addr & 0x00ffff) | (W[0] << 16);
		else if (0x6ef7 == data)
			addr = (addr & 0xff00ff) | (W[0] << 8);
		else if (0x6ef6 == data)
			addr = (addr & 0xffff00) | W[0];
		goto sw;

	case twrite_dec2:
		addr -= 2;
		goto sw;

	case twrite_inc2:
		addr += 2;
    // FALLTHROUGH

	case twrite:
	case twrite_prog:
  sw:
//    for (i = 0; i < 16; i++)
//      p_out ((data >> i) & 1);
//    set_clock_data (0, 0); // set data down
		send_n_bits(16,data);
		break;

	case tread_dec:
		--addr;
		goto sr;

	case tread_inc:
	case inc_tread:
		++addr;
    // FALLTHROUGH

	case shift_out:
	case tread:
  sr:
//    	for (i = 0; i < 8; i++)
//    		p_out(0);
//    	delay (1);
//    	for (i = 0; i < 8; i++)
//    		shift |= p_in () << i;
//    	set_clock_data (0, 0); // set data down
    	shift = read_n_bits(8,exec);
	}

	delay (1);
	return shift;
}

int picport::command30 (enum commands30 comm, int data, Bool exec)
{
  int i, shift;
//  for (i = 0; i < 4; i++)
//    p_out ((shift >> i) & 1);

  shift = 0; // default return value

  switch (comm) {
  case SIX:

	send_n_bits(4,SIX);

    if (0x200000 == (data & 0xff0000))
      W[data & 15] = (data & 0x0ffff0) >> 4;
    else if (0xEB0000 == (data & 0xfff87f))
      W[(data >> 7) & 15] = 0;
    else if (0xBA1830 == (data & 0xfff870)) {
      // TBLRDL
      ++W[(data >> 7) & 15];
      ++W[data & 15];
    } else if (0xBA0830 == (data & 0xfff870)) {
      // TBLRDL
      ++W[(data >> 7) & 15];
      ++W[data & 15];
    } else if (0x880190 == (data & 0xfffff0))
      addr = (W[data & 15] << 16) & 0xff0000;
    addr = (addr & 0xff0000) | W[6];

//    for (i = 0; i < 24; i++)
//      p_out ((data >> i) & 1);
    send_n_bits(24,data);
    break;

  case REGOUT:
	  shift = read_n_bits(16,exec);
/*    for (i = 0; i < 8; i++)
      p_out(0);
    for (i = 0; i < 16; i++)
      shift |= p_in () << i;
*/
  }
//  set_clock_data (0, 0); // set data down

  return shift;
}

void picport::setaddress (unsigned long a)
{
  if (0 != a && addr == a)
    return;

  command18 (instr, 0x0e00 | ((a & 0xff0000) >> 16));
  command18 (instr, 0x6ef8);
  command18 (instr, 0x0e00 | ((a & 0x00ff00) >> 8));
  command18 (instr, 0x6ef7);
  command18 (instr, 0x0e00 | (a & 0x0000ff));
  command18 (instr, 0x6ef6);
}

void picport::setaddress30 (unsigned long a)
{
  if (0 != a && addr == a)
    return;

  command30 (SIX, 0x200000 | ((a & 0xff0000) >> 12)); // MOV #, W0
  command30 (SIX, 0x880190); // MOV W0, TBLPAG
  command30 (SIX, 0x200006 | ((a & 0x00ffff) << 4)); // MOV #, W6
}

// -1 == error, no programmer present

int picport::command (enum commands comm, int data, Bool exec)
{
  int tmp1, tmp2;

  // first, send out the command, 6 bits

  int i, shift;
//  for (i = 0; i < 6; i++)
//    p_out ((shift >> i) & 1);
//  set_clock_data (0, 0); // set data down
  send_n_bits(6,comm);

  shift = 0; // default return value

  switch (comm) {
  case inc_addr:
    ++addr;
    if (data != 0) { // 12f508 and 12f509
      if (addr >= (unsigned long)(data))
	addr = 0;
      break;
    }

    if (addr >= 0x4000)
      addr = 0x2000;
    break;

  case data_from_prog:
  case data_from_data:
    delay (1);
    shift = read_n_bits(14,exec);
/*    tmp1 = p_in ();
    for (i = 0; i < 14; i++)
      shift |= p_in () << i;
    tmp2 = p_in ();
    set_clock_data (0, 0); // set data down
*/
#ifdef CHECK_START_STOP
    // Start and stop bits must be 1.  Most of the old chips at least
    // conform to this test, but apparently pic12f635 and some other
    // later chips do not.

    if (!tmp1 || !tmp2) {
      cerr << portname << ":PIC programmer missing or chip fault" << endl;
      return -1;
    }
#endif

    if (data_from_data == comm) {

      // Check that the leftover bits were valid, all 1's.
      // This detects if the programmer is not connected to the port.
      // Unfortunately later chips clear these bits, so we must
      // accept both all 1's and all 0's.

      if ((shift & 0x3f00) != 0x3f00
	  && (shift & 0x3f00) != 0x0000) {
	cerr << ": read value "
	     << hex << setfill('0') << setw(4) << shift << dec
	     << ": PIC programmer or chip fault\n"
	  "Is code protection enabled?  "
	  "Use --erase option to disable code protection." << endl;
	return -1;
      }

      shift &= 0xff;
    }
    break;

  case load_conf:
    addr = 0x2000;
    // FALLTHROUGH

  case data_for_prog:
  case data_for_data:
    delay (1);
/*    p_out (0);
    for (i = 0; i < 14; i++)
      p_out ((data >> i) & 1);
    p_out (0);
    set_clock_data (0, 0); // set data down
*/
    tmp1 = (data&0b11111111111111) << 1;
    send_n_bits(16,tmp1);
    break;

  default:
    ;
  }

  delay (1);
  return shift;
}

