/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/experimental/micro/examples/micro_speech/audio_provider.h"
#include "tensorflow/lite/experimental/micro/examples/micro_speech/micro_features/micro_model_settings.h"
#include <Arduino.h>

//#define DEBUG_SAMPLES

namespace {
int16_t g_dummy_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

extern int16_t *recording_buffer;
extern volatile uint32_t recording_length;
extern volatile uint32_t audio_timestamp_ms;

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter,
                             int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) { 
  const int start_sample = (start_ms * kAudioSampleFrequency) / 1000;
#ifdef DEBUG_SAMPLES
  digitalWrite(13, HIGH);
  Serial.printf("Getting %d samples: ", kMaxAudioSampleSize);
#endif

  uint32_t curr_audio_start_sample = (audio_timestamp_ms * kAudioSampleFrequency) / 1000;
  uint32_t curr_audio_end_sample = curr_audio_start_sample+recording_length;

  //Serial.printf("Samples start @ %d, last audio @ %d\n", start_sample, curr_audio_start_sample);
 
  for (int i = 0; i < kMaxAudioSampleSize; ++i) {
    const int32_t sample_index = start_sample + i;
    int16_t sample;

    if ((sample_index < 0) || (sample_index < curr_audio_start_sample)) {
      sample = 0;
    } else if (sample_index >= curr_audio_end_sample) {
      sample = 0;      
    } else {
      sample = recording_buffer[sample_index-curr_audio_start_sample];
      //Serial.printf("%d, ", sample);
    }
    
    g_dummy_audio_data[i] = sample;
#ifdef DEBUG_SAMPLES
    Serial.printf("%d, ", sample);
#endif
  }
  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_dummy_audio_data;

#ifdef DEBUG_SAMPLES
  Serial.println();
  digitalWrite(13, LOW);
#endif
  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
  //g_latest_audio_timestamp += 100;
  return millis();
}
