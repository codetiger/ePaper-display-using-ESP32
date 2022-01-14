#include "pic.h"
#include <WiFi.h>
#include "config.h"

#define MAX_BATCH 1024

const uint serverPort = 8319;
WiFiServer server(serverPort);

typedef struct {
  uint8_t command;
  uint8_t color;
  uint16_t x, y;
  uint16_t w, h;
} CommandHeader;

void setup(void) {
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(HRDY, INPUT);

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("Tring to connect to Wifi SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  Serial.print("WiFi connected successfully. ");
  Serial.print("Assigned IP: ");
  Serial.println(WiFi.localIP());

  if (IT8951_Init() != 0) {
    Serial.println("Display init failed");
    exit(1);
  };
  Serial.println("Display init successful");

  server.begin();
}

extern unsigned char pic[];
extern unsigned int pic_width;
extern unsigned int pic_height;

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      while (client.available() > 0) {
        CommandHeader header;
        int size = client.read((uint8_t *)&header, sizeof(CommandHeader));
        Serial.print("Data Read: ");
        Serial.println(size);
        if(size == sizeof(CommandHeader)) {
          Serial.print("Command: ");
          Serial.println(header.command);

          if(header.command == 1) {
            IT8951LoadDataStart(header.x, header.y, header.w, header.h);
            uint8_t c = header.color & 0xf;
            c = (c << 4) | (header.color & 0xf);
            for (int j = 0; j < header.h; j++)
              for (int i = 0; i < header.w / 2; i++)
                IT8951LoadDataColor(c);
            IT8951LoadDataEnd();
          } else if(header.command == 2) {
            IT8951LoadDataStart(header.x, header.y, header.w, header.h);
            int totalBytes = header.h * header.w / 2;
            while(totalBytes > 0) {
              int batchSize = totalBytes;
              if(batchSize > MAX_BATCH)
                batchSize = MAX_BATCH;
              if(batchSize > client.available())
                batchSize = client.available();

              if(batchSize == 0) {
                delay(2);
                continue;
              }
              uint8_t *c = new uint8_t[batchSize];
              int size = client.read(c, batchSize);
              if(size != -1) {
                IT8951LoadDataColor(c, size);
                free(c);
                totalBytes -= size;
              }
            }
            IT8951LoadDataEnd();
          } else if(header.command == 4) {
            IT8951DisplayArea(header.x, header.y, header.w, header.h, 2);
          } else if(header.command == 8) {
            int x = header.x;
            int y = header.y;
            gpFrameBuf = pic;
            IT8951_BMP_Example(x, y, pic_width, pic_height);
            IT8951DisplayArea(x, y, pic_width, pic_height, 2);
            LCDWaitForReady();
          }
        }
      }
      delay(10);
    }
 
    client.stop();
    Serial.println("Client disconnected");
  }
}
