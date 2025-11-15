### Lepton-3.5-image-and-DHT11-themperature-value-with-Arduino-esp-32 ###

This project is based on (https://github.com/Jacob-Lundstrom/ESP-FLIR/blob/main/ESP-FLIR.ino) [ESP-FLIR by Z1codia]
Modifications and improvements made by FrancoAnco

The code is designed to communicate with the ESP-32 using specific commands: You need to send command `B` to capture an image with Lepton 3.5, command `A` to reset the device, and command `C` to acquire temperature and humidity values from DHT11 sensor.

The image is captured by dividing it into 4 segments, which are then recombined. The ESP-32 sends the pixel values corresponding to the thermal image to the Arduino console. To create the image file, these values must be copied into a file with a `.pgm` extension, preceded by the following lines:

```
P2
160 120
65535
```

you can have pgm image, pixels values matrix and themperature matrix by using the python code (I tried only on Raspberry Pi 4 B)


Lepton Datasheet: https://cdn.sparkfun.com/assets/f/6/3/4/c/Lepton_Engineering_Datasheet_Rev200.pdf

## Boards Manager

esp32 by Espressif Systems

(You can add on the left, under sketchbook in arduino program)

## Arduino Library

DHT-sensor-library by adafruit

Lepton FLiR Thermal Camera Module Library by NachtRaveVL

(You can add on the left, under Boards Manager in arduino program)


