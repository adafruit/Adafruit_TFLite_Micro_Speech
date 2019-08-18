#include "audio.h"
#include "yes.h"
#include "no.h"

#if !defined NRF52840_XXAA
#error "This example is for NRF52 boards only!"
#endif

#define SPEAKER_ENABLE 11
#define SAMPLE_RATE 16000

const int16_t *currentplay = NULL;
volatile bool playing = false;
volatile uint32_t play_size = 0, play_idx = 0;
extern "C" {
void SysTick_Handler(void) {
  if ((play_size == 0) || (currentplay == NULL)) {
    playing = false;
  } else if (play_idx == 0) {
    playing = true;       // start playing
  }
  if (! playing) {
    return;
  }
 
  digitalToggle(LED_RED);
  int16_t sample16 = currentplay[play_idx];
  uint8_t sample8 = ((sample16 / 256) + 128);
  analogWrite(A0, sample8);
  play_idx++;
  if (play_idx >= play_size) {
    playing = false;
  }
}
} // extern C


void setup() {
  pinMode(SPEAKER_ENABLE, OUTPUT);
  digitalWrite(SPEAKER_ENABLE, LOW);
  
  Serial.begin(115200);
  
  analogWriteResolution(8);
  analogWrite(A0, 128);
}

void loop() {
  Serial.println("Yes");
  play_tune_i16(g_yes_1000ms_sample_data, g_yes_1000ms_sample_data_size);
  delay(100);
  Serial.println("No");
  play_tune_i16(g_no_1000ms_sample_data, g_no_1000ms_sample_data_size);
  delay(100);
  Serial.println("Recording");
  play_tune_i16(g_recaudio_sample_data, g_recaudio_sample_data_size);
  delay(100);
}


void play_tune_i16(const int16_t *audio, uint32_t audio_length) {
  play_size = 0;
  play_idx = 0;
  SysTick_Config(F_CPU/SAMPLE_RATE);
  currentplay = audio;
  play_size = audio_length;

  // wait to start playing
  delay(10);
  while (playing) {
    delay(1);
  }
}
