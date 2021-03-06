#include "audio.h"
#include "yes.h"
#include "no.h"

// Simple DAC-output audio player for recorded samples, good for testing!
#define AUDIO_OUT A0

void setup() {
  Serial.begin(115200);
  
  analogWriteResolution(12);
  analogWrite(AUDIO_OUT, 128);

  play_tune_i16(g_yes_1000ms_sample_data, g_yes_1000ms_sample_data_size);
  delay(100);
  play_tune_i16(g_no_1000ms_sample_data, g_no_1000ms_sample_data_size);
  delay(100);
}

void loop() {
}

void play_tune(const uint8_t *audio, uint32_t audio_length) {
  uint32_t t;
  uint32_t prior, usec = 1000000L / SAMPLE_RATE;
  
  for (uint32_t i=0; i<audio_length; i++) {
    while((t = micros()) - prior < usec);
    analogWrite(AUDIO_OUT, (uint16_t)audio[i] / 8);
    prior = t;
  }
}



void play_tune_i16(const int16_t *audio, uint32_t audio_length) {
  uint32_t t;
  uint32_t prior, usec = 1000000L / SAMPLE_RATE;
  
  for (uint32_t i=0; i<audio_length; i++) {
    while((t = micros()) - prior < usec);
    analogWrite(AUDIO_OUT, (audio[i]+32767) / 16);
    prior = t;
  }
}