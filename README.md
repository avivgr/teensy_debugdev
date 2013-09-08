teensy_debugdev
===============

USB 2.0 Debug cable using teensy

* What is a USB 2.0 Debug Device and what is it used for?

A usb debug device can be used to debug a remote machine through a usb port.
For debugging linux search for the kgdbdbgp or earlyprintk=dbgp kernel parameters.
For using WinDbg and Windows see this guide at http://blogs.msdn.com/b/usbcoreblog/archive/2010/10/25/setting-up-kernel-debugging-with-usb-2-0.aspx

There are existing commercial debug devices for sale, but they cost ~100USD :( this project is aimed to be a DIY replacement using Teensy 2.0.

* What is Teensy ?

http://www.pjrc.com/store/teensy.html Teensy 2.0 is a tiny USB Development Board based on ATMEGA32U4 8 bit AVR 16 MHz chip.
It costs only 16 USD and its a lot of fun to play with. Check out the cool projects using Teensy!

Some useful links related to USB 2.0 debug device:
* http://www.usb.org/developers/presentations/pres0602/john_keys.pdf - this is a ppt explaining usb debug
* google usb2 debug device specification - there is a PDF that describes the USB descriptor requirements for implementing debug device.
* http://lxr.linux.no/linux+v3.11/drivers/usb/early/ehci-dbgp.c - simple code for programming ehci for debug + using debug device for kgdb
* http://lxr.linux.no/linux+v3.11/drivers/usb/serial/usb_debug.c - for using debug as serial, early printk etc
* http://www.pjrc.com/teensy/td_uart.html - project that show how to connect serial to teensy. but - he uses a max chip, i use a TTL cable, much less complicated.
* http://www.coreboot.org/EHCI_Debug_Port - great info, especially the DYI section, includes some FW code. Inspiring!

Possible hardware configirations:

1) USB<->Serial : On the debugee machine, connect teensy to the USB debug port. Then connect teensy through its i/o pins, using serial, to the debugger machine. However, You need to convert the serial signals from Teensy (TTL) to something the remote PC could consume. I use a PL2303HX based TTL Converter Cable that cost 5 USD on eBay (cheaper than 100 USD, right?) [picture that shows how i soldered the pins to connect to the TTL converter]. I used RX/TX and GND (found no need for VSS)

2) USB<->USB : you will need 2 Teensy devices for this. Connect the 2 teensy devices through their i/o pins (serial, cross link).


-----------------------

Current code status: 
* debug device is recognized and configured by the OS
* i can send and receive data over the debug device
* i could not actually use a debugger using it yet. WIP, so please patience (or contribute code) :)
