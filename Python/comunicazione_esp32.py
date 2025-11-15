# Original code by FrancoAnco (https://github.com/FrancoAnco)

import serial
import datetime
import os
import time

def pixel_to_temperature(pixel_value):
    temperature = (pixel_value - 30000)*0.01 + 26.85
    return temperature

def convert_matrix(pixel_matrix):
    temperature_matrix = [[pixel_to_temperature(pixel) for pixel in row] for row in pixel_matrix]
    return temperature_matrix

def write_to_file(matrix, name):
    with open(name, 'w') as file:
        for row in matrix:
            line = '\t'.join("{:.2f}".format(temperature) for temperature in row)
            file.write(line + '\n')

directory = "/home/pi/Desktop/lepton2" ############### Set your directory ################
ser = serial.Serial('/dev/ttyUSB0', 115200) ############### check at what port number is your usb device (esp32) ################

try:
    ora_corrente = datetime.datetime.now()
    timestamp = ora_corrente.strftime("%Y_%m_%d-%H_%M_%S")

    filename = os.path.join(directory, f"Lepton_2_matrix_{timestamp}.txt")

    ser.write(b'B')
    count = 1

    with open(filename, 'w') as file:
        while count < 123:
            readedText = ser.readline()
            count += 1
            string_object = readedText.decode('utf-8')
            file.write(string_object)
            file.flush()
            print(string_object)

    second_filename = os.path.join(directory, f"Lepton_2_foto_{timestamp}.pgm")
    with open(second_filename, 'w') as second_file:
        second_file.write("P2\n")
        second_file.write("160 120\n")
        second_file.write("65535\n")

        maxval = 0
        minval = 65535

        with open(filename, 'r') as source_file:
            for line in source_file:
                values = list(map(int, line.split()))
                for value in values:
                    if value > maxval:
                        maxval = value
                    if 0 < value < minval:
                        minval = value

        with open(filename, 'r') as source_file:
            for line in source_file:
                values = list(map(int, line.split()))
                for value in values:
                    if value > 0:
                        normalized_value = int((value - minval) * 65535 / (maxval - minval))
                        second_file.write(f"{normalized_value}\t")
                    else:
                        second_file.write("32767\t")
                second_file.write("\n")

    third_filename = os.path.join(directory, f"Lepton_2_temperature_{timestamp}.txt")
    with open(filename, 'r') as file:
        pixel_matrix = [[int(value) for value in line.split()] for line in file]
        temperature_matrix = convert_matrix(pixel_matrix)
        write_to_file(temperature_matrix, third_filename)

    print("\nLepton 2 ok")
    time.sleep(1)

except Exception as e:
    print(f"errore: {e}")

# ser.close()  # Scommenta se vuoi chiudere la seriale
