#include "sfx_audio.h"

volatile byte *_timer1_pin_port;
volatile byte _timer1_pin_mask;

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

void SFXAudio::on() {
  duration1.sixteen_bits = 0;
  audio_enabled = true;
}

bool SFXAudio::enabled() {
  return audio_enabled;
}

void SFXAudio::off() {
  audio_enabled = false;
}

void SFXAudio::save_on_off() {
  EEPROM.write(EEPROM_AUDIO_ON_OFF, audio_enabled);
}

void SFXAudio::begin() {
  // idk what any of this does
  //power_timer1_enable(); // doesn't seem to make any difference
  pinMode(PIN_SPEAKER_1, OUTPUT);
  pinMode(PIN_SPEAKER_2, OUTPUT); // need to enable both speaker pins, even if you only use 1 :)
  _timer1_pin_port = portOutputRegister(digitalPinToPort(PIN_SPEAKER_1));
  _timer1_pin_mask = digitalPinToBitMask(PIN_SPEAKER_1);
  TCCR1A = 0;
  TCCR1B = 1 << WGM12;
  TCCR1B |= 1 << CS10;
  TCCR1B = (TCCR1B & 0b11111000) | 0b001;
  OCR1A = F_CPU / 4096 - 1;
  TIMSK1 = 1 << OCIE1A;
  if (EEPROM.read(EEPROM_AUDIO_ON_OFF)) on();
}

void SFXAudio::sfx(const SFX_Data * data)
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
          *_timer1_pin_port ^= _timer1_pin_mask;
        }
        phase2.sixteen_bits += pitch2;
        if(phase2.eight_bits[1] & B10000000)
        {
          phase2.eight_bits[1] &= B01111111;
          *_timer1_pin_port ^= _timer1_pin_mask;
        }
        break;
      case 1:
        phase1.sixteen_bits++;
        if(phase1.sixteen_bits > pitch1 >> 5)
        {
          phase1.sixteen_bits = 0;
          *_timer1_pin_port ^= _timer1_pin_mask;
        }
        phase2.sixteen_bits++;
        if(phase2.sixteen_bits > pitch2 >> 5)
        {
          phase2.sixteen_bits = 0;
          *_timer1_pin_port ^= _timer1_pin_mask;
        }
        break;
    }
    duration1.sixteen_bits--;
  }
  else
  {
    *_timer1_pin_port &= ~(_timer1_pin_mask); // set pin low
  }
}

