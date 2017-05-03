#include <ESP8266WiFi.h>
#include <FS.h>
#include <SoftwareSerial.h>

#define FILENAME  "/cam.jpg"
#define LED_PIN 13

extern "C" {
#include "user_interface.h"
}

const char *ssid =  "WiFiのSSID";
const char *password =  "WiFiのパスワード";

static char     buf[128];
static uint8_t  line[256];

// サーバインスタンスを生成（８０番ポートで待ち受け）
WiFiServer      server(80);
SoftwareSerial  softwareSerial(12, 14);

char *getReqURI(String req) {
  // HTTP要求からURIを取り出す  
  req.toCharArray(buf, 128);
  char *method = strtok(buf, " ");
  char *uri = strtok(NULL, " ");
  Serial.print("URI:");
  Serial.println(uri);
  char *type = strtok(NULL, " ");
  
  return uri;
}


void setup() {
  Serial.begin(9600);
  delay(10);

  bool result = SPIFFS.begin();
  
  // WiFiに接続
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // カメラ電源ON
  Serial.println("Initialize Camera");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);      
  // カメラの初期化
  softwareSerial.begin(115200);
  CamInitialize();
  CamSizeCmd(1);

  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.println(WiFi.localIP());
}

void takeSnap(WiFiClient agent) {
  Serial.println("Captrue image");
  File file = SPIFFS.open(FILENAME, "w");
  if (file != NULL) {
    Serial.println("Cam Capt");
    CamCapture();
    size_t size = CamGetData(file);
    file.close();
    // 再度ファイルから読み取る  
    file = SPIFFS.open(FILENAME, "r");
    if (file) {
      agent.println("HTTP/1.0 200 OK");
      agent.println("Content-Type: image/jpeg");
      agent.print("Content-Length: ");
      agent.println(size);
      agent.println("Connection: close");
      agent.println();
      for (size_t t = 0; t < size; ) {
        if (!agent.connected())
          break;  // 切断されたら転送終了
        int len = file.read(line, sizeof(line));
        if (len >0)
          agent.write((const uint8_t*)line, len);
        t += len;
        delay(1);  // WDT対策
      }
      file.close();
    }
   }
}

void loop() {
  // クライアントからの接続要求があるかチェック
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // クライアントからのHTTP要求を待つ
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  // リクエスを読み込む
  String req = client.readStringUntil('\r');
  Serial.println(req);
  // HTTP要求からURIを取り出す
  char *uri = getReqURI(req);

  if (strcmp(uri, "/getTemp") == 0) {
    float t = ((float)system_adc_read())/1023.0*100;
    // float t = 21.0;
    client.print("{ \"temperature\" : ");
    client.print(t);
    client.println("}");
  }
  else if (strstr(uri, "takeSnap")) {
    takeSnap(client);   
  }
  else {
    // Prepare the response
    File f = SPIFFS.open(uri, "r");
    if (!f) {
        Serial.println("file open failed");
        return;
    }  
    Serial.println("====== Reading from SPIFFS file =======");
  
    while(f.available()) {
      if (!client.connected())
        break;  // 切断されたら転送終了
      int len = f.read(line, sizeof(line));
      if (len > 0)
        client.write((const uint8_t*)line, len);
      delay(1);  // WDT対策
    }
    f.close();   
  }
  client.flush();
  client.stop();
  
  delay(1);
  Serial.println("Client disonnected");
}

