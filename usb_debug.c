/* USB Keyboard Example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release
// Version 1.1: Add support for Teensy 2.0

#define USB_SERIAL_PRIVATE_INCLUDE
#include "usb_debug.h"

#define LED_CONFIG      (DDRD |= (1<<6))
#define LED_ON          (PORTD &= ~(1<<6))
#define LED_OFF         (PORTD |= (1<<6))
/**************************************************************************
 *
 *  Configurable Options
 *
 **************************************************************************/

// You can change these to give your code its own name.
#define STR_MANUFACTURER	L"Manufacturer"
#define STR_PRODUCT		L"USB 2.0 Debug Cable"
#define STR_SERIAL L"002"

/* These are recognized by all major OSs as debug device */
#define VENDOR_ID		0x0525 
#define PRODUCT_ID		0x127a 


// USB devices are supposed to implment a halt feature, which is
// rarely (if ever) used.  If you comment this line out, the halt
// code will be removed, saving 102 bytes of space (gcc 4.3.0).
// This is not strictly USB compliant, but works with all major
// operating systems.
#define SUPPORT_ENDPOINT_HALT



/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/

#define ENDPOINT0_SIZE		64

#define USB_DEBUG_INTERFACE	0


#define DEBUG_RX_ENDPOINT	3
#define DEBUG_RX_SIZE		32
#define DEBUG_RX_BUFFER	EP_DOUBLE_BUFFER

#define DEBUG_TX_ENDPOINT	2
#define DEBUG_TX_SIZE		64
#define DEBUG_TX_BUFFER		EP_DOUBLE_BUFFER

static const uint8_t PROGMEM endpoint_config_table[] = {
	0,
	1, EP_TYPE_BULK_IN,  EP_SIZE(DEBUG_TX_SIZE) | DEBUG_TX_BUFFER,
	1, EP_TYPE_BULK_OUT,  EP_SIZE(DEBUG_RX_SIZE) | DEBUG_RX_BUFFER,	
	0
};
static volatile uint8_t transmit_flush_timer=0;
static uint8_t transmit_previous_timeout=0;
#define TRANSMIT_FLUSH_TIMEOUT	5 
#define TRANSMIT_TIMEOUT	25   /* in milliseconds */

/**************************************************************************
 *
 *  Descriptor Data
 *
 **************************************************************************/

// Descriptors are the data that your computer reads when it auto-detects
// this USB device (called "enumeration" in USB lingo).  The most commonly
// changed items are editable at the top of this file.  Changing things
// in here should only be done by those who've read chapter 9 of the USB
// spec and relevant portions of any USB class specifications!


static uint8_t PROGMEM device_descriptor[] = {
	18,					// bLength
	1,					// bDescriptorType
	0x00, 0x02,				// bcdUSB
	255,					// bDeviceClass
	0,					// bDeviceSubClass
	0,					// bDeviceProtocol
	ENDPOINT0_SIZE,				// bMaxPacketSize0
	LSB(VENDOR_ID), MSB(VENDOR_ID),		// idVendor
	LSB(PRODUCT_ID), MSB(PRODUCT_ID),	// idProduct
	0x01, 0x01,				// bcdDevice
	1,					// iManufacturer
	2,					// iProduct
	3,					// iSerialNumber
	1					// bNumConfigurations
};

static uint8_t PROGMEM device_qualifier_desc[] = {
     10,          //bLength            
      6,          //bDescriptorType    
     0x00, 0x02, // bcdUSB 
    255,          //bDeviceClass        Vendor Specific Class
      0,          //bDeviceSubClass     
      0,          //bDeviceProtocol     
     ENDPOINT0_SIZE,          //bMaxPacketSize0    
      1,          //bNumConfigurations
      0
};

// Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60
static uint8_t PROGMEM debug_desc[] = {
    4,					// bLength
	10,					// bDescriptorType
	DEBUG_TX_ENDPOINT | 0x80, // bDebugInEndpoint          
	DEBUG_RX_ENDPOINT	// bDebugOutEndpoint   
};

#define CONFIG1_DESC_SIZE        (9+9+7+7)
#define USB_DEBUG_DESC_OFFSET (9+9)
static uint8_t PROGMEM config1_descriptor[CONFIG1_DESC_SIZE] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	2,					// bDescriptorType;
	LSB(CONFIG1_DESC_SIZE),			// wTotalLength
	MSB(CONFIG1_DESC_SIZE),
	1,					// bNumInterfaces
	1,					// bConfigurationValue
	0,					// iConfiguration
	0xA0,					// bmAttributes
	50,					// bMaxPower
	// interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
	9,					// bLength
	4,					// bDescriptorType
	USB_DEBUG_INTERFACE,// bInterfaceNumber
	0,					// bAlternateSetting
	2,					// bNumEndpoints
	0xff,				// bInterfaceClass (0x03 = HID)
	0x00,				// bInterfaceSubClass (0x01 = Boot)
	0x00,				// bInterfaceProtocol (0x01 = Keyboard)
	0,					// iInterface
	// Endpoint 1
	7,					// bLength
	5,					// bDescriptorType
	DEBUG_RX_ENDPOINT,	// bEndpointAddress     
	2,					// bmAttributes            
	DEBUG_RX_SIZE,0x00,// wMaxPacketSize
	0,					// bInterval  
	// Endpoint 2
	7,					// bLength
	5,					// bDescriptorType
	DEBUG_TX_ENDPOINT | 0x80,// bEndpointAddress     
	2,					// bmAttributes            
	DEBUG_TX_SIZE,0x00,	// wMaxPacketSize
	0					// bInterval  
};

static uint8_t PROGMEM other_speed_descriptor[9] = {
	// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
	9, 					// bLength;
	7,					// bDescriptorType;
	9,0,		// wTotalLength
	1,					// bNumInterfaces
	1,					// bConfigurationValue
	0,					// iConfiguration
	0xA0,					// bmAttributes
	50,					// bMaxPower
};
// If you're desperate for a little extra code memory, these strings
// can be completely removed if iManufacturer, iProduct, iSerialNumber
// in the device desciptor are changed to zeros.
struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	int16_t wString[];
};
static struct usb_string_descriptor_struct PROGMEM string0 = {
	4,
	3,
	{0x0409}
};
static struct usb_string_descriptor_struct PROGMEM string1 = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};
static struct usb_string_descriptor_struct PROGMEM string2 = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};
static struct usb_string_descriptor_struct PROGMEM string3 = {
	sizeof(STR_SERIAL),
	3,
	STR_SERIAL
};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
static struct descriptor_list_struct {
	uint16_t	wValue;
	uint16_t	wIndex;
	const uint8_t	*addr;
	uint8_t		length;
} PROGMEM descriptor_list[] = {
	{0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
	{0x0200, 0x0000, config1_descriptor, sizeof(config1_descriptor)},
	{0x0700, 0x0000, other_speed_descriptor, sizeof(other_speed_descriptor)},
	{0x0A00, 0x0000, debug_desc, sizeof(debug_desc)},
	{0x0600, 0x0000, device_qualifier_desc, sizeof(device_qualifier_desc)},
	{0x0300, 0x0000, (const uint8_t *)&string0, 4},
	{0x0301, 0x0409, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x0302, 0x0409, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},
	{0x0303, 0x0409, (const uint8_t *)&string3, sizeof(STR_SERIAL)}
};
#define NUM_DESC_LIST (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))


/**************************************************************************
 *
 *  Variables - these are the only non-stack RAM usage
 *
 **************************************************************************/

// zero when we are not configured, non-zero when enumerated
static volatile uint8_t usb_configuration=0;


/**************************************************************************
 *
 *  Public Functions - these are the API intended for the user
 *
 **************************************************************************/


// initialize USB
void usb_init(void)
{
	HW_CONFIG();
	USB_FREEZE();	// enable USB
	PLL_CONFIG();				// config PLL
        while (!(PLLCSR & (1<<PLOCK))) ;	// wait for PLL lock
        USB_CONFIG();				// start USB clock
        UDCON = 0;				// enable attach resistor
	usb_configuration = 0;
        UDIEN = (1<<EORSTE)|(1<<SOFE);
        //UDADDR = 127 | (1<<ADDEN);
	sei();
}

// return 0 if the USB is not configured, or the configuration
// number selected by the HOST
uint8_t usb_configured(void)
{
	return usb_configuration;
}

// transmit a character.  0 returned on success, -1 on error
int8_t usb_putchar(uint8_t c)
{
	uint8_t timeout, intr_state;

	// if we're not online (enumerated and configured), error
	if (!usb_configuration) return -1;
	// interrupts are disabled so these functions can be
	// used from the main program or interrupt context,
	// even both in the same program!
	intr_state = SREG;
	cli();
	UENUM = DEBUG_TX_ENDPOINT;
	// if we gave up due to timeout before, don't wait again
	if (transmit_previous_timeout) {
		if (!(UEINTX & (1<<RWAL))) {
			SREG = intr_state;
			return -1;
		}
		transmit_previous_timeout = 0;
	}
	// wait for the FIFO to be ready to accept data
	timeout = UDFNUML + TRANSMIT_TIMEOUT;
	while (1) {
		// are we ready to transmit?
		if (UEINTX & (1<<RWAL)) break;
		SREG = intr_state;
		// have we waited too long?  This happens if the user
		// is not running an application that is listening
		if (UDFNUML == timeout) {
			transmit_previous_timeout = 1;
			return -1;
		}
		// has the USB gone offline?
		if (!usb_configuration) return -1;
		// get ready to try checking again
		intr_state = SREG;
		cli();
		UENUM = DEBUG_TX_ENDPOINT;
	}
	// actually write the byte into the FIFO
	UEDATX = c;
	// if this completed a packet, transmit it now!
	if (!(UEINTX & (1<<RWAL))) UEINTX = 0x3A;
	transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	SREG = intr_state;
	return 0;
}

int8_t usb_putchar_nowait(uint8_t c)
{
	uint8_t intr_state;

	if (!usb_configuration) return -1;
	intr_state = SREG;
	cli();
	UENUM = DEBUG_TX_ENDPOINT;
	if (!(UEINTX & (1<<RWAL))) {
		// buffer is full
		SREG = intr_state;
		return -1;
	}
	// actually write the byte into the FIFO
	UEDATX = c;
	// if this completed a packet, transmit it now!
	if (!(UEINTX & (1<<RWAL))) UEINTX = 0x3A;
	transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	SREG = intr_state;
	return 0;
}

// get the next character, or -1 if nothing received
uint8_t usb_getchar(void)
{
	uint8_t c, intr_state;

	// interrupts are disabled so these functions can be
	// used from the main program or interrupt context,
	// even both in the same program!
	intr_state = SREG;
	cli();
	if (!usb_configuration) {
		SREG = intr_state;
		return -1;
	}
	UENUM = DEBUG_RX_ENDPOINT;
	retry:
	c = UEINTX;
	if (!(c & (1<<RWAL))) {
		// no data in buffer
		if (c & (1<<RXOUTI)) {
			UEINTX = 0x6B;
			goto retry;
		}	
		SREG = intr_state;
		return -1;
	}
	// take one byte out of the buffer
	c = UEDATX;
	// if buffer completely used, release it
	if (!(UEINTX & (1<<RWAL))) UEINTX = 0x6B;
	SREG = intr_state;
	return c;
}

// number of bytes available in the receive buffer
uint8_t usb_available(void)
{
	uint8_t n=0, i, intr_state;

	intr_state = SREG;
	cli();
	if (usb_configuration) {
		UENUM = DEBUG_RX_ENDPOINT;
		n = UEBCLX;
		if (!n) {
			i = UEINTX;
			if (i & (1<<RXOUTI) && !(i & (1<<RWAL))) UEINTX = 0x6B;
		}
	}
	SREG = intr_state;
	return n;
}

/**************************************************************************
 *
 *  Private Functions - not intended for general user consumption....
 *
 **************************************************************************/

// USB Device Interrupt - handle all device-level events
// the transmit buffer flushing is triggered by the start of frame
//
ISR(USB_GEN_vect)
{

	uint8_t intbits, t;
	
        intbits = UDINT;
        UDINT = 0;
        if (intbits & (1<<EORSTI)) {
			UENUM = 0;
			UECONX = 1;
			UECFG0X = EP_TYPE_CONTROL;
			UECFG1X = EP_SIZE(ENDPOINT0_SIZE) | EP_SINGLE_BUFFER;
			UEIENX = (1<<RXSTPE);
			//UDADDR = 127 | (1<<ADDEN);
			usb_configuration = 0;
			LED_OFF;
        }
		if ((intbits & (1<<SOFI)) /*&& usb_configuration*/) {
			if (usb_configuration) {
				t = transmit_flush_timer;
				if (t) {
					transmit_flush_timer = --t;
					if (!t) {
						UENUM = DEBUG_TX_ENDPOINT;
						UEINTX = 0x3A;
					}
				}
			}		
		}

}



// Misc functions to wait for ready and send/receive packets
static inline void usb_wait_in_ready(void)
{
	while (!(UEINTX & (1<<TXINI))) ;
}
static inline void usb_send_in(void)
{
	UEINTX = ~(1<<TXINI);
}
static inline void usb_wait_receive_out(void)
{
	while (!(UEINTX & (1<<RXOUTI))) ;
}
static inline void usb_ack_out(void)
{
	UEINTX = ~(1<<RXOUTI);
}



// USB Endpoint Interrupt - endpoint 0 is handled here.  The
// other endpoints are manipulated by the user-callable
// functions, and the start-of-frame interrupt.
//
ISR(USB_COM_vect)
{
        uint8_t intbits;
	const uint8_t *list;
        const uint8_t *cfg;
	uint8_t i, n, len, en;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint16_t desc_val;
	const uint8_t *desc_addr;
	uint8_t	desc_length;

        UENUM = 0;
	intbits = UEINTX;
        if (intbits & (1<<RXSTPI)) {
                bmRequestType = UEDATX;
                bRequest = UEDATX;
                wValue = UEDATX;
                wValue |= (UEDATX << 8);
                wIndex = UEDATX;
                wIndex |= (UEDATX << 8);
                wLength = UEDATX;
                wLength |= (UEDATX << 8);
                UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI));
                if (bRequest == GET_DESCRIPTOR) {
					list = (const uint8_t *)descriptor_list;
					for (i=0; ; i++) {
						if (i >= NUM_DESC_LIST) {
							UECONX = (1<<STALLRQ)|(1<<EPEN);  //stall
							return;
						}
						desc_val = pgm_read_word(list);
						if (desc_val != wValue) {
							list += sizeof(struct descriptor_list_struct);
							continue;
						}
						list += 2;
						desc_val = pgm_read_word(list);
						if (desc_val != wIndex) {
							list += sizeof(struct descriptor_list_struct)-2;
							continue;
						}
						list += 2;
						desc_addr = (const uint8_t *)pgm_read_word(list);
						list += 2;
						desc_length = pgm_read_byte(list);
						break;
					}
					len = (wLength < 256) ? wLength : 255;
					if (len > desc_length) len = desc_length;
					do {
						// wait for host ready for IN packet
						do {
							i = UEINTX;
						} while (!(i & ((1<<TXINI)|(1<<RXOUTI))));
						if (i & (1<<RXOUTI)) return;	// abort
						// send IN packet
						n = len < ENDPOINT0_SIZE ? len : ENDPOINT0_SIZE;
						for (i = n; i; i--) {
							UEDATX = pgm_read_byte(desc_addr++);
						}
						len -= n;
						usb_send_in();
					} while (len || n == ENDPOINT0_SIZE);
					return;
                }
		if (bRequest == SET_ADDRESS) {
			usb_send_in();
			usb_wait_in_ready();
			UDADDR = wValue | (1<<ADDEN);
			return;
		}
		if (bRequest == SET_FEATURE && bmRequestType == 0 
			&& wValue == DEBUG_MODE) {
			usb_configuration = 1;
			LED_ON;
			usb_send_in();
			/* BUGBUG: Not sure i need to re-configure the device here */
			cfg = endpoint_config_table;
			for (i=1; i<5; i++) {
				UENUM = i;
				en = pgm_read_byte(cfg++);
				UECONX = en;
				if (en) {
					UECFG0X = pgm_read_byte(cfg++);
					UECFG1X = pgm_read_byte(cfg++);
				}
			}
        		UERST = 0x1E;
        		UERST = 0;
			return;
		}
		
		if (bRequest == SET_CONFIGURATION && bmRequestType == 0) {
			usb_configuration = wValue;
			usb_send_in();
			cfg = endpoint_config_table;
			for (i=1; i<5; i++) {
				UENUM = i;
				en = pgm_read_byte(cfg++);
				UECONX = en;
				if (en) {
					UECFG0X = pgm_read_byte(cfg++);
					UECFG1X = pgm_read_byte(cfg++);
				}
			}
        		UERST = 0x1E;
        		UERST = 0;
			return;
		}
		if (bRequest == GET_CONFIGURATION && bmRequestType == 0x80) {
			usb_wait_in_ready();
			UEDATX = usb_configuration;
			usb_send_in();
			return;
		}

		if (bRequest == GET_STATUS) {
			usb_wait_in_ready();
			i = 0;
			#ifdef SUPPORT_ENDPOINT_HALT
			if (bmRequestType == 0x82) {
				UENUM = wIndex;
				if (UECONX & (1<<STALLRQ)) i = 1;
				UENUM = 0;
			}
			#endif
			UEDATX = i;
			UEDATX = 0;
			usb_send_in();
			return;
		}
		#ifdef SUPPORT_ENDPOINT_HALT
		if ((bRequest == CLEAR_FEATURE || bRequest == SET_FEATURE)
		  && bmRequestType == 0x02 && wValue == 0) {
			i = wIndex & 0x7F;
			if (i >= 1 && i <= MAX_ENDPOINT) {
				usb_send_in();
				UENUM = i;
				if (bRequest == SET_FEATURE) {
					UECONX = (1<<STALLRQ)|(1<<EPEN);
				} else {
					UECONX = (1<<STALLRQC)|(1<<RSTDT)|(1<<EPEN);
					UERST = (1 << i);
					UERST = 0;
				}
				return;
			}
		}
		#endif
		if (wIndex == USB_DEBUG_INTERFACE) {
			usb_wait_in_ready();
			UEDATX = 0;
			usb_send_in();
			return;
			
			#if 0
			if (bmRequestType == 0xA1) {
				if (bRequest == HID_GET_REPORT) {
					usb_wait_in_ready();
					UEDATX = keyboard_modifier_keys;
					UEDATX = 0;
					for (i=0; i<6; i++) {
						UEDATX = keyboard_keys[i];
					}
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_IDLE) {
					usb_wait_in_ready();
					UEDATX = keyboard_idle_config;
					usb_send_in();
					return;
				}
				if (bRequest == HID_GET_PROTOCOL) {
					usb_wait_in_ready();
					UEDATX = keyboard_protocol;
					usb_send_in();
					return;
				}
			}
			if (bmRequestType == 0x21) {
				if (bRequest == HID_SET_REPORT) {
					usb_wait_receive_out();
					keyboard_leds = UEDATX;
					usb_ack_out();
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_IDLE) {
					keyboard_idle_config = (wValue >> 8);
					keyboard_idle_count = 0;
					usb_send_in();
					return;
				}
				if (bRequest == HID_SET_PROTOCOL) {
					keyboard_protocol = wValue;
					usb_send_in();
					return;
				}

			}
			#endif

		}

	}
	UECONX = (1<<STALLRQ) | (1<<EPEN);	// stall
}


