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
c_nop =0,
c_setPGDinput=10,		//0a
c_setPGDoutput=11,		//0b

c_PGDlow=20,			//14
c_PGDhigh=21,			//15
c_PGClow=22,			//16
c_PGChigh=23,			//17
c_VDDon=24,				//18
c_VDDoff=25,			//19

//c_HVReset_ENABLE=30,
c_HVReset_OFF=31,		//1f
c_HVReset_TO_RESET=32,	//20
c_HVReset_TO_HV=33,		//21

c_clock_delay=40,		//28
c_DelayMs=41,//+1 byte	//29
c_DelayUs=42,//+1 byte	//2a

c_pic_send=50, 			//32	 + 2-5 bytes (c_pic_send_8  c_pic_send_16  c_pic_send_24 c_pic_send_32)
c_dspic_send_24=51,		//33	+3 bytes

c_pic_read=60,			//3c
c_pic_read_14_bits=61,	//3d
c_pic_read_byte2=62,	//3e
c_dspic_read_16_bits=63,//3f

c_set_param=70			//46	+2 bytes - num param and value
};

#define toCMD(x) x
//x<<3
#define CMD_MASK_HI 0b11111000
#define CMD_MASK_LO 0b111

// for c_set_param
enum{
	p_param_clock_delay = 0,
};

#endif /* HW_DEFS_H_ */
