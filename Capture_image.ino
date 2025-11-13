/*
MIT License

Copyright (c) 2023 Z1codia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/*Original code is modified by FrancoAnco (https://github.com/FrancoAnco)*/


#include <SPI.h>
#include <Wire.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MISO_PIN      19
#define MOSI_PIN      23
#define SCK_PIN       18
#define FLIR_NCS_PIN  5
#define spiClk        20000000

#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define ADDRESS       0x2A

#define VOSPI_FRAME_SIZE 164
uint16_t lepton_frame_packet[VOSPI_FRAME_SIZE / 2];
uint16_t lepton_frame_segment[60][VOSPI_FRAME_SIZE / 2];

#define image_x 160
#define image_y 120
int image[image_x][image_y];

SPIClass* hspi = NULL;

void setup() {
  Serial.begin(115200);
  dht.begin();

  hspi = new SPIClass(HSPI);
  hspi->begin(SCK_PIN, MISO_PIN, MOSI_PIN, FLIR_NCS_PIN);
  hspi->setDataMode(SPI_MODE3);
  hspi->setClockDivider(0);
  hspi->begin();

  pinMode(FLIR_NCS_PIN, OUTPUT);
  delay(7000);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  while (true) {
    int status = readRegister(0x0002);
    if (!(status & 0b100) && (status & 0b1)) {
      Serial.println("I2C is busy.");
      delay(1000);
    } else {
      break;
    }
  }

  //Serial.println("Sistema pronto. Premi:\nA - per riavviare\nB - per acquisire immagine\nC - per leggere temperatura/umidità");
}

void loop() {
  if (Serial.available() > 0) {
    char inputChar = Serial.read();

    if (inputChar == 'A') {
      //Serial.println("Riavvio...");
      delay(100);
      ESP.restart();
    }
    else if (inputChar == 'B') {
      //Serial.println("\n\nAvvio acquisizione immagine\n");
      delay(100);
      bool validImage = false;
      int maxTries = 50;
      for (int t = 1; t <= maxTries && !validImage; t++) {
        //Serial.print("Tentativo #"); Serial.println(t);
        validImage = captureImage();
        if (!validImage) {
          //Serial.println("Immagine non valida, riprovo...");
          delay(100);
        }
      }
    }
    else if (inputChar == 'C') {
      const int maxRetries = 50;
      int retryCount = 0;
      float h, t;
      bool validRead = false;

      while (retryCount < maxRetries && !validRead) {
        h = dht.readHumidity();
        t = dht.readTemperature();

        if (!isnan(h) && !isnan(t)) {
          validRead = true;
        } 
        
        else {
          retryCount++;
          delay(100);  // breve pausa prima di ritentare
        }
      }

      if (validRead) {
        Serial.print("Temperatura: ");
        Serial.print(t);
        Serial.print(" °C\t");
        Serial.print("Umidità: ");
        Serial.print(h);
        Serial.println(" %");
      } 
      
      else {
        Serial.println("Errore nella lettura del sensore DHT dopo 50 tentativi.");
      }
    }

  }
  
}

void leptonSync() {
  int data = 0x0f00;
  digitalWrite(FLIR_NCS_PIN, HIGH);
  delay(185);
  digitalWrite(FLIR_NCS_PIN, LOW);
  while ((data & 0x0f00) == 0x0f00) {
    digitalWrite(FLIR_NCS_PIN, HIGH);
    delay(185);
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE3));
    data = hspi->transfer(0x00) << 8;
    data |= hspi->transfer(0x00);
    for (int i = 0; i < (VOSPI_FRAME_SIZE - 2) / 2; i++) {
      digitalWrite(FLIR_NCS_PIN, LOW);
      hspi->transfer(0x00);
      hspi->transfer(0x00);
      digitalWrite(FLIR_NCS_PIN, HIGH);
    }
    digitalWrite(FLIR_NCS_PIN, HIGH);
    hspi->endTransaction();
  }
}

bool imageHasZeros() {
  for (int y = 0; y < image_y; y++) {
    for (int x = 0; x < image_x; x++) {
      if (image[x][y] == 0) return true;
    }
  }
  return false;
}

bool captureImage() {
  //Serial.println("Inizio acquisizione immagine...");
  leptonSync();
  delay(100);

  bool collectedSegments[4] = {false, false, false, false};
  int segmentSequence[4] = {0, 0, 0, 0};
  int segmentCount = 0;
  unsigned long startTime = millis();
  const unsigned long timeoutMs = 8000;
  int totalAttempts = 0;

  while ((!collectedSegments[0] || !collectedSegments[1] || !collectedSegments[2] || !collectedSegments[3]) && millis() - startTime < timeoutMs) {
    for (int packetNumber = 0; packetNumber < 60; packetNumber++) {
      int attempts = 0;
      const int maxAttempts = 100;
      do {
        attempts++;
        totalAttempts++;
        if (attempts > maxAttempts) {
          //Serial.println("Timeout pacchetto (troppi discard)");
          return false;
        }
        digitalWrite(FLIR_NCS_PIN, LOW);
        hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE3));
        byte dummyBuffer[164];
        hspi->transfer(dummyBuffer, sizeof(dummyBuffer));
        hspi->endTransaction();
        digitalWrite(FLIR_NCS_PIN, HIGH);
        for (int i = 0; i < 164; i += 2) {
          lepton_frame_packet[i / 2] = dummyBuffer[i] << 8 | dummyBuffer[i + 1];
        }
      } while (((lepton_frame_packet[0] & 0x0f00) >> 8 == 0x0f));

      for (int i = 0; i < VOSPI_FRAME_SIZE / 2; i++) {
        lepton_frame_segment[packetNumber][i] = lepton_frame_packet[i];
      }
    }

    int segmentNumber = (lepton_frame_segment[20][0] >> 12) & 0b0111;
    if (segmentNumber != 1 && segmentCount == 0) {
      //Serial.println("Primo segmento non è 1. Riprovo...");
      return false;
    }
    if (segmentNumber >= 1 && segmentNumber <= 5) {
      if (!collectedSegments[segmentNumber - 1]) {
        segmentSequence[segmentCount++] = segmentNumber;
        collectedSegments[segmentNumber - 1] = true;
        //Serial.print(" Segmento ricevuto: ");
        //Serial.println(segmentNumber);
        for (int packetNumber = 0; packetNumber < 60; packetNumber++) {
          for (int px = 0; px < 80; px++) {
            int value = lepton_frame_segment[packetNumber][px + 2];
            if (packetNumber % 2)
              image[80 + px][(segmentNumber - 1) * 30 + packetNumber / 2] = value;
            else
              image[px][(segmentNumber - 1) * 30 + packetNumber / 2] = value;
          }
        }
      }
    }
  }

  if (millis() - startTime >= timeoutMs) {
    //Serial.println("Timeout: non tutti i segmenti ricevuti");
    return false;
  }

  //Serial.print("Tentativi totali pacchetti: ");
  //Serial.println(totalAttempts);

  for (int i = 0; i < 4; i++) {
    if (segmentSequence[i] != i + 1) {
      //Serial.println("Segmenti fuori ordine o mancanti");
      return false;
    }
  }

  if (imageHasZeros()) {
    //Serial.println("L'immagine contiene zeri.");
    return false;
  }

  //Serial.println("Immagine valida. Stampo valori:");
  for (int i = 0; i < 120; i++) {
    for (int j = 0; j < 160; j++) {
      Serial.print(image[j][i]);
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println("\n\n");
  return true;
}

int readRegister(unsigned int reg) {
  setRegister(reg);
  Wire.requestFrom(ADDRESS, 2);
  int reading = Wire.read() << 8;
  reading |= Wire.read();
  return reading;
}

void setRegister(unsigned int reg) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(reg >> 8 & 0xff);
  Wire.write(reg & 0xff);
  Wire.endTransmission();
}
