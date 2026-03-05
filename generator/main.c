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

const uint8_t max_amp = 204;
uint8_t values[4][max_amp];
volatile uint8_t step_ch1 = 0;
volatile uint8_t step_ch2 = 0;
float amp = 4000;
uint8_t amp_dac = 0;
float freq1 = 600;
float freq2 = 600;

//-------Setup-------

void Setup()
{
  DDRCH1 = 0xFF;   // Nastavení PORTB na výstupní - CH1
  DDRCH2 = 0xFF;   // Nastavení PORTK na výstupní - CH2
  CH1 = 0x00;  // Nastavení log. 0 na celý PORTB
  CH2 = 0x00;  // Nastavení log. 0 na celý PORTK
  amp_dac = Amp(amp);
}
void SetupTimer()
{
 TCCR1A = 0; // Ohlídání WGM10 a WGM11 LOW a zároveň COM1Xn v LOW (hw piny)
 TCCR1B = (1 << WGM12) | (1 << CS10); // Nastavení TIMER1 na CTC a Prescaler na 1
 OCR1A = 16000000 / (100.0 * 2.04 * freq1) - 1; // Nastavení na (147456MHz / 10 000 Hz - 1 = ?
 TIMSK1 = (1 << OCIE1A); // Povolit přerušení pri shodě s TCNT1 a OCR1A

 TCCR3A = 0;
 TCCR3B = (1 << WGM32) | (1 << CS30);
 OCR3A = 16000000 / (100.0 * 2.04 * freq2) - 1;
 TIMSK3 = (1 << OCIE3A);//ETIMSK = (1 << OCIE3A);
}

ISR(TIMER1_COMPA_vect) // Přerušení vyvolané TIMER1
{
  CH1 = values[0][step_ch1];
  step_ch1++;
  if (step_ch1 >= max_amp)
	  step_ch1 = 0;
}
ISR(TIMER3_COMPA_vect) // Přerušení vyvolané TIMER3
{
  CH2 = values[2][step_ch2];
  step_ch2++;
  if (step_ch2 >= max_amp)
	  step_ch2 = 0;
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
  for (uint8_t i = 0; i < max_amp; i++)  // SQR
  {
    if (i < 102)
      values[0][i] = amp_dac;
    else
      values[0][i] = 0; 
  }
  for (uint8_t i = 0; i < max_amp; i++) // SAW
  {
    values[1][i] = i;
  }
  for (uint8_t i = 0; i <= max_amp / 2; i++) // TRI
  {
	values[2][i] = i * 2;
  }
  for (uint8_t i = max_amp / 2; i < max_amp; i++)
  {
	values[2][i] = values[2][max_amp - i];
  }
  for (uint8_t i = 0; i < max_amp; i++)  // SIN
  {
    float angle = 6.28318531 * ((float)i / amp_dac);
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
