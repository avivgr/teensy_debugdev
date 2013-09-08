teensy_debugdev
===============

USB 2.0 Debug cable using teensy

* What is a USB 2.0 Debug Device and what is it used for?

A usb debug device can be used to debug a remote machine through a usb port.
In linux search for kgdbdbgp or earlyprintk=dbgp kernel params.
In Windows see http://blogs.msdn.com/b/usbcoreblog/archive/2010/10/25/setting-up-kernel-debugging-with-usb-2-0.aspx

There are existing debug devices for sale, but these cost ~100USD and this project is aimed to be a DIY replacement using Teensy 2.0.

Some useful links:
http://www.usb.org/developers/presentations/pres0602/john_keys.pdf
google usb2 debug device specification

Possible hardware configirations:

1) Connect teensy to the debug port using USB, connect teensy through its i/o pins to serial to the debugger machine. However,  You need to convert the serial signals from TTL to something the PC can consume. I use a PL2303HX based TTL Converter Cable that cost 5 USD on eBay (cheaper than 100 USD, right?)

2) Connect 2 teensy debug devices through their i/o pins (serial, cross link)

For config #1 i have a picture that shows how i soldered the pins to connect to the TTL converter. I used RX/TX and GND (no need for VSS)

Current status: i can send and receive data over the debug device, but i could not actually use a debugger using it yet. WIP, so please patience :)
