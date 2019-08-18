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
volatile uint32_t recording_length = 0;
int16_t *recording_buffer;
volatile uint32_t audio_timestamp_ms;

/*          PLAYBACK CODE      */
extern "C" { void SysTick_Handler(void) {  TimerCallback(); } } // extern C
volatile bool isRecording=false;
volatile uint8_t button_counter = 0;

void TimerCallback() {
  if (isRecording) {
    digitalToggle(LED_OUT);
    PDM.IrqHandler();
    if (samplesRead) {
      for (int i = 0; i < samplesRead; i++) {
        recording_buffer[audio_idx] = sampleBuffer[i];
        audio_idx++;
        if (audio_idx >= REC_BUFFER_SIZE) {
          isRecording = false;
          return;
        }
      }
      samplesRead = 0;
    }
  }

  button_counter++;
  if (button_counter == 160) {
    button_counter = 0;
    // this is called once every 10 millisecond!
    if (!isRecording && digitalRead(LEFT_BUTTON)) {
      // button was pressed!
      isRecording = true;
      // reset audio to beginning of buffer
      recording_length = audio_idx = 0;
      // erase the buffer
      memset(recording_buffer, 0, REC_BUFFER_SIZE * sizeof(int16_t));
      //Serial.printf("Recording @ %d...", millis());
    }
    if (isRecording && !digitalRead(LEFT_BUTTON)) {
      isRecording = false;  // definitely done!
    
      recording_length = audio_idx;
      //Serial.printf("Done! Recorded %d samples\n", recording_length);  
      // we have to track when this happened
      audio_timestamp_ms = millis();
    }
  }
}

extern int tflite_micro_main(int argc, char* argv[]);
extern unsigned char model_data[];
extern const char *model_labels[];
unsigned char *g_tiny_conv_micro_features_model_data;
extern unsigned char kCategoryCount;
extern const char *model_labels[];
char **kCategoryLabels;

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
  isRecording = false;

  Serial.begin(115200);
  //while(!Serial);                 // Wait for Serial monitor before continuing

  recording_buffer = (int16_t *)malloc(REC_BUFFER_SIZE * sizeof(int16_t));
  if (!recording_buffer) {
    Serial.println("Unable to allocate recording buffer");
    while (1);
  }
  memset(recording_buffer, 0, REC_BUFFER_SIZE * sizeof(int16_t));
  recording_length = 0;

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
    pixel.clear();
    if (found_command[0] == 'y') {      // yes!
        pixel.fill(pixel.Color(0, 255, 0));
        pixel.show();
        digitalWrite(SPEAKER_ENABLE, HIGH);
        tone(AUDIO_OUT, 440, 100);
        delay(100);
        tone(AUDIO_OUT, 880, 100);
        delay(100);
        tone(AUDIO_OUT, 1760, 100);
        delay(100);
        digitalWrite(SPEAKER_ENABLE, LOW);
        delay(250);
        pixel.clear();
    }
    if (found_command[0] == 'n') {      // no!
        pixel.fill(pixel.Color(255, 0, 0));
        pixel.show();
        digitalWrite(SPEAKER_ENABLE, HIGH);
        tone(AUDIO_OUT, 1760, 100);
        delay(100);
        tone(AUDIO_OUT, 880, 100);
        delay(100);
        tone(AUDIO_OUT, 440, 100);
        delay(100);
        digitalWrite(SPEAKER_ENABLE, LOW);
        delay(250);
        pixel.clear();
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
