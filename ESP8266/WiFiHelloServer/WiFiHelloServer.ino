#include <ESP8266WiFi.h>
#include <FS.h>

const char *ssid =  "WiFiのSSID";
const char *password =  "WiFiのパスワード";

// サーバインスタンスを生成（８０番ポートで待ち受け）
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  delay(10);

  bool result = SPIFFS.begin();
  
  // WiFiに接続
  Serial.println();
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

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
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
  client.flush();

  // index.htmlの内容を返す
  File f = SPIFFS.open("/index.html", "r");
  if (!f) {
      Serial.println("file open failed");
      return;
  }  
  Serial.println("====== Reading from SPIFFS file =======");

  while(f.available()) {
    String line = f.readStringUntil('\n');
    client.print(line);
  }
  f.close();
  
  delay(1);
  Serial.println("Client disonnected");
}

