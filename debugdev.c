
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "uart.h"
#include "usb_debug.h"

#define BAUD_RATE 38400
#define LED_CONFIG      (DDRD |= (1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

// write a string to the uart
#define uart_print(s) uart_print_P(PSTR(s))
void uart_print_P(const char *str)
{
	char c;
	while (1) {
		c = pgm_read_byte(str++);
		if (!c) break;
		uart_putchar(c);
	}
}

// A very basic example...
// when the user types a character, print it back
int main(void)
{
	uint8_t c;
	uint32_t cnt = 0;

	CPU_PRESCALE(0);  // run at 16 MHz
	LED_CONFIG;
	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	//while (!usb_configured()) /* wait */ ;

	uart_init(BAUD_RATE);
	
	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	while (1) {
		/* BUGBUG : need to read/write in chunks and not char at a time */
		if (uart_available()) {
			c = uart_getchar();
			usb_putchar(c);
		}
		if(usb_available()) {
			c = usb_getchar();
			uart_putchar(c);
		}
		
	}
}
