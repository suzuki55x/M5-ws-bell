#include <M5Stack.h>
#include <ArduinoJson.h>
#include "WebSocketsClient.h"
#include "WiFi.h"

const char* wifi_fname = "/wifi.csv";
char ssid[32];
char pass[32];
const char* ws_host = "192.168.10.5";
const int   ws_port = 4000;
const char* ws_path = "/socket/websocket";

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

//std::string parseReceivedJson(uint8_t *payload)
//{
//  char *json = (char *)payload;
//  DeserializationError error = deserializeJson(doc, json);
//
//  if (error) {
//    return "none";
//  }
//
//  JsonObject obj = doc.as<JsonObject>();
//
//  // You can use a String to get an element of a JsonObject
//  // No duplication is done.
//  return obj[String("topic")];
//}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  M5.Lcd.print("event");

  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[WSc] Disconnected!\n");
      M5.Lcd.print("disconnected");
      break;
    case WStype_CONNECTED:
      //Serial.printf("[WSc] Connected to url: %s\n", payload);
      M5.Lcd.print("connected");
      // {"topic":"room:lobby","ref":1, "payload":{},"event":"phx_join"}
      webSocket.sendTXT("{\"topic\":\"room:lobby\",\"ref\":1, \"payload\":{},\"event\":\"phx_join\"}");
      break;
    case WStype_TEXT:
      //Serial.printf("[WSc] get text: %s\n", payload);
      //M5.Lcd.printf("message: %s\n", (char*)parseReceivedJson(payload));
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
}

void loop() { 
  bool isPressedBtnA = false;
  bool isPressedBtnB = false;
  bool isPressedBtnC = false;
  if(M5.BtnA.wasPressed() || M5.BtnA.isPressed())
  {
    isPressedBtnA = true;
  }
  if(M5.BtnB.wasPressed() || M5.BtnB.isPressed())
  {
    isPressedBtnB = true;
  }
  if(M5.BtnC.wasPressed() || M5.BtnC.isPressed())
  {
    isPressedBtnC = true;
  }
  static uint32_t pre_send_time = 0;
  uint32_t time = millis();
  if(time - pre_send_time > 100){
    pre_send_time = time;
    String isPressedBtnAStr = (isPressedBtnA ? "true": "false");
    String isPressedBtnBStr = (isPressedBtnB ? "true": "false");
    String isPressedBtnCStr = (isPressedBtnC ? "true": "false");
    String btn_str = "{\"red\":" + isPressedBtnAStr +
      ", \"green\":" + isPressedBtnBStr +
      ", \"blue\":" + isPressedBtnCStr + "}";
    //Serial.println(btn_str);
//    webSocket.sendTXT(btn_str);
  }
  webSocket.loop();

  M5.update();
}
