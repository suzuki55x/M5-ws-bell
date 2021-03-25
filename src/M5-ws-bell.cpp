// MEMO:  SDに固有名(設置場所とか)書いておいてもいいかも。

//#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#define BEAT 300
#define SPEAKER_PIN 25

#include <M5Stack.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "WiFi.h"

const char* wifi_fname = "/wifi.csv";
char ssid[32];
char pass[32];
const char* ws_host = "192.168.1.10";
//const char* ws_host = "192.168.10.5";
const int   ws_port = 4000;
const char* ws_path = "/socket/websocket";
const char* image1 = "/disp.jpg";

WebSocketsClient webSocket;
DynamicJsonDocument doc(1024);

// 設定ファイルからSSIDとパスワードを読み出して接続
void wifiInit() {
  int cnt = 0;
  File fp = SD.open(wifi_fname, FILE_READ);
  char data[64];
  char *str;

  M5.Lcd.setCursor(0, 20);
  M5.Lcd.print("Wi-Fi connection");

  if(!fp) {
    M5.Lcd.println("\nERR: SD");

    delay(10000);
    esp_restart();
  }

  while(fp.available()) {
    data[cnt++] = fp.read();
  }
  str = strtok(data, ",");
  strncpy(&ssid[0], str, strlen(str));
  str = strtok(NULL, " \n");
  strncpy(&pass[0], str, strlen(str));

  WiFi.begin(ssid, pass);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 42);
  M5.Lcd.printf("\nSSID: %s", ssid);
  M5.Lcd.printf("\nPASS: %s", pass);
  M5.Lcd.printf("\nconnecting...");
  int resetCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
    if (resetCnt++ > 30) {
      esp_restart();
    }
  }
  M5.Lcd.setTextSize(2);
}

void beep(int freq, int duration, uint8_t volume) {
  // freq(Hz), duration(ms), volume(1~255)
  int t = 1000000 / freq / 2;
  unsigned long start = millis();
  while ((millis() -start) < duration) {
    dacWrite(SPEAKER_PIN, 0);
    delayMicroseconds(t);
    dacWrite(SPEAKER_PIN, volume);
    delayMicroseconds(t);
  }
  dacWrite(SPEAKER_PIN, 0);
}

// ピンポン(ボタン押した時)
void chimeA(String type="other") {
  if (type=="use") {
    beep(659, 500, 10);
  } else if (type=="mtg") {
    beep(523, 1000, 10);
  } else {
    beep(659, 500, 10);
    beep(523, 1000, 10);
    delay(400);
    beep(659, 500, 10);
    beep(523, 1000, 10);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  //M5.Lcd.print("event");
  M5.Lcd.setCursor(0, 20);
  char *json = (char *)payload;
  deserializeJson(doc, json);

  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[WSc] Disconnected!\n");
      //M5.Lcd.print("disconnected");
      break;
    case WStype_CONNECTED:
      //Serial.printf("[WSc] Connected to url: %s\n", payload);
      //M5.Lcd.print("connected");
      // {"topic":"room:lobby","ref":1, "payload":{},"event":"phx_join"}
      webSocket.sendTXT("{\"topic\":\"room:lobby\",\"ref\":\"1\", \"payload\":{},\"event\":\"phx_join\"}");
      break;
    case WStype_TEXT:
      //M5.Lcd.fillScreen(BLACK);
      //Serial.printf("[WSc] get text: %s\n", payload);
      //M5.Lcd.printf("message: %s\n", doc["payload"]["body"]["msg"].as<String>().c_str());
      //M5.Lcd.printf("event: %s\n", doc["event"].as<String>().c_str());
      if(doc["event"].as<String>()=="call") chimeA(doc["payload"]["body"]["type"].as<String>());
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void initWebSocket() {
  M5.Lcd.print("A");
  webSocket.begin(ws_host, ws_port, ws_path);
  M5.Lcd.print("B");

  // event handler
  webSocket.onEvent(webSocketEvent);

  webSocket.setReconnectInterval(5000);
  M5.Lcd.print("C");
}

void setup() {
  M5.begin();

  M5.Lcd.setBrightness(100);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN , BLACK);
  M5.Lcd.setTextSize(2);

  // Wi-Fi設定
  wifiInit();

  initWebSocket();
  // 画面クリア
  M5.Lcd.fillScreen(BLACK);

  M5.Lcd.drawJpgFile(SD, image1);
}

void loop() { 
  bool isPressedBtn  = false;
  bool isPressedBtnA = false;
  bool isPressedBtnB = false;
  bool isPressedBtnC = false;
  char* type = "";
  if(M5.BtnA.wasPressed())
  {
    isPressedBtn  = true;
    isPressedBtnA = true;
    type = "use";
  }
  if(M5.BtnB.wasPressed())
  {
    isPressedBtn  = true;
    isPressedBtnB = true;
    type = "mtg";
  }
  if(M5.BtnC.wasPressed())
  {
    isPressedBtn  = true;
    isPressedBtnC = true;
    type = "other";
  }
  static uint32_t pre_send_time = 0;
  uint32_t time = millis();
  if(isPressedBtn && time - pre_send_time > 100){
    pre_send_time = time;
    String ws_str = R"({"topic":"room:lobby", "ref":1, "payload":{"body":{"type":")"+ (String)type +R"(", "msg":"time:[)" + (String)time + R"(]"}},"event":"call"})";
    //Serial.println(btn_str);
    webSocket.sendTXT(ws_str);
    // chimeA();
  }
  webSocket.loop();

  M5.update();
}
