#ifndef SFXAudio_h
#define SFXAudio_h

#include <Arduino.h>
#include <EEPROM.h>
#include "Arduboy_x.h"
#include <avr/pgmspace.h>
#include <avr/power.h>

typedef struct
{
  unsigned int frequency;
  int frequency_slide;
  int frequency_accel;
  unsigned int pulse_width;
  unsigned int pulse_freq;
  unsigned int duration;
  unsigned char method;
} SFX_Data;

class SFXAudio
{
public:
	void begin();
	void on();
	void off();
	void save_on_off();
	bool enabled();
  void sfx(const SFX_Data * data);

protected:
	bool audio_enabled;
};

#endif
