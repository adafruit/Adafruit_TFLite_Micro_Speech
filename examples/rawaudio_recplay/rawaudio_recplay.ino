#include <SPI.h>
#include "Adafruit_ZeroTimer.h"
#include "Adafruit_NeoPixel.h"

#define LED_OUT      13
#define BUTTON        5
#define NEOPIXEL_PIN  8
#define AUDIO_OUT     A0
#define AUDIO_IN      A5

#define DAC_TIMER        5
void TC5_Handler(){
  Adafruit_ZeroTimer::timerHandler(DAC_TIMER);
}

Adafruit_ZeroTimer zt = Adafruit_ZeroTimer(DAC_TIMER);
#define SAMPLERATE_HZ 16000

#define BUFFER_SIZE 16000
volatile uint16_t audio_idx = 0;
uint16_t audio_max = 0;
int16_t audio_buffer[BUFFER_SIZE];

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
    audio_buffer[audio_idx] = sample;
    audio_idx++;
    if (audio_idx == BUFFER_SIZE) {
      isRecording = false;
    }
  }
  if (isPlaying) {
    digitalWrite(LED_OUT, val);
    val = !val;
    int16_t sample = audio_buffer[audio_idx];
    sample /= 16;    // convert 16 bit to 12 bit
    sample += 2047;  // turn into signed 0-4095
    analogWrite(AUDIO_OUT, sample);
    audio_idx++;
    if (audio_idx == audio_max) {
      audio_idx = 0;
    }
  }
}

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

  audio_max = audio_idx;
  Serial.printf("Done! Recorded %d samples\n", audio_max);

  Serial.println("/**********************************************/");
  Serial.printf("const int16_t g_recaudio_sample_data[%d] = {\n", audio_max);
  for (int i=0; i<audio_max; i+=16) {
    for (int x=0; x<16; x++) {
      Serial.printf("%d,\t", audio_buffer[i+x]);
    }
    Serial.println();
  }
  Serial.println("};");

  Serial.println("/**********************************************/");

  Serial.println("Waiting for button press to play...");
  button_state = true;
  audio_idx = 0;
  // make sure we're not pressed now
  while (!digitalRead(BUTTON)) {
    delay(10);
  }
}

void loop() {
  if (button_state == digitalRead(BUTTON)) {
    delay(10);
    return; // no change!
  }

  // something different...
  if (!digitalRead(BUTTON)) {
    // button is pressed
    pixel.setPixelColor(0, pixel.Color(0, 20, 0));
    pixel.show();
    audio_idx = 0;
    isPlaying = true;
    Serial.println("Playing");
  } else {
    // released
    isPlaying = false;
    pixel.setPixelColor(0, pixel.Color(0, 0, 0));
    pixel.show();
    Serial.println("Stopped");
  }
  button_state = digitalRead(BUTTON);
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
