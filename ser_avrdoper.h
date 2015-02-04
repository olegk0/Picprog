/*
 * ser_avrdoper.h
 *
 *  Created on: 25.01.2015
 *      Author: iam
 */

#ifndef SER_AVRDOPER_H_
#define SER_AVRDOPER_H_

#include "serial.h"
#include <usb.h>

class avrdoper{
public:
	avrdoper();
	~avrdoper();
	int avrdoper_open();
	void avrdoper_close();
	int avrdoper_send(unsigned char *buf, size_t buflen);
	int avrdoper_recv(unsigned char *buf, size_t buflen);
	int avrdoper_drain();

private:
	usb_dev_handle *pfd;
	static const int  reportDataSizes[4];

	unsigned char    avrdoperRxBuffer[280];  /* buffer for receive data */
	int              avrdoperRxLength;   /* amount of valid bytes in rx buffer */
	int              avrdoperRxPosition; /* amount of bytes already consumed in rx buffer */

	int  usesReportIDs;

	void avrdoperFillBuffer();
	int  chooseDataSize(int len);
	const char *usbErrorText(int usbErrno);
	void dumpBlock(const char *prefix, unsigned char *buf, int len);
	int usbGetReport(int reportType, int reportNumber,char *buffer, int *len);
	int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen);
	int usbOpenDevice(int vendor, const char *vendorName, int product, const char *productName, int doReportIDs);
	int usbSetReport( int reportType, char *buffer, int len);
};


#endif /* SER_AVRDOPER_H_ */
