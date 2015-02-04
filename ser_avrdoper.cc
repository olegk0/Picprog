/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2003-2004  Theodore A. Roth  <troth@openavr.org>
 * Copyright (C) 2006 Joerg Wunsch <j@uriah.heep.sax.de>
 * Copyright (C) 2006 Christian Starkjohann
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

/* $Id: ser_avrdoper.c 1294 2014-03-12 23:03:18Z joerg_wunsch $ */

/*
 * Serial Interface emulation for USB programmer "AVR-Doper" in HID mode.
 */

//#include "ac_cfg.h"
#define HAVE_LIBUSB
#define HAVE_USB_H
unsigned char verbose = 0;

#if defined(HAVE_LIBUSB) || (defined(WIN32NATIVE) && defined(HAVE_LIBHID))

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ser_avrdoper.h"


/* ------------------------------------------------------------------------ */

/* Numeric constants for 'reportType' parameters */
#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3

/* These are the error codes which can be returned by functions of this
 * module.
 */
#define USB_ERROR_NONE      0
#define USB_ERROR_ACCESS    1
#define USB_ERROR_NOTFOUND  2
#define USB_ERROR_BUSY      16
#define USB_ERROR_IO        5

#define USB_VENDOR_ID   0x16c0
#define USB_PRODUCT_ID  0x05df



/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#if defined(WIN32NATIVE) && defined(HAVE_LIBHID)

#include <windows.h>
#include <setupapi.h>

#if defined(HAVE_DDK_HIDSDI_H)
#  include <ddk/hidsdi.h>
#else
#  include "my_ddk_hidsdi.h"
#endif
#include <ddk/hidpi.h>

#ifdef USB_DEBUG
#define DEBUG_PRINT(arg)    printf arg
#else
#define DEBUG_PRINT(arg)
#endif

/* ------------------------------------------------------------------------ */

static void convertUniToAscii(char *buffer)
{
    unsigned short  *uni = (void *)buffer;
    char            *ascii = buffer;

    while(*uni != 0){
        if(*uni >= 256){
            *ascii++ = '?';
            uni++;
        }else{
            *ascii++ = *uni++;
        }
    }
    *ascii++ = 0;
}

static int usbOpenDevice(union filedescriptor *fdp, int vendor, char *vendorName,
			 int product, char *productName, int usesReportIDs)
{
    GUID                                hidGuid;        /* GUID for HID driver */
    HDEVINFO                            deviceInfoList;
    SP_DEVICE_INTERFACE_DATA            deviceInfo;
    SP_DEVICE_INTERFACE_DETAIL_DATA     *deviceDetails = NULL;
    DWORD                               size;
    int                                 i, openFlag = 0;  /* may be FILE_FLAG_OVERLAPPED */
    int                                 errorCode = USB_ERROR_NOTFOUND;
    HANDLE                              handle = INVALID_HANDLE_VALUE;
    HIDD_ATTRIBUTES                     deviceAttributes;

    HidD_GetHidGuid(&hidGuid);
    deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL,
					 DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    deviceInfo.cbSize = sizeof(deviceInfo);
    for(i=0;;i++){
        if(handle != INVALID_HANDLE_VALUE){
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
        if(!SetupDiEnumDeviceInterfaces(deviceInfoList, 0, &hidGuid, i, &deviceInfo))
            break;  /* no more entries */
        /* first do a dummy call just to determine the actual size required */
        SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, NULL, 0, &size, NULL);
        if(deviceDetails != NULL)
            free(deviceDetails);
        deviceDetails = malloc(size);
        deviceDetails->cbSize = sizeof(*deviceDetails);
        /* this call is for real: */
        SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, deviceDetails,
					size, &size, NULL);
        DEBUG_PRINT(("checking HID path \"%s\"\n", deviceDetails->DevicePath));
        /* attempt opening for R/W -- we don't care about devices which can't be accessed */
        handle = CreateFile(deviceDetails->DevicePath, GENERIC_READ|GENERIC_WRITE,
			    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			    openFlag, NULL);
        if(handle == INVALID_HANDLE_VALUE){
            DEBUG_PRINT(("opening failed: %d\n", (int)GetLastError()));
            /* errorCode = USB_ERROR_ACCESS; opening will always fail for mouse -- ignore */
            continue;
        }
        deviceAttributes.Size = sizeof(deviceAttributes);
        HidD_GetAttributes(handle, &deviceAttributes);
        DEBUG_PRINT(("device attributes: vid=%d pid=%d\n",
		     deviceAttributes.VendorID, deviceAttributes.ProductID));
        if(deviceAttributes.VendorID != vendor || deviceAttributes.ProductID != product)
            continue;   /* ignore this device */
        errorCode = USB_ERROR_NOTFOUND;
        if(vendorName != NULL && productName != NULL){
            char    buffer[512];
            if(!HidD_GetManufacturerString(handle, buffer, sizeof(buffer))){
                DEBUG_PRINT(("error obtaining vendor name\n"));
                errorCode = USB_ERROR_IO;
                continue;
            }
            convertUniToAscii(buffer);
            DEBUG_PRINT(("vendorName = \"%s\"\n", buffer));
            if(strcmp(vendorName, buffer) != 0)
                continue;
            if(!HidD_GetProductString(handle, buffer, sizeof(buffer))){
                DEBUG_PRINT(("error obtaining product name\n"));
                errorCode = USB_ERROR_IO;
                continue;
            }
            convertUniToAscii(buffer);
            DEBUG_PRINT(("productName = \"%s\"\n", buffer));
            if(strcmp(productName, buffer) != 0)
                continue;
        }
        break;  /* we have found the device we are looking for! */
    }
    SetupDiDestroyDeviceInfoList(deviceInfoList);
    if(deviceDetails != NULL)
        free(deviceDetails);
    if(handle != INVALID_HANDLE_VALUE){
	fdp->pfd = (void *)handle;
	errorCode = 0;
    }
    return errorCode;
}

/* ------------------------------------------------------------------------ */

static void    usbCloseDevice(union filedescriptor *fdp)
{
    CloseHandle((HANDLE)fdp->pfd);
}

/* ------------------------------------------------------------------------ */

static int usbSetReport(union filedescriptor *fdp, int reportType, char *buffer, int len)
{
    HANDLE  handle = (HANDLE)fdp->pfd;
    BOOLEAN rval = 0;
    DWORD   bytesWritten;

    switch(reportType){
    case USB_HID_REPORT_TYPE_INPUT:
        break;
    case USB_HID_REPORT_TYPE_OUTPUT:
        rval = WriteFile(handle, buffer, len, &bytesWritten, NULL);
        break;
    case USB_HID_REPORT_TYPE_FEATURE:
        rval = HidD_SetFeature(handle, buffer, len);
        break;
    }
    return rval == 0 ? USB_ERROR_IO : 0;
}

/* ------------------------------------------------------------------------ */

static int usbGetReport(union filedescriptor *fdp, int reportType, int reportNumber,
			char *buffer, int *len)
{
    HANDLE  handle = (HANDLE)fdp->pfd;
    BOOLEAN rval = 0;
    DWORD   bytesRead;

    switch(reportType){
    case USB_HID_REPORT_TYPE_INPUT:
        buffer[0] = reportNumber;
        rval = ReadFile(handle, buffer, *len, &bytesRead, NULL);
        if(rval)
            *len = bytesRead;
        break;
    case USB_HID_REPORT_TYPE_OUTPUT:
        break;
    case USB_HID_REPORT_TYPE_FEATURE:
        buffer[0] = reportNumber;
        rval = HidD_GetFeature(handle, buffer, *len);
        break;
    }
    return rval == 0 ? USB_ERROR_IO : 0;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#else /* !(WIN32NATIVE && HAVE_LIBHID) */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#if defined(HAVE_USB_H)
#  include <usb.h>
#elif defined(HAVE_LUSB0_USB_H)
#  include <lusb0_usb.h>
#else
#  error "libusb needs either <usb.h> or <lusb0_usb.h>"
#endif

/* ------------------------------------------------------------------------- */

#define USBRQ_HID_GET_REPORT    0x01
#define USBRQ_HID_SET_REPORT    0x09

const int  avrdoper::reportDataSizes[4] = {13, 29, 61, 125};
/*int  avrdoper::avrdoperRxLength = 0;
int  avrdoper::avrdoperRxPosition = 0;
*/
/* ------------------------------------------------------------------------- */
avrdoper::avrdoper()
{
	avrdoperRxLength = 0;
	avrdoperRxPosition = 0;

	pfd = NULL;
}

avrdoper::~avrdoper()
{
	if(pfd != NULL)
		avrdoper_close();
}

int  avrdoper::usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
    char    buffer[256];
    int     rval, i;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
			       (USB_DT_STRING << 8) + index, langid, buffer,
			       sizeof(buffer), 1000)) < 0)
        return rval;
    if(buffer[1] != USB_DT_STRING)
        return 0;
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for(i=1;i<rval;i++){
        if(i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}

int avrdoper::usbOpenDevice(int vendor, const char *vendorName,
			 int product, const char *productName, int doReportIDs)
{
    struct usb_bus      *bus;
    struct usb_device   *dev;
    usb_dev_handle      *handle = NULL;
    int                 errorCode = USB_ERROR_NOTFOUND;
    static int          didUsbInit = 0;

    if(!didUsbInit){
        usb_init();
        didUsbInit = 1;
    }
    usb_find_busses();
    usb_find_devices();
    for(bus=usb_get_busses(); bus; bus=bus->next){
        for(dev=bus->devices; dev; dev=dev->next){
            if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product){
                char    string[256];
                int     len;
                handle = usb_open(dev); /* we need to open the device in order to query strings */
                if(!handle){
                    errorCode = USB_ERROR_ACCESS;
                    fprintf(stderr, "Warning: cannot open USB device: %s\n",
			    usb_strerror());
                    continue;
                }
                if(vendorName == NULL && productName == NULL){  /* name does not matter */
                    break;
                }
                /* now check whether the names match: */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer,
					0x0409, string, sizeof(string));
                if(len < 0){
                    errorCode = USB_ERROR_IO;
                    fprintf(stderr, "Warning: cannot query manufacturer for device: %s\n",  usb_strerror());
                }else{
                    errorCode = USB_ERROR_NOTFOUND;
                    // fprintf(stderr, "seen device from vendor ->%s<-\n", string);
                    if(strcmp(string, vendorName) == 0){
                        len = usbGetStringAscii(handle, dev->descriptor.iProduct,
						0x0409, string, sizeof(string));
                        if(len < 0){
                            errorCode = USB_ERROR_IO;
                            fprintf(stderr, "Warning: cannot query product for device: %s\n",  usb_strerror());
                        }else{
                            errorCode = USB_ERROR_NOTFOUND;
                            // fprintf(stderr, "seen product ->%s<-\n", string);
                            if(strcmp(string, productName) == 0){
/*                            	len = usbGetStringAscii(handle, dev->descriptor.iSerialNumber, 0x0409, string, sizeof(string));
       	                        if(len > 0)
       	                        	fprintf(stderr,"Serial(%d): %c%c%c%c%c%c\n",len, string[0], string[1], string[2], string[3], string[4], string[5]);
*/                            	break;
                            }
                        }
                    }
                }
                usb_close(handle);
                handle = NULL;
            }
        }
        if(handle)
            break;
    }
    if(handle != NULL){
        int rval, retries = 3;
        if(usb_set_configuration(handle, 1)){
            fprintf(stderr, "Warning: could not set configuration: %s\n",
		    usb_strerror());
        }
        /* now try to claim the interface and detach the kernel HID driver on
         * linux and other operating systems which support the call.
         */
        while((rval = usb_claim_interface(handle, 0)) != 0 && retries-- > 0){
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
            if(usb_detach_kernel_driver_np(handle, 0) < 0){
                fprintf(stderr, "Warning: could not detach kernel HID driver: %s\n",
			usb_strerror());
            }
#endif
        }
        if(rval != 0)
            fprintf(stderr, "Warning: could not claim interface\n");
/* Continue anyway, even if we could not claim the interface. Control transfers
 * should still work.
 */
        errorCode = 0;
        pfd = handle;
        usesReportIDs = doReportIDs;
    }
    return errorCode;
}

/* ------------------------------------------------------------------------- */

int avrdoper::usbSetReport( int reportType, char *buffer, int len)
{
    int bytesSent;

    if(!usesReportIDs){
        buffer++;   /* skip dummy report ID */
        len--;
    }
    bytesSent = usb_control_msg(pfd, USB_TYPE_CLASS |
				USB_RECIP_INTERFACE | USB_ENDPOINT_OUT, USBRQ_HID_SET_REPORT,
				reportType << 8 | buffer[0], 0, buffer, len, 5000);
    if(bytesSent != len){
        if(bytesSent < 0)
            fprintf(stderr, "Error sending message: %s\n", usb_strerror());
        return USB_ERROR_IO;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int avrdoper::usbGetReport( int reportType, int reportNumber,
			char *buffer, int *len)
{
    int bytesReceived, maxLen = *len;

    if(!usesReportIDs){
        buffer++;   /* make room for dummy report ID */
        maxLen--;
    }
    bytesReceived = usb_control_msg(pfd, USB_TYPE_CLASS |
				    USB_RECIP_INTERFACE | USB_ENDPOINT_IN, USBRQ_HID_GET_REPORT,
				    reportType << 8 | reportNumber, 0, buffer, maxLen, 5000);
    if(bytesReceived < 0){
        fprintf(stderr, "Error sending message: %s\n", usb_strerror());
        return USB_ERROR_IO;
    }
    *len = bytesReceived;
    if(!usesReportIDs){
        buffer[-1] = reportNumber;  /* add dummy report ID */
        len++;
    }
    return 0;
}

#endif  /* WIN32NATIVE */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------- */

void avrdoper::dumpBlock(const char *prefix, unsigned char *buf, int len)
{
    int i;

    if(len <= 8){   /* more compact format for short blocks */
        fprintf(stderr, "%s: %d bytes: ", prefix, len);
        for(i = 0; i < len; i++){
            fprintf(stderr, "%02x ", buf[i]);
        }
        fprintf(stderr, " \"");
        for(i = 0; i < len; i++){
            if(buf[i] >= 0x20 && buf[i] < 0x7f){
                fputc(buf[i], stderr);
            }else{
                fputc('.', stderr);
            }
        }
        fprintf(stderr, "\"\n");
    }else{
        fprintf(stderr, "%s: %d bytes:\n", prefix, len);
        while(len > 0){
            for(i = 0; i < 16; i++){
                if(i < len){
                    fprintf(stderr, "%02x ", buf[i]);
                }else{
                    fprintf(stderr, "   ");
                }
                if(i == 7)
                    fputc(' ', stderr);
            }
            fprintf(stderr, "  \"");
            for(i = 0; i < 16; i++){
                if(i < len){
                    if(buf[i] >= 0x20 && buf[i] < 0x7f){
                        fputc(buf[i], stderr);
                    }else{
                        fputc('.', stderr);
                    }
                }
            }
            fprintf(stderr, "\"\n");
            buf += 16;
            len -= 16;
        }
    }
}

const char *avrdoper::usbErrorText(int usbErrno)
{
    static char buffer[32];

    switch(usbErrno){
        case USB_ERROR_NONE:    return "Success.";
        case USB_ERROR_ACCESS:  return "Access denied.";
        case USB_ERROR_NOTFOUND:return "Device not found.";
        case USB_ERROR_BUSY:    return "Device is busy.";
        case USB_ERROR_IO:      return "I/O Error.";
        default:
            sprintf(buffer, "Unknown error %d.", usbErrno);
            return buffer;
    }
}

/* ------------------------------------------------------------------------- */

void avrdoper::avrdoper_close()
{
	usb_close(pfd);
	pfd = NULL;
}

/* ------------------------------------------------------------------------- */

int avrdoper::avrdoper_open()
{
    int rval;
    const char *vname = "obdev.at";
    const char *devname = "AVR-Doper";

    rval = usbOpenDevice(USB_VENDOR_ID, vname, USB_PRODUCT_ID, devname, 1);
    if(rval != 0){
        fprintf(stderr, "avrdoper_open(): %s\n", usbErrorText(rval));
        exit(1);
	//return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */


int  avrdoper::chooseDataSize(int len)
{
    int i;

    for(i = 0; i < sizeof(reportDataSizes)/sizeof(reportDataSizes[0]); i++){
        if(reportDataSizes[i] >= len)
            return i;
    }
    return i - 1;
}

int avrdoper::avrdoper_send( unsigned char *buf, size_t buflen)
{
    if(verbose > 3)
        dumpBlock("Send", buf, buflen);
    while(buflen > 0){
        unsigned char buffer[256];
        int rval, lenIndex = chooseDataSize(buflen);
        int thisLen = buflen > reportDataSizes[lenIndex] ?  reportDataSizes[lenIndex] : buflen;
        buffer[0] = lenIndex + 1;   /* report ID */
        buffer[1] = thisLen;
        memcpy(buffer + 2, buf, thisLen);
        if(verbose > 3)
            fprintf(stderr, "Sending %d bytes data chunk\n", thisLen);
        rval = usbSetReport(USB_HID_REPORT_TYPE_FEATURE, (char *)buffer,
			    reportDataSizes[lenIndex] + 2);
        if(rval != 0){
            fprintf(stderr, "avrdoper_send(): %s\n", usbErrorText(rval));
            exit(1);
        }
        buflen -= thisLen;
        buf += thisLen;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

void avrdoper::avrdoperFillBuffer()
{
    int bytesPending = reportDataSizes[1];  /* guess how much data is buffered in device */

    avrdoperRxPosition = avrdoperRxLength = 0;
    while(bytesPending > 0){
        int len, usbErr, lenIndex = chooseDataSize(bytesPending);
        unsigned char buffer[128];
        len = sizeof(avrdoperRxBuffer) - avrdoperRxLength;  /* bytes remaining */
        if(reportDataSizes[lenIndex] + 2 > len) /* requested data would not fit into buffer */
            break;
        len = reportDataSizes[lenIndex] + 2;
        usbErr = usbGetReport(USB_HID_REPORT_TYPE_FEATURE, lenIndex + 1,
			      (char *)buffer, &len);
        if(usbErr != 0){
            fprintf(stderr, "avrdoperFillBuffer(): %s\n", usbErrorText(usbErr));
            exit(1);
        }
        if(verbose > 3)
            fprintf(stderr, "Received %d bytes data chunk of total %d\n", len - 2, buffer[1]);
        len -= 2;   /* compensate for report ID and length byte */
        bytesPending = buffer[1] - len; /* amount still buffered */
        if(len > buffer[1])             /* cut away padding */
            len = buffer[1];
        if(avrdoperRxLength + len > sizeof(avrdoperRxBuffer)){
            fprintf(stderr, "avrdoperFillBuffer(): internal error: buffer overflow\n");
            exit(1);
        }
        memcpy(avrdoperRxBuffer + avrdoperRxLength, buffer + 2, len);
        avrdoperRxLength += len;
    }
}

int avrdoper::avrdoper_recv( unsigned char *buf, size_t buflen)
{
    unsigned char   *p = buf;
    int             remaining = buflen;

    while(remaining > 0){
        int len, available = avrdoperRxLength - avrdoperRxPosition;
        if(available <= 0){ /* buffer is empty */
            avrdoperFillBuffer();
            continue;
        }
        len = remaining < available ? remaining : available;
        memcpy(p, avrdoperRxBuffer + avrdoperRxPosition, len);
        p += len;
        remaining -= len;
        avrdoperRxPosition += len;
    }
    if(verbose > 3)
        dumpBlock("Receive", buf, buflen);
    return 0;
}

/* ------------------------------------------------------------------------- */

int avrdoper::avrdoper_drain()
{
    do{
        avrdoperFillBuffer();
    }while(avrdoperRxLength > 0);
    return 0;
}


#endif /* defined(HAVE_LIBUSB) || (defined(WIN32NATIVE) && defined(HAVE_LIBHID)) */
