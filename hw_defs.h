/*
 * hw_defs.h
 *
 *  Created on: 26.01.2015
 *      Author: iam
 */

#ifndef HW_DEFS_H_
#define HW_DEFS_H_

/* =================== [ My commands for PIC ] =================== */

#define STK_CMD_PREPARE_PROGMODE_ISCP           0x50
#define STK_CMD_LEAVE_PROGMODE_ISCP             0x51
#define STK_CMD_RUN_ISCP		            0x52


//----------------list of commands--------------------
//hi(5)+low(3) => 8bits - commands with his variants
//low - count of parameters > 0-7 bytes
//hi - code of command from list: (0-31)
enum{
c_nop=0,
c_enablePGC_D=5,			//05
//c_setPGDinput=10,			//0a
//c_setPGDoutput=11,		//0b

c_PGDlow=20,				//14
c_PGDhigh=21,				//15
c_PGClow=22,				//16
c_PGChigh=23,				//17
c_VDDon=24,					//18
c_VDDoff=25,				//19

c_HVReset_ENABLE=30,		//1e
c_HVReset_OFF=31,			//1f
c_HVReset_TO_RESET=32,		//20
c_HVReset_TO_HV=33,			//21

c_clock_delay=40,			//28
c_DelayMs=41,				//29	+1 byte
c_DelayUs=42,				//2a	+1 byte

c_pic_send=50, 				//32	+1b(size bits) +1-4 bytes data (lo-hi)
c_dspic_send_24=51,			//33	+3 bytes (lo -hi)

c_pic_read=60,				//3c
c_pic_read_14_bits=61,		//3d
c_pic_read_byte2=62,		//3e
c_dspic_read_16_bits=63,	//3f

c_set_param=70				//46	+2 bytes - num param and value
};
#define c_setPGDinput c_PGDhigh
#define c_setPGDoutput c_PGDlow

#define toCMD(x) x
//x<<3
#define CMD_MASK_HI 0b11111000
#define CMD_MASK_LO 0b111

// for c_set_param
enum{
	p_param_clock_delay = 0,
};

#endif /* HW_DEFS_H_ */
