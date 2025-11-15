# Code by FrancoAnco (https://github.com/FrancoAnco)

import serial
import datetime
import os
import time

def pixel_to_temperature(pixel_value):
    temperature = (pixel_value - 30000)*0.01+26.85
    return temperature

def convert_matrix(pixel_matrix):
    temperature_matrix = [[pixel_to_temperature(pixel) for pixel in row] for row in pixel_matrix]
    return temperature_matrix

def write_to_file(matrix, name):
    with open(name, 'w') as file:
        for row in matrix:
            #line = ' '.join(map(str,row))
            line = '\t'.join("{:.2f}".format(temperature) for temperature in row)
            file.write(line+'\n')
            
directory1 = "/home/pi/Desktop/sensori"   ############### Set your directory ################         
            
os.makedirs(directory1, exist_ok=True)
ser = serial.Serial('/dev/ttyUSB0', 115200) ############### check at what port number is your usb device (esp32) ################

try:
    ora_corrente = datetime.datetime.now()
    timestamp = ora_corrente.strftime("%Y_%m_%d-%H_%M_%S")
    filename1 = os.path.join(directory1, f"Sensor_2_{timestamp}.txt")
    
    ser.write(b'C')
    time.sleep(0.5)
    sensor_response = ser.readline().decode('utf-8').strip()
      #third_filename = os.path.join(directory1, f"Sensor_2_temperature_{timestamp}.txt")
    with open(filename1,'w') as file:
        file.write(f"{sensor_response}\n")
            #readedText = ser.readline()
        print(f"Sensor 2 data saved: {sensor_response}")
        
except Exception as sensor_error:
    print(f"Errore letture sensore 2: {sensor_error}")


#print("ok\n")







