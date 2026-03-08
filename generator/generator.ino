#define F_CPU 16000000UL //14745600UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "src/BitOps.h"
#include "src/usart.h"

#define DDRCH1 DDRK
#define DDRCH2 DDRF
#define CH1 PORTK
#define CH2 PORTF

uint8_t values1[4][256];
uint8_t values2[4][256];
char string[20];
uint8_t index = 0;
const uint8_t max_amp = 204;
const uint32_t word_hz = 107374; // 1 Hz // 214748 -> 20 kHz base
volatile uint32_t phase_ch1 = 0;
volatile uint32_t phase_ch2 = 0;
volatile uint32_t word_ch1 = 0;
volatile uint32_t word_ch2 = 0;
volatile uint8_t* table1 = values1[0];
volatile uint8_t* table2 = values2[3];

//-------Setup-------

void Setup(void)
{
  DDRCH1 = 0xFF;   // CH1 výstup
  DDRCH2 = 0xFF;   // CH2 výstup
  usart_setup(9600, bits8, parityNone, stop1);
}

//-------Timer-------

void SetupTimer(void)
{
 TCCR1A = 0;
 TCCR1B = (1 << WGM12) | (1 << CS10);
 OCR1A = 16000000 / 40000 - 1; // 147456MHz / 40 000 - 1 (20 000 když by to dělalo brikule ale zdvojnásobit word_hz)
 TIMSK1 = (1 << OCIE1A); // TIMSK = (1 << OCIE1A);
}

//-------Interrupt-------

ISR(TIMER1_COMPA_vect)
{
 phase_ch1 += word_ch1;
 phase_ch2 += word_ch2;
 
 CH1 = table1[phase_ch1 >> 24];
 CH2 = table2[phase_ch2 >> 24];
}

uint8_t Amp(float amp)
{
  if (amp / 1000.0 > max_amp / 51)
    amp = (float)max_amp * 1000.0 / 51.0;
  else if (amp < 0)
    amp = 0;

  return (float)amp / 1000.0 * 51.0;
}

void UpdateTable(uint8_t (*values)[256], uint8_t amp, uint32_t *word_ch, float freq)
{
  *word_ch = word_hz * freq;
  float half_amp = (float)amp / 2.0;
  float angle = 6.28318531 / 255.0; 

  for (uint16_t i = 0; i < 256; i++)
  {
	// SQR
    if (i < 128)
      values[0][i] = amp;
    else
      values[0][i] = 0; 
			
	// SAW
	values[1][i] = (i * amp) / 255;
	
	// TRI
	if (i < 128) 
	  values[2][i] = (i * amp) / 127;
	else
	  values[2][i] = ((255 - i) * amp) / 128;
	
	// SIN
    values[3][i] = round(half_amp * sin(i * angle) + half_amp);
  }
}

void ProcessUSART(void) // Zpracování příkazu přes USART
{
  char *channel  = strtok(string, ":"); // Rozklad na části
  char *wave     = strtok(NULL, ":");
  char *str_amp  = strtok(NULL, ":");
  char *str_freq = strtok(NULL, ":");

  if (channel == NULL || wave == NULL || str_amp == NULL || str_freq == NULL)
	return;
  
  float amp = atoi(str_amp); // Převod na číslo
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
	printf("Spatny druh vlny!");
  	return;
  }

  if (strcmp(channel, "CH1") == 0) // Úprava hodnot v tabulkách podle kanálu 
  {
    UpdateTable(values1, Amp(amp), &word_ch1, freq);
    table1 = values1[wave_index];
  }
  else if (strcmp(channel, "CH2") == 0)
  {
    UpdateTable(values2, Amp(amp), &word_ch2, freq);
    table2 = values2[wave_index]; 
  }
  else
  {
    printf("Spatny kanal!");
  }
}

//-------Main-------

int main()
{
  Setup(); // Setup AVR PORT
  SetupTimer();
  sei();
  char znak;

  printf("Zadej prikaz ve formatu: channel:druh vlny:mV:Hz!\r\n");

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
