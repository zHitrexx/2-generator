# 2-generator
Simple firmware for generating the main wave forms, using an 8-bit DAC, parameters configuration via USART for ATmega2560.
Communication via USART implemented with library by Tomas Kolousek.

Default ports for DAC output - PORTK (CH1) and PORTB (CH2)

Terminal setting - 9600 BAUD, 8 data bits, no parity, 1 stop bit
Command format - CH1:SIN:3000:200 [channel:waveform:amplitude:frequency]
