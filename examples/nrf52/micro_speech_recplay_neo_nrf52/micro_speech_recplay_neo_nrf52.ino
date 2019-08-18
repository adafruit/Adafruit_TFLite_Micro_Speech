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
#include <PDM.h>
#include <Adafruit_NeoPixel.h>

#define LED_OUT       LED_RED
#define LEFT_BUTTON        4
#define NEOPIXEL_PIN       8
#define AUDIO_OUT         A0
#define SPEAKER_ENABLE    11
#define SAMPLERATE_HZ     kAudioSampleFrequency

PDMClass PDM(33, 34, -1);
Adafruit_NeoPixel pixel(10, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

/*          RECORDING CODE      */
short sampleBuffer[256];  // pdm buffer
volatile int samplesRead;
#define REC_BUFFER_SIZE (SAMPLERATE_HZ*3)
volatile uint32_t audio_idx = 0;
uint32_t recording_length = 0;
int16_t *recording_buffer;

/*          TFLITE CODE      */
extern unsigned char model_data[];
extern const char *model_labels[];
unsigned char *g_tiny_conv_micro_features_model_data;
extern unsigned char kCategoryCount;
extern const char *model_labels[];
char **kCategoryLabels;

/*          PLAYBACK CODE      */
extern "C" { void SysTick_Handler(void) {  TimerCallback(); } } // extern C

volatile bool isPlaying=false, isRecording=false;

void TimerCallback() {
  if (!isPlaying) { 
    return;
  }
  digitalToggle(LED_OUT);
  int16_t sample16 =  recording_buffer[audio_idx]; // g_recaudio_sample_data[audio_idx]; ( to test audio feedback)
  uint8_t sample8 = ((sample16 / 256) + 128);
  analogWrite(AUDIO_OUT, sample8);
  audio_idx++;
  if (audio_idx >= recording_length) {
    isPlaying = false;
  }
}

extern int tflite_micro_main(int argc, char* argv[]);

void setup() {
  pinMode(LED_OUT, OUTPUT);   // Onboard LED can be used for precise
  digitalWrite(LED_OUT, LOW); // benchmarking with an oscilloscope
  pinMode(SPEAKER_ENABLE, OUTPUT);
  digitalWrite(SPEAKER_ENABLE, LOW);
  pinMode(LEFT_BUTTON, INPUT_PULLDOWN);
  pixel.begin();
  pixel.setBrightness(30);
  pixel.clear();
  pixel.show();
  delay(10);
  pixel.show();
  isPlaying = isRecording = false;
  
  Serial.begin(115200);
  //while(!Serial);                 // Wait for Serial monitor before continuing

  recording_buffer = (int16_t *)malloc(REC_BUFFER_SIZE * sizeof(int16_t));
  if (!recording_buffer) {
    Serial.println("Unable to allocate recording buffer");
    while (1);
  }
  memset(recording_buffer, 0, REC_BUFFER_SIZE * sizeof(int16_t));

  // Set up the model dynamically!
  g_tiny_conv_micro_features_model_data = (unsigned char *)model_data;
  Serial.printf("Model Data: ", g_tiny_conv_micro_features_model_data, model_data);
  for (uint8_t x=0; x<16; x++) {
    Serial.printf("0x%02X, ", g_tiny_conv_micro_features_model_data[x]);
  }
  Serial.println("...");

  kCategoryLabels = (char **)malloc(kCategoryCount * sizeof(char **)); 
  Serial.printf("Found %d labels:\n", kCategoryCount);
  for (int i=0; i<kCategoryCount; i++) {
    kCategoryLabels[i] = (char *)malloc(strlen(model_labels[i])+1);
    strcpy(kCategoryLabels[i], model_labels[i]);
    Serial.print("\t"); Serial.println(kCategoryLabels[i]);
  }

  SysTick_Config(F_CPU/SAMPLERATE_HZ);
  pinMode(AUDIO_OUT, OUTPUT);
  analogWriteResolution(8);
  analogWrite(AUDIO_OUT, 128);
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while (1) delay(10);
  }
    
  Serial.println("Record & Play audio test");
  Serial.println("Waiting for button press to record...");
  // wait till its pressed
  while (!digitalRead(LEFT_BUTTON)) { delay(10); }
  delay(10); // debounce

  // make pixel red
  pixel.setPixelColor(0, pixel.Color(20, 0, 0));
  pixel.show();
  // reset audio to beginning of buffer
  audio_idx = 0;

  // and begin!
  isRecording = true;
  Serial.print("Recording...");
  
  // while its pressed...
  while (isRecording && digitalRead(LEFT_BUTTON)) {
    PDM.IrqHandler();
    if (!samplesRead) {    // wait for samples to be read
      continue;
    }
    
    for (int i = 0; i < samplesRead; i++) {
      recording_buffer[audio_idx] = sampleBuffer[i];
      audio_idx++;
      if (audio_idx >= REC_BUFFER_SIZE) {
        Serial.println("DONE");
        isRecording = false;
        break;
      }
    }
    samplesRead = 0;
    yield();
  }
  
  isRecording = false;  // definitely done!
  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();

  recording_length = audio_idx;
  Serial.printf("Done! Recorded %d samples\n", recording_length);

/*
  Serial.println("Waiting for button press to play...");
  audio_idx = 0;
  // make sure we're not pressed now
  while (digitalRead(LEFT_BUTTON)) {
    delay(10);
  }
  // while released
  while (!digitalRead(LEFT_BUTTON)) {
    delay(10);
  }
  pixel.setPixelColor(0, pixel.Color(0, 20, 0));
  pixel.show();
  audio_idx = 0;
  isPlaying = true;
  Serial.println("Playing");
  digitalWrite(SPEAKER_ENABLE, HIGH);
  while (isPlaying) {
    yield();
  }
  digitalWrite(SPEAKER_ENABLE, LOW);
  
  //while (1) { Serial.println("."); delay(100); }
  */
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
    pixel.clear();
    if (found_command[0] == 'y') {      // yes!
        pixel.fill(pixel.Color(0, 255, 0));
    }
    if (found_command[0] == 'n') {      // no!
        pixel.fill(pixel.Color(255, 0, 0));
    }
    pixel.show();                           
  }
}



void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
