// This demo of micro speech uses the timer to check if the button is pressed.
// When pressed, the button will record audio (neopixel turns red). Once released
// the tensorflow lite runtime will pick up the audio samples and perform the analysis


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

// Include an empty header so that Arduino knows to build the TF Lite library.
#include <TensorFlowLite.h>
#include "tensorflow/lite/experimental/micro/examples/micro_speech/command_responder.h"
#include "tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/micro_model_settings.h"

#include "Adafruit_ZeroTimer.h"
#include "Adafruit_NeoPixel.h"
#define LED_OUT       13
#define BUTTON        5
#define NEOPIXEL_PIN  8
#define AUDIO_IN      A5

#define DAC_TIMER        5
void TC5_Handler(){
  Adafruit_ZeroTimer::timerHandler(DAC_TIMER);
}

Adafruit_ZeroTimer zt = Adafruit_ZeroTimer(DAC_TIMER);
#define SAMPLERATE_HZ kAudioSampleFrequency

#define BUFFER_SIZE (SAMPLERATE_HZ*3) // up to 3 seconds of audio
volatile uint32_t audio_idx = 0;
volatile uint32_t recording_length = 0;
int16_t *recording_buffer;
volatile uint32_t audio_timestamp_ms;

Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

volatile bool val;
volatile bool isRecording;
volatile uint8_t button_counter = 0;
void TimerCallback() {
  if (isRecording) {
    digitalWrite(LED_OUT, val);  // tick tock test
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

  button_counter++;
  if (button_counter == 160) {
    button_counter = 0;
    // this is called once every 10 millisecond!
    if (!isRecording && !digitalRead(BUTTON)) {
      // button was pressed!
      isRecording = true;
      // NeoPixel red to indicate recording
      pixel.setPixelColor(0, pixel.Color(20, 0, 0));
      pixel.show();
      // reset audio to beginning of buffer
      recording_length = audio_idx = 0;
      // erase the buffer
      memset(recording_buffer, 0, BUFFER_SIZE * sizeof(int16_t));
      Serial.printf("Recording @ %d...", millis());
    }
    if (isRecording && digitalRead(BUTTON)) {
      isRecording = false;  // definitely done!
      pixel.setPixelColor(0, pixel.Color(0, 0, 0));
      pixel.show();
    
      recording_length = audio_idx;
      Serial.printf("Done! Recorded %d samples\n", recording_length);  
      // we have to track when this happened
      audio_timestamp_ms = millis();
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
  isRecording = false;
  pinMode(BUTTON, INPUT_PULLUP);

  Serial.begin(115200);
  while(!Serial);                 // Wait for Serial monitor before continuing

  recording_buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
  if (!recording_buffer) {
    Serial.println("Unable to allocate recording buffer");
    while (1);
  }
  memset(recording_buffer, 0, BUFFER_SIZE * sizeof(int16_t));
  recording_length = 0;

  initTimer();
  analogWriteResolution(12);
  analogReadResolution(12);
  analogWrite(A0, 512);

  Serial.println("Waiting for button press to record...");
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
