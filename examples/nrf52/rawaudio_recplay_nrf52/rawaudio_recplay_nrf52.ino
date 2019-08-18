#include <SPI.h>
#include <PDM.h>
#include "Adafruit_NeoPixel.h"
#include "audio.h"

#if !defined NRF52840_XXAA
#error "This example is for NRF52 boards only!"
#endif


#define LED_OUT            LED_RED
#define LEFT_BUTTON        4
#define NEOPIXEL_PIN       8
#define AUDIO_OUT         A0
#define SAMPLE_RATE       16000UL
#define SPEAKER_ENABLE    11

PDMClass PDM(33, 34, -1);
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define REC_BUFFER_SIZE 16000UL
volatile uint16_t audio_idx = 0;
uint16_t audio_max = 0;

int16_t audio_buffer[REC_BUFFER_SIZE];

short sampleBuffer[256];
volatile int samplesRead;

volatile bool isPlaying=false, isRecording=false;
bool button_state;

extern "C" { void SysTick_Handler(void) {  TimerCallback(); } } // extern C

void TimerCallback() {
  if (!isPlaying) { 
    return;
  }
  digitalToggle(LED_OUT);
  int16_t sample16 =  audio_buffer[audio_idx]; // g_recaudio_sample_data[audio_idx]; ( to test audio feedback)
  uint8_t sample8 = ((sample16 / 256) + 128);
  analogWrite(AUDIO_OUT, sample8);
  audio_idx++;
  if (audio_idx >= audio_max) {
    audio_idx = 0;
  }
}

void setup() {
  pinMode(LED_OUT, OUTPUT);   // Onboard LED can be used for precise
  digitalWrite(LED_OUT, LOW); // benchmarking with an oscilloscope
  pinMode(SPEAKER_ENABLE, OUTPUT);
  digitalWrite(SPEAKER_ENABLE, LOW);
  pinMode(LEFT_BUTTON, INPUT_PULLDOWN);
  pixel.begin();
  pixel.clear();
  pixel.show();
  delay(10);
  pixel.show();

  isPlaying = isRecording = false;
  
  Serial.begin(115200);
  while(!Serial) delay(1);                 // Wait for Serial monitor before continuing

  SysTick_Config(F_CPU/SAMPLE_RATE);
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
      audio_buffer[audio_idx] = sampleBuffer[i];
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

  audio_max = audio_idx;
  Serial.printf("Done! Recorded %d samples\n", audio_max);


  Serial.println("////////////////////////////////////////");

  Serial.printf("const int16_t g_recaudio_sample_data[%d] = {\n", audio_max);
  for (int i=0; i<audio_max; i+=16) {
    for (int x=0; x<16; x++) {
      Serial.printf("%d,\t", audio_buffer[i+x]);
    }
    Serial.println();
  }
  Serial.println("};");
  Serial.println("/////////////////////////////////");


  // make sure we're not pressed now
  while (digitalRead(LEFT_BUTTON)) {
    delay(10);
  }

  Serial.println("Waiting for button press to play...");
  button_state = false;
  audio_idx = 0;
}

void loop() {
  if (button_state == digitalRead(LEFT_BUTTON)) {
    return; // no change!
  }

  // something different...
  if (digitalRead(LEFT_BUTTON)) {
    // button is pressed
    pixel.setPixelColor(0, pixel.Color(0, 20, 0));
    pixel.show();
    audio_idx = 0;
    isPlaying = true;
    Serial.println("Playing");
    digitalWrite(SPEAKER_ENABLE, HIGH);
  } else {
    // released
    isPlaying = false;
    digitalWrite(SPEAKER_ENABLE, LOW);
    pixel.setPixelColor(0, pixel.Color(0, 0, 0));
    pixel.show();
    Serial.println("Stopped");
  }
  button_state = digitalRead(LEFT_BUTTON);
}

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
