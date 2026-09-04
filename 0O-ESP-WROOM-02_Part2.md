---
created at: 2017-10-15
tags:
  - arduino
  - project
---

オリジナルの作成：2015/10/31

## Arduinoのスケッチを作る
Arduino勉強会/0N-WiFiモジュールその１ でESP-WROOM-02を使ってWebサーバにアクセスできるところまで確認しました。

今回は、プログラムを書いてESP-WROOM-02を操作してみましょう。


### 定数の定義
ATコマンドやSSID、パスワードは#define分で定義しておきます。

```C++
#define ST_SSID           "<<SSIDを指定>>"
#define ST_PASSWD     "<<パスワードを指定>>"

#define SERVER_ADDR   "49.212.164.205"
#define SERVER_PORT   80
#define CRLF          "\r\n"
#define CRLF2         "\r\n\r\n"

#define STATION_MODE  1
```


### コマンドを関数にまとめる
以下のATコマンドを関数に整理します。

- AT+CWMODE
- AT+CWJAP
- AT+CIPSTART
- AT+CIPSEND

ATコマンドは終わりにCR+LFを追加するためにatCommand関数も一緒に作ります。

```C++
int atCommand(char *cmd) {
  Serial.print(cmd);
  Serial.print(CRLF);
  return 1;
}

int atCwMode(int mode) {
  sprintf(buf, "AT+CWMODE=%d", mode);
  return atCommand(buf);
}

int atCwJap(char *ssid, char *passwd) {
  sprintf(buf, "AT+CWJAP=\"%s\",\"%s\"", ssid, passwd);
  return atCommand(buf);
}

int atCipStart(char *server_addr, int server_port) {
  sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d", server_addr, server_port);
  return atCommand(buf);
}
int atCipSend(char *uri) {
  sprintf(buf, "GET %s HTTP/1.0%s%s", uri, CRLF, CRLF2);
  int len = strlen(buf);
  Serial.print("AT+CIPSEND=");
  Serial.print(len);
  Serial.print(CRLF);
  delay(1000);
  Serial.print(buf);

}
```
### 初期化
ESP-WROOM-02の操作の内、以下の処理は1回だけすればよいので初期化（setup） にまとめます。

- モード設定：
- ルータに接続：

```C++
  atCwMode(STATION_MODE);
  delay(1000);
  atCwJap(ST_SSID, ST_PASSWD);
```

### ボタンアクションの追加
タクトスイッチのボタンを押したときに、Sageサーバにアクセスするように スケッチを修正します。

```C++
  if (digitalRead(sw_pin) == LOW) {
    pc.println("SW pressed");
    delay(500);
    atCipStart(SERVER_ADDR, SERVER_PORT);
    delay(1000);
    atCipSend("/hello.html");  
  }
```

## テスト
準備ができたので、スケッチを動かしてみます。 コマンドの実行状況がシリアルモニタに表示されるので、プログラムミスや設定ミスがすぐに分かります。

```
ESP8266IF3tTest
AT+CWMODE=1

OK
AT+CWJAP="<<SSIDを指定>>","<<パスワードを指定>>"

WIFI DISCONNECT
WIFI CONNECTED
WIFI GOT IP

OK
SW pressed
AT+CIPSTART="TCP","49.212.164.205",80

CONNECT

OK
AT+CIPSEGET /hello.html HTTP/1.0
Recv 30 bytes

SEND OK

+IPD,303:HTTP/1.1 200 OK
Date: Tue, 27 Oct 2015 12:29:51 GMT
Server: Apache
Last-Modified: Sat, 24 Oct 2015 23:49:16 GMT
ETag: "c005e-34-522e260431c88"
Accept-Ranges: bytes
Content-Length: 52
Connection: close
Content-Type: text/html; charset=UTF-8

<html>
<body>
<h1>Hello World!</h1>
</body>
</html>
CLOSED
```


## atCommandの変更
これまでatCommand関数が正しく処理できたかを確認していませんでした。

ATコマンドでは正しく処理できたときにOK、失敗の時にERRORを返します。 そこで、最後に返された文字がKかRで正常修了と異常終了の判定することにします。

atCommandを以下の様に変更して、再度スケッチを実行してみます。

処理によって、OKがでるまでの時間が異なるため、atCommandにコマンドを送った後の 待ち時間を入れることにします。

レスポンスの出力に少し文字の取りこぼしがありますが、何とかHello Worldのメッセージ を受信できるようになりました。

現段階のスケッチは、以下の通りです。

```C++
#include <SoftwareSerial.h>


#define ST_SSID           "<<SSIDを指定>>"
#define ST_PASSWD     "<<パスワードを指定>>"

#define SERVER_ADDR   "49.212.164.205"
#define SERVER_PORT   80
#define CRLF          "\r\n"
#define CRLF2         "\r\n\r\n"

#define STATION_MODE  1
#define WAIT_ONECE    2000
#define OK            'K'
#define ERR           'R'
#define FAIL          'L'

char buf[128];

int interval_waits[] = {50, 80, 160, 200, 300, 600, 1200, 1500};
int retry_count = sizeof(interval_waits)/sizeof(int);
int sw_pin = 10;
int sTx_pin = 12;
int sRx_pin = 11;
int c;

SoftwareSerial  pc(sRx_pin, sTx_pin);

int last_char = -1;
int waitForResponse() {
  last_char = -1;
  int err_count = 0;
  // 返されたデータをすべて読み込む
  for (int i = 0; i < retry_count; i++) {
    while (Serial.available()) {
      while((c = Serial.read()) != -1) { 
        if (c != '\r' && c != '\n')
          last_char = c;
        // デバッグ用
        pc.write(c);
      } 
    }
    // OK, ERROR, > , FAIL, CLOSED
    if (last_char == OK || last_char == ERR || last_char == ' ' || last_char == 'L' || last_char == 'D') break;
    // pc.println("wait");
    delay(interval_waits[err_count++]);
  }
}

int atCommand(char *cmd, int wait_time) {
  int c;
  
  Serial.print(cmd);
  Serial.print(CRLF);
  delay(wait_time);
  // 応答を待つ
  waitForResponse();
  // 最後に返された文字がKまたはRでなかったら、WAIT_ONECEミリ秒待つ
  if (last_char != OK && last_char != ERR && last_char != ' ') {
    delay(WAIT_ONECE);
    pc.println("Do wait onece");
    if (Serial.available()) {
      while((c = Serial.read()) != -1) { 
        if (c != '\r' || c != '\n')
          last_char = c;
        // デバッグ用
        pc.write(c);
      } 
    }
  }
  if (last_char == OK)
    return 1;
  else if (last_char == ERR)
    return 0;
  else
    return -1;
}

int atCwMode(int mode) {
  sprintf(buf, "AT+CWMODE=%d", mode);
  return atCommand(buf, 0);
}

int atCwJap(char *ssid, char *passwd) {
  sprintf(buf, "AT+CWJAP=\"%s\",\"%s\"", ssid, passwd);
  return atCommand(buf, 2000);
}

int atCipStart(char *server_addr, int server_port) {
  sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d", server_addr, server_port);
  return atCommand(buf, 500);
}

int atCipSend(char *uri) {
  sprintf(buf, "GET %s HTTP/1.0%s%s", uri, CRLF, CRLF2);
  int len = strlen(buf);
  char tmp[32];
  sprintf(tmp, "AT+CIPSEND=%d%s", len, CRLF);
  // uriを送る
  atCommand(tmp, 1000);
  Serial.print(buf);
  // 応答を待つ
  waitForResponse();
}

void setup() {
  pc.begin(9600);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(sw_pin, INPUT);
  pc.println("ESP8266IF3tTest");
  delay(1000);
  atCwMode(STATION_MODE);
  atCwJap(ST_SSID, ST_PASSWD);
}

void loop() {
  if (pc.available()) {
    while((c = pc.read()) != -1)
      Serial.write(c);
  }

  if (Serial.available()) {
    pc.println("Got Response");
    while((c = Serial.read()) != -1)
      pc.write(c);
  }

  if (digitalRead(sw_pin) == LOW) {
    pc.println("SW pressed");

    delay(500);
    atCipStart(SERVER_ADDR, SERVER_PORT);
    atCipSend("/hello.html");  
  }
}
```

## クラスにまとめる
取りあえず、HTTPサーバにGET要求を送るだけのクラスEspClientを以下の様に定義し、 作成しました。

```C++
class EspClient {
public:
  EspClient(int rx_pin, int tx_pin);
  void begin(int baud);
  int  atCommand(char *cmd, int wait_time);
  int  atCwMode(int mode);
  int  atCwJap(char *ssid, char *passwd);
  int  atCipStart(char *server_addr, int server_port);
  int  atCipSend(char *uri);
  int  atRest();
  void atClose();
  void println(char *s);
protected:
  int  waitForResponse();
  static const int  interval_waits[8];
  static const int  retry_count;
private:
  char            buf[256];
  int             last_char;
  SoftwareSerial  pc;
};
```

### クラスのテスト
先ほどのスケッチをEspClientを使った形に修正します。

```C++
#include "EspClient.h"
#include <SoftwareSerial.h>


#define ST_SSID           "<<SSIDを指定>>"
#define ST_PASSWD     "<<パスワードを指定>>"
#define SERVER_ADDR   "49.212.164.205"
#define SERVER_PORT   80

int sw_pin = 10;
int sTx_pin = 12;
int sRx_pin = 11;

EspClient  esp(sRx_pin, sTx_pin);

void setup() {
  pinMode(sw_pin, INPUT);
  esp.begin(9600);
  esp.println("ESP8266IF3tTest");
  esp.atCwMode(STATION_MODE);
  esp.atCwJap(ST_SSID, ST_PASSWD);
}

void loop() {
  if (digitalRead(sw_pin) == LOW) {
    esp.println("SW pressed");
    // チャタリング防止
    delay(500);
    esp.atCipStart(SERVER_ADDR, SERVER_PORT);
    esp.atCipSend("/hello.html");  
  }
}
```

### スケッチのダウンロード
ESPClientTestのスケッチを以下からダウンロードできます。

- ESPClientTestスケッチ [fileESPClientTest.zip](data/fileESPClientTest.zip)
