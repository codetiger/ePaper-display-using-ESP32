import socket, struct, time, random
from PIL import Image, ImageOps
from io import BytesIO
from selenium import webdriver

TCP_IP = '192.168.0.214'
TCP_PORT = 8319

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

def clearScreen(color, apply=True):
    header = struct.pack('<BBHHHH', 1, color, 0, 0, 1872, 1404)
    s.sendall(header)
    if apply:
        header = struct.pack('<BBHHHH', 4, 0, 0, 0, 1872, 1404)
        s.sendall(header)

def drawImage(posx, posy, origImage):
    grayImage = ImageOps.grayscale(origImage)
    width, height = grayImage.size
    pixels = grayImage.load()
    image = bytearray()
    for y in range(height):
        for x in range(0, width, 2):
            byt = (pixels[x,y] // 17) + ((pixels[x+1,y] // 17) << 4)
            image.append(byt)
    header = struct.pack('<BBHHHH', 2, 15, posx, posy, width, height)
    s.sendall(header)
    s.sendall(image)
    header = struct.pack('<BBHHHH', 4, 0, posx, posy, width, height)
    s.sendall(header)


opts = webdriver.FirefoxOptions()
opts.headless = True
browser = webdriver.Firefox(options=opts)
url = 'http://192.168.0.42:3000/d/x847S8ZRk/home-monitoring?refresh=5s&kiosk'
# url = 'http://192.168.0.42:3000/d/x847S8ZRk/home-monitoring?viewPanel=2&refresh=5s&kiosk'
# url = 'https://en.wikipedia.org/wiki/Pongal_(festival)'
browser.get(url)
browser.set_window_size(1872*0.7, 1404*0.7)
time.sleep(3)
browser.save_screenshot('screen_shot.png')

clearScreen(15)
time.sleep(1)
origImage = Image.open('screen_shot.png')
origImage = origImage.resize((1872, 1404))
drawImage(0, 0, origImage)
# browser.close()
# browser.quit()

time.sleep(1)
s.close()
