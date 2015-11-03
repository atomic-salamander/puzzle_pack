#include "Ardumod.h"
#include "audio.h"

volatile byte *_tunes_timer1_pin_port;
volatile byte _tunes_timer1_pin_mask;
volatile byte *_tunes_timer3_pin_port;
volatile byte _tunes_timer3_pin_mask;

union
{
  volatile unsigned short sixteen_bits;
  volatile unsigned char eight_bits[2];
} phase1, phase2, duration1;

volatile unsigned short pitch1, pitch2;
volatile signed short slide1;
volatile signed short accel1;
volatile unsigned char method;

/* AUDIO */

void ArduboyAudio::on() {
  duration1.sixteen_bits = 0;
  /*
  power_timer1_enable();
  power_timer3_enable();*/
  audio_enabled = true;
}

bool ArduboyAudio::enabled() {
  return audio_enabled;
}

void ArduboyAudio::off() {
  audio_enabled = false;/*
  power_timer1_disable();
  power_timer3_disable();*/
}

void ArduboyAudio::save_on_off() {
  EEPROM.write(EEPROM_AUDIO_ON_OFF, audio_enabled);
}

void ArduboyAudio::setup() {
  // idk what any of this does
  pinMode(PIN_SPEAKER_1, OUTPUT);
  TCCR1A = 0;
  TCCR1B = 0;
  bitWrite(TCCR1B, WGM12, 1);
  bitWrite(TCCR1B, CS10, 1);
  _tunes_timer1_pin_port = portOutputRegister(digitalPinToPort(PIN_SPEAKER_1));
  _tunes_timer1_pin_mask = digitalPinToBitMask(PIN_SPEAKER_1);
  TCCR1B = (TCCR1B & 0b11111000) | 0b001;
  OCR1A = F_CPU / 4096  - 1;
  bitWrite(TIMSK1, OCIE1A, 1);
  if (EEPROM.read(EEPROM_AUDIO_ON_OFF)) on();
}

void ArduboyAudio::sfx(const SFX_Data * data)
{
  if(audio_enabled == 0) return;
  duration1.sixteen_bits = 0; // this should make this interrupt safe
  phase1.sixteen_bits = 0;
  phase2.eight_bits[0] = 0;
  phase2.eight_bits[1] = B10000000 + (data->pulse_width + 1 << 3);
  method = data->method;
  switch(method)
  {
    case 0:
      pitch1 = data->frequency;
      pitch1 <<= 6;
      slide1 = data->frequency_slide;
      accel1 = data->frequency_accel;
      break;
    case 1:
      pitch1 = 32 - data->frequency;
      phase2.sixteen_bits = pitch1 + (7 - data->pulse_width) * data->frequency / 8;
      pitch1 <<= 6;
      slide1 = 0 - data->frequency_slide;
      accel1 = 0 - data->frequency_accel;
      break;
  }
  pitch2 = pitch1 + data->pulse_freq * 4;
  duration1.sixteen_bits = data->duration << 5; // sound generation is now active
}

ISR(TIMER1_COMPA_vect) {  // TIMER 1
  if(duration1.sixteen_bits)
  {
    if(duration1.eight_bits[0] == 0)
    {
      slide1 += accel1;
    }
    pitch1 += slide1;
    pitch2 += slide1;
    switch(method)
    {
      case 0:
        phase1.sixteen_bits += pitch1;
        if(phase1.eight_bits[1] & B10000000)
        {
          phase1.eight_bits[1] &= B01111111;
          *_tunes_timer1_pin_port ^= _tunes_timer1_pin_mask;
        }
        phase2.sixteen_bits += pitch2;
        if(phase2.eight_bits[1] & B10000000)
        {
          phase2.eight_bits[1] &= B01111111;
          *_tunes_timer1_pin_port ^= _tunes_timer1_pin_mask;
        }
        break;
      case 1:
        phase1.sixteen_bits++;
        if(phase1.sixteen_bits > pitch1 >> 5)
        {
          phase1.sixteen_bits = 0;
          *_tunes_timer1_pin_port ^= _tunes_timer1_pin_mask;
        }
        phase2.sixteen_bits++;
        if(phase2.sixteen_bits > pitch2 >> 5)
        {
          phase2.sixteen_bits = 0;
          *_tunes_timer1_pin_port ^= _tunes_timer1_pin_mask;
        }
        break;
    }
    duration1.sixteen_bits--;
  }
  else
  {
    *_tunes_timer1_pin_port &= ~(_tunes_timer1_pin_mask); // set pin low
  }
}

ISR(TIMER3_COMPA_vect) {  // TIMER 3
  // nothing
}

