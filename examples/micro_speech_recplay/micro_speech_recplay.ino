/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// Template sketch that calls into the detailed TensorFlow Lite example code.

// Include an empty header so that Arduino knows to build the TF Lite library.
#include <TensorFlowLite.h>
#include "tensorflow/lite/experimental/micro/examples/micro_speech/command_responder.h"
#include "tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/micro_model_settings.h"

#include "Adafruit_ZeroTimer.h"
#include "Adafruit_NeoPixel.h"
#define LED_OUT       13
#define BUTTON        5
#define NEOPIXEL_PIN  8
#define AUDIO_OUT     A0
#define AUDIO_IN      A5

#define DAC_TIMER        5
void TC5_Handler(){
  Adafruit_ZeroTimer::timerHandler(DAC_TIMER);
}

Adafruit_ZeroTimer zt = Adafruit_ZeroTimer(DAC_TIMER);
#define SAMPLERATE_HZ kAudioSampleFrequency

#define BUFFER_SIZE (SAMPLERATE_HZ*3)
volatile uint32_t audio_idx = 0;
uint32_t recording_length = 0;
int16_t *recording_buffer;


Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

volatile bool val;
volatile bool isPlaying, isRecording;
bool button_state;

void TimerCallback()
{
  if (isRecording) {
    digitalWrite(LED_OUT, val);
    val = !val;
    int16_t sample = analogRead(AUDIO_IN);
    sample -= 2047; // 12 bit audio unsigned  0-4095 to signed -2048-~2047
    sample *= 16;   // convert 12 bit to 16 bit
    recording_buffer[audio_idx] = sample;
    audio_idx++;
    if (audio_idx == BUFFER_SIZE) {
      isRecording = false;
    }
  }
  if (isPlaying) {
    digitalWrite(LED_OUT, val);
    val = !val;
    int16_t sample = recording_buffer[audio_idx];
    sample /= 16;    // convert 16 bit to 12 bit
    sample += 2047;  // turn into signed 0-4095
    analogWrite(AUDIO_OUT, sample);
    audio_idx++;
    if (audio_idx == recording_length) {
      isPlaying = false;
    }
  }
}

extern int tflite_micro_main(int argc, char* argv[]);

void setup() {
  pinMode(LED_OUT, OUTPUT);   // Onboard LED can be used for precise
  digitalWrite(LED_OUT, LOW); // benchmarking with an oscilloscope
  pixel.begin();
  pixel.clear();
  pixel.show();
  delay(10);
  pixel.show();
  isPlaying = isRecording = false;
  
  Serial.begin(115200);
  while(!Serial);                 // Wait for Serial monitor before continuing

  recording_buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
  if (!recording_buffer) {
    Serial.println("Unable to allocate recording buffer");
    while (1);
  }
  memset(recording_buffer, 0, BUFFER_SIZE * sizeof(int16_t));


  initTimer();
  analogWriteResolution(12);
  analogReadResolution(12);
  analogWrite(A0, 512);

  Serial.println("Record & Play audio test");
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.println("Waiting for button press to record...");
  // wait till its pressed
  while (digitalRead(BUTTON)) { delay(10); }

  // make pixel red
  pixel.setPixelColor(0, pixel.Color(20, 0, 0));
  pixel.show();
  // reset audio to beginning of buffer
  audio_idx = 0;

  // and begin!
  isRecording = true;
  Serial.print("Recording...");
  
  // while its pressed...
  while (!digitalRead(BUTTON) && isRecording) {
    delay(10);
  }
  isRecording = false;  // definitely done!
  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();

  recording_length = audio_idx;
  Serial.printf("Done! Recorded %d samples\n", recording_length);

  Serial.println("Waiting for button press to play...");
  button_state = true;
  audio_idx = 0;
  // make sure we're not pressed now
  while (!digitalRead(BUTTON)) {
    delay(10);
  }
  // while released
  while (digitalRead(BUTTON)) {
    delay(10);
  }
  pixel.setPixelColor(0, pixel.Color(0, 20, 0));
  pixel.show();
  audio_idx = 0;
  isPlaying = true;
  Serial.println("Playing");
  while (isPlaying) {
    delay(10); // wait till done!!
  }

  Serial.println("-----------TFLITE----------");

  
  tflite_micro_main(0, NULL);
}

void loop() {

}


void RespondToCommand(tflite::ErrorReporter* error_reporter,
                      int32_t current_time, const char* found_command,
                      uint8_t score, bool is_new_command) {
  if (is_new_command) {
    error_reporter->Report("I heard %s (score %d) @%dms (realtime %d)", found_command, score,
                           current_time, millis());
  }
}



void initTimer(void) {
  zt.configure(TC_CLOCK_PRESCALER_DIV1, // prescaler
                TC_COUNTER_SIZE_16BIT,   // bit width of timer/counter
                TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode
                );

  zt.setCompare(0, (24000000/SAMPLERATE_HZ)*2);
  zt.setCallback(true, TC_CALLBACK_CC_CHANNEL0, TimerCallback);  // this one sets pin low
  zt.enable(true);
}
