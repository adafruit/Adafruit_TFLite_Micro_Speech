#include "GifDecoder.h"
#include <Adafruit_Arcada.h>

GifDecoder<ARCADA_TFT_WIDTH, ARCADA_TFT_HEIGHT, 12> decoder;
extern Adafruit_Arcada arcada;

static File file;
static int16_t  gif_offset_x, gif_offset_y;


bool playGIF(const char *filename, uint8_t times) {
  file = arcada.open(filename, O_READ);
  if (!file) {
    return false;
  }

  arcada.display->endWrite();   // End transaction from any prior callback
  arcada.display->fillScreen(ARCADA_BLACK);
  decoder.startDecoding();

  // Center the GIF
  uint16_t w, h;
  decoder.getSize(&w, &h);
  Serial.print("Width: "); Serial.print(w); Serial.print(" height: "); Serial.println(h);
  if (w < arcada.display->width()) {
    gif_offset_x = (arcada.display->width() - w) / 2;
  } else {
    gif_offset_x = 0;
  }
  if (h < arcada.display->height()) {
    gif_offset_y = (arcada.display->height() - h) / 2;
  } else {
    gif_offset_y = 0;
  }

  Serial.print("Drawing GIF");
  while (true) {
    Serial.print(".");
    decoder.decodeFrame();
    if (decoder.getCycleNo() > times) {
      Serial.println("done!");
      return true;
    }
    delay(1);
  }
  return true;
}



/******************************* Drawing functions */

void updateScreenCallback(void) {  }

void screenClearCallback(void) {  }

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    arcada.display->drawPixel(x, y, arcada.display->color565(red, green, blue));
}

void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) {
    uint8_t pixel;
    uint32_t t = millis();
    x += gif_offset_x;
    y += gif_offset_y;
    if (y >= arcada.display->height() || x >= arcada.display->width() ) return;
    
    if (x + w > arcada.display->width()) w = arcada.display->width() - x;
    if (w <= 0) return;

    uint16_t buf565[2][w];
    bool first = true; // First write op on this line?
    uint8_t bufidx = 0;
    uint16_t *ptr;

    for (int i = 0; i < w; ) {
        int n = 0, startColumn = i;
        ptr = &buf565[bufidx][0];
        // Handle opaque span of pixels (stop at end of line or first transparent pixel)
        while((i < w) && ((pixel = buf[i++]) != skip)) {
            ptr[n++] = palette[pixel];
        }
        if (n) {
            arcada.display->dmaWait(); // Wait for prior DMA transfer to complete
            if (first) {
              arcada.display->endWrite();   // End transaction from prior call
              arcada.display->dmaWait();
              arcada.display->startWrite(); // Start new display transaction
              first = false;
            }
            arcada.display->setAddrWindow(x + startColumn, y, n, 1);
            arcada.display->writePixels(ptr, n, false, false);
            bufidx = 1 - bufidx;
        }
    }
    arcada.display->dmaWait(); // Wait for last DMA transfer to complete
}


bool fileSeekCallback(unsigned long position) {
  return file.seek(position);
}

unsigned long filePositionCallback(void) { 
  return file.position(); 
}

int fileReadCallback(void) {
    return file.read(); 
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    return file.read((uint8_t*)buffer, numberOfBytes); //.kbv
}



void initGIFDecode() {
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);
  decoder.setDrawLineCallback(drawLineCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);
}
