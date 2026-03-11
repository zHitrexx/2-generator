#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "src/usart.h"

#define DDRCH1 DDRK
#define DDRCH2 DDRF
#define CH1 PORTK
#define CH2 PORTF

uint8_t values1[4][256];
uint8_t values2[4][256];
char string[20];
uint8_t index = 0;
const float max_amp = 4000;
const float word_hz = 65536.0 / 40000.0; // 1 Hz
volatile uint16_t phase_ch1 = 0;
volatile uint16_t phase_ch2 = 0;
uint16_t word_ch1 = 0;
uint16_t word_ch2 = 0;
uint8_t* table1 = values1[0];
uint8_t* table2 = values2[3];

//-------Setup-------

void Setup(void)
{
  DDRCH1 = 0xFF;   // CH1 output
  DDRCH2 = 0xFF;   // CH2 output
  usart_setup(9600, bits8, parityNone, stop1);
}

//-------Timer-------

void SetupTimer(void)
{
 TCCR1A = 0;
 TCCR1B = (1 << WGM12) | (1 << CS10); // CTC, prescaler 1
 OCR1A = 16000000 / 40000 - 1;
 TIMSK1 = (1 << OCIE1A);
}

//-------Interrupt-------

ISR(TIMER1_COMPA_vect)
{
 phase_ch1 += word_ch1;
 phase_ch2 += word_ch2;
 
 CH1 = table1[phase_ch1 >> 8]; // Using phase_ch as an index after trimming the lower 8 bits
 CH2 = table2[phase_ch2 >> 8];
}

uint8_t ConvertAmp(float amp)
{
  if (amp > max_amp)
    amp = max_amp;
  else if (amp < 0)
    amp = 0;

  return amp / 1000.0 * 51.0; // Conversion according to the constant 51 = 1V
}

void UpdateTable(uint8_t (*values)[256], float amp, uint16_t *word_ch, float freq)
{
  amp = ConvertAmp(amp); // Conversion of voltage to a value suitable for writing to the port
  float half_amp = amp / 2.0; // Half of the given amplitude for the sine wave
  float angle = 6.28318531 / 255.0;  // Angle for the sine wave

  for (uint16_t i = 0; i < 256; i++)
  {
    values[0][i] = (i < 128) ? amp : 0;                                   // SQR - 0

	values[1][i] = (i * amp) / 255;                                       // SAW - 1
	
	values[2][i] = (i < 128) ? (i * amp) / 127 : ((255 - i) * amp) / 128; // TRI - 2

    values[3][i] = round(half_amp * sin(i * angle) + half_amp);           // SIN - 3
  }
}

void UpdateWord(uint16_t *word_ch, float freq)
{
  *word_ch = round(word_hz * freq);  // Setting the word to add to phase_ch according to the given frequency
}

void ProcessUSART(void) // Processing the command via USART
{
  char *channel  = strtok(string, ":"); // Seperating the parameters 
  char *wave     = strtok(NULL, ":");
  char *str_amp  = strtok(NULL, ":");
  char *str_freq = strtok(NULL, ":");

  if (strcmp(channel, "HELP") == 0)
  {
    if (strcmp(wave, "channel") == 0)
      printf("CH1, CH2\r\n");
    else if (strcmp(wave, "wave") == 0)
      printf("SQR, SAW, TRI, SIN\r\n");
    else if (strcmp(wave, "mV") == 0)
      printf("0-4000\r\n");
    else if (strcmp(wave, "Hz") == 0)
      printf("0-5000\r\n");
    else if (strcmp(wave, "format") == 0)
	  printf("channel:wave:mV:Hz!\r\n");
	else
      printf("HELP:channel | HELP:wave | HELP:mV | HELP:freq | HELP:format\r\n");
    return;
  }

  if (channel == NULL || wave == NULL || str_amp == NULL || str_freq == NULL)
  {
    printf("Missing parameter! Try HELP :)\r\n");
    return;
  }
	
  float amp  = atoi(str_amp);  // Conversion from string to number
  float freq = atoi(str_freq);

  uint8_t wave_index = 0;
  if (strcmp(wave, "SQR") == 0)
    wave_index = 0;
  else if (strcmp(wave, "SAW") == 0)
    wave_index = 1;
  else if (strcmp(wave, "TRI") == 0)
    wave_index = 2;
  else if (strcmp(wave, "SIN") == 0)
    wave_index = 3;
  else
  {
	printf("Wave does not exist! Try HELP :)\r\n");
  	return;
  }

  if (strcmp(channel, "CH1") == 0) // Adjusting values in tables according to the channel and processed parameters
  {
    UpdateTable(values1, amp);
    UpdateWord(&word_ch1, freq);
    table1 = values1[wave_index];
  }
  else if (strcmp(channel, "CH2") == 0)
  {
    UpdateTable(values2, amp);
	UpdateWord(&word_ch2, freq);
    table2 = values2[wave_index]; 
  }
  else
  {
    printf("Channel does not exist! Try HELP :)\r\n");
  }
}

//-------Main-------

int main(void)
{
  Setup(); // Setup AVR PORT
  SetupTimer();
  sei();
  char znak;

  printf("Type HELP or enter a command in the format: channel:wave:mV:Hz!\r\n");

  while(1)
  {
	  if (usart_dataready())
	  {
	    znak = usart_getchar();
	    if (znak == '\r' || znak == '\n')
	    {
	      if (index == 0)
		      continue;
        string[index] = '\0';
        ProcessUSART();
		index = 0;
	    }
	    else
	    {
	      if (index < 19)
		    {
		      string[index] = znak;
		      index++;
	      }
	    }
	  }
  }
  return 0;
}


