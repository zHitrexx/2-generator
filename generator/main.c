#define F_CPU 16000000UL //14745600UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <math.h>
#include "src/BitOps.h" // vložit src do Dokumenty -> Arduiono -> sketch složka -> src -> BitOps.h a .c
#include "src/usart.h"

#define DDRCH1 DDRK
#define DDRCH2 DDRF
#define CH1 PORTK
#define CH2 PORTF

uint8_t values[4][256];
const uint8_t max_amp = 204;
const uint32_t word_hz = 214748 / 2;
volatile uint32_t phase_ch1 = 0;
volatile uint32_t phase_ch2 = 0;
volatile uint32_t word_ch1 = word_hz * 300; // 143165 = 1 Hz -> 143165 * freq
volatile uint32_t word_ch2 = word_hz * 200;
volatile uint8_t index_ch1 = 0;
volatile uint8_t index_ch2 = 0;
volatile uint8_t* table1 = values[3];
volatile uint8_t* table2 = values[2];
float amp = 4000;
uint8_t amp_dac = amp * 51 / 1000;

//-------Setup-------

void Setup()
{
  DDRCH1 = 0xFF;   // Nastavení PORTB na výstupní - CH1
  DDRCH2 = 0xFF;   // Nastavení PORTK na výstupní - CH2
  CH1 = 0x00;  // Nastavení log. 0 na celý PORTB
  CH2 = 0x00;  // Nastavení log. 0 na celý PORTK
}
void SetupTimer()
{
 TCCR1A = 0; // Ohlídání WGM10 a WGM11 LOW a zároveň COM1Xn v LOW (hw piny)
 TCCR1B = (1 << WGM12) | (1 << CS10); // Nastavení TIMER1 na CTC a Prescaler na 1
 OCR1A = 16000000 / (40000) - 1; // Nastavení na (147456MHz / 10 000 Hz - 1 = ?
 TIMSK1 = (1 << OCIE1A); // Povolit přerušení pri shodě s TCNT1 a OCR1A
}

ISR(TIMER1_COMPA_vect) // Přerušení vyvolané TIMER1
{
 phase_ch1 += word_ch1;
 phase_ch2 += word_ch2;
 
 index_ch1 = phase_ch1 >> 24;
 index_ch2 = phase_ch2 >> 24;

 CH1 = table1[index_ch1];
 CH2 = table2[index_ch2];
}

uint8_t Amp(int amp)
{
  if ((float)(amp) / 1000.0 > (float)max_amp / 51.0)   // 4500 [mV] / 1000 -> 4.5[V] > 4V
    amp = (float)max_amp * 1000.0 / 51.0;                // 204 000 / 51 -> 4000[mV]
  else if (amp < 0)
    amp = 0;

  return (float)amp / 1000.0 * 51.0; // převedení z mV na V a potom na 8bitovou hodnotu (0 - 204)
}

float Period(float freq)
{
  return 1.0 / freq; // f -> T 
}

void SetupValues()
{
  for (uint16_t i = 0; i < 256; i++)  // SQR
  {
    if (i < 102)
      values[0][i] = amp_dac;
    else
      values[0][i] = 0; 
  }
  for (uint16_t i = 0; i < 256; i++) // SAW
  {
    values[1][i] = (i * amp_dac) / 256;
  }
  for (uint16_t i = 0; i <= 256 / 2; i++) // TRI
  {
	values[2][i] = (i * 2) * amp_dac / 256;
  }
  for (uint16_t i = 256 / 2; i < 256; i++)
  {
	values[2][i] = values[2][256 - i];
  }
  for (uint16_t i = 0; i < 256; i++)  // SIN
  {
    float angle = 6.28318531 * ((float)i / 256.0);
    values[3][i] = round((float)amp_dac / 2.0 * sin(angle) + (float)amp_dac / 2.0);
  }
}

void Off()
{
  CH1 = 0x00;
  CH2 = 0x00;
}

//-------Main-------
 


int main()
{
  Setup(); // Setup AVR PORT
  SetupTimer();
  SetupValues();
  sei();

  while(true)
  {
   // Vymyslet ten usart
   // DDS?
   // 
  }

  
  return 0;
}
