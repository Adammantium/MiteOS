import sys
import glob
import serial
import io
import datetime
import time
from PIL import Image, ImageTk
import tkinter as tk
import numpy as np
import time

window = tk.Tk()

window.title("Watchy")
window.geometry("500x500")
window.configure(background='grey')

panel = tk.Label(window)
panel.pack(side = "bottom", fill = "both", expand = "yes")

def updateImg():
    global panel
    try:
        f = Image.open("buf.png")
        f = f.resize((500, 500))
        img = ImageTk.PhotoImage(f)
        panel.configure(image=img)
        panel.image = img
    except:
        print("No Image found yet")

def byte_array_to_booleans(byte_array):
    boolean_list = []
    
    for byte in byte_array:
        # Konvertiert das Byte in eine Binärdarstellung und füllt es mit führenden Nullen auf (8 Bits)
        bin_string = f"{byte:08b}"
        
        # Konvertiert jedes Bit in einen Boolean
        for bit in bin_string:
            boolean_list.append(bit == '1')
    
    return boolean_list

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/ttyACM[A-Za-z0-9]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

updateImg()

if(len(sys.argv) < 2 or sys.argv[1] != "false"):
    window.update()

#ser = serial.Serial('COM12', baudrate = 115200)
#ser = serial.Serial('/dev/ttyACM0', baudrate = 115200)
while(len(serial_ports()) == 0):
    time.sleep(1)

print(serial_ports()[0])
ser = serial.Serial(serial_ports()[0], baudrate = 921600)
sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))


#ser.setDTR(False)
#time.sleep(1)
#ser.flushInput()
#ser.setDTR(True)

print("Waiting...")
seq = []
count = 1 ## row index
last_frame = time.time()
while True:
    if (ser.inWaiting() > 0):
        try:
            line = ser.readline().strip()
            try:
                line = line.decode('ascii')
            except:
                line = line.decode("utf-8")
            
            if line.startswith('screenshot|'):
                # print("Screenshot")
                imageData = line.split("|")[1]
                # print(len(imageData))
                imageData = byte_array_to_booleans(bytes.fromhex(imageData))
                
                # print(len(imageData))
                data = np.zeros((200,200,3), dtype=np.uint8)
                for x in range(0, 200):
                    for y in range(0, 200):
                        if imageData[x * 200 + y]:
                            #data[y, x] = (255, 236, 221)
                            data[y, x] = (255, 255, 255)
                
                im = Image.fromarray(data)
                im.save('buf.png')
                updateImg()
                end = time.time()
                print("[Screenshot] @ " + str(round(1 / (end - last_frame) * 60)) + " FPM")
                last_frame = time.time()
            else:
                print(line);
        except:
            print("err")
    
    if(len(sys.argv) < 2 or sys.argv[1] != "false"):
        window.update()