---
created at: 2017-10-07
tags:
  - arduino
  - project
---

オリジナルの作成：2014/07/06

# 0A-MacOSXへのinoのインストールと使い方
## 動機
Arduino勉強会では、目の不自由な方も参加されており、コマンドラインからの コンパイルとアップロードができるようにしたいのとの要望がありました。

調べたところ、Arduinoにコマンドラインでプログラム、書込ができるinoが 以下のサイトに見つかりました。

- http://inotool.org/

### inoに必要なもの †
最新のinoのバージョンは、0.3.6で、 GitHubで公開されており、pythonのeasy_installを使って簡単にインストール することができます。

inoを使うのに必要なものは、以下の３つです。

- Python 2.6以降
- Arduino IDE
- picocom （シリアル通信ソフト）

現在inoがサポートしているOSは、以下の２つです。

- Linux
- MacOSX

## MacPortsを使ってMacOSXにInoをインストール 
私は、MacPortsを使って必要なソフトをインストールしています。 

MacPortsを使わない場合には、以下のサイトを参考にしてください。

- [ino使うとCUIでArduinoをビルドできて便利](http://shokai.org/blog/archives/7454)

MacPortsの開発には、アップルの提供しているXCodeが必要です。

以下のサイトからMacPortsの最新をダウンロードしてインストールしてください。

- http://www.macports.org/

Xcodeのインストールは、MacApp Storeからダウンロードしてください。

- Xcode

コマンドラインツールのインストールには、Xcodeの

- Xcode-> Open DeveloperTool -> More Developer Tools… を選択
- Auxiliary Tools for Xcodeを選択し、Downloads for Apple Developerページが開くのでそこのリストから”Command Line Tools(OS X Mavericks) for Xcode”をダウンロードしてインストールしてください

### Python2.7のインストール 
以下の作業は、アプリケーション/ユーティリティ/ターミナルを使って行います。

Pythonは、いろいろなバージョンが使われており、ここでは古いですが安定した python2.7を使います。

```bash
$ sudo port install python27
```

### picocomのインストール 
同様にpicocomをインストールします。

```bash
$ sudo port install picocom
```

### inoのインストール
inoのインストールは、easy_installを使って行います。python2.7用の easy_install-2.7で以下の様にインストールします。 

```bash
$ sudo /opt/local/bin/easy_install-2.7 ino
```



## inoの使い方

### プロジェクトの作成
新しいスケッチを作成するために、プロジェクト用のディレクトリを作成し、 テンプレートからスケッチのひな形を作成します。

用意されているテンプレートは、以下の２つです。

- blink : Basic01のBlinkと同じスケッチがひな形として作成されます。
- empty : 空のsetupとloopが作成されます。

早速プロジェクトをつくってみましょう。

```bash
$ mkdir test
$ cd test
$ ino init -t blink
```

blinkテンプレートを使ってinoのinitコマンドを実行すると 以下の様にsrc, libのディレクトリが作成され、srcの下にsketch.inoファイル が作成されます。

```bash
$ ls -R
libsrc

./lib:

./src:
sketch.ino
```

sketch.inoの中身は、以下の様になっています。

```bash
$ cat src/sketch.ino 
```

```C++
#define LED_PIN 13

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(900);
}
```

### スケッチのビルド
Arduino IDEでも最初にすることは、ボードの選択ですが、inoでも 同様です。

inoのデフォルトボードは、unoですが、それ以外のボードを使う場合 には、list-modelsコマンドで使用するボードに対応するモデル名を確認します。

```bash
$ ino list-models
Searching for Board description file (boards.txt) ... /Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/boards.txt
         uno: [DEFAULT] Arduino Uno
   atmega328: Arduino Duemilanove w/ ATmega328
   diecimila: Arduino Diecimila or Duemilanove w/ ATmega168
     nano328: Arduino Nano w/ ATmega328
        nano: Arduino Nano w/ ATmega168
    mega2560: Arduino Mega 2560 or Mega ADK
        mega: Arduino Mega (ATmega1280)
    leonardo: Arduino Leonardo
     esplora: Arduino Esplora
       micro: Arduino Micro
     mini328: Arduino Mini w/ ATmega328
        mini: Arduino Mini w/ ATmega168
    ethernet: Arduino Ethernet
         fio: Arduino Fio
       bt328: Arduino BT w/ ATmega328
          bt: Arduino BT w/ ATmega168
  LilyPadUSB: LilyPad Arduino USB
  lilypad328: LilyPad Arduino w/ ATmega328
     lilypad: LilyPad Arduino w/ ATmega168
    pro5v328: Arduino Pro or Pro Mini (5V, 16 MHz) w/ ATmega328
       pro5v: Arduino Pro or Pro Mini (5V, 16 MHz) w/ ATmega168
      pro328: Arduino Pro or Pro Mini (3.3V, 8 MHz) w/ ATmega328
         pro: Arduino Pro or Pro Mini (3.3V, 8 MHz) w/ ATmega168
   atmega168: Arduino NG or older w/ ATmega168
     atmega8: Arduino NG or older w/ ATmega8
robotControl: Arduino Robot Control
  robotMotor: Arduino Robot Motor
```

私は、Arduino Duemilanoveを使っているので、atmega328がボード を表すモデル名となります。

以下のコマンドでビルドを実行します。

```bash
$ ino build -m atmega328
Searching for Board description file (boards.txt) ... /Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/boards.txt
途中省略
arduino/WString.cpp
Linking libarduino.a
Linking firmware.elf
Converting to firmware.hex
```

エラーも無くビルドが終了しました。

しかし、lsコマンドでみても何も変わったものはありません。

```bash
$ ls
lib     src
```

ビルドコマンド作成されたファイルは、見えないディレクトリ.build/モデル名/以下のあります。

```bash
$ ls  .build/atmega328/
Makefile     Makefile.sketch     firmware.elf     src
Makefile.deps     arduino          firmware.hex
```

Arduinoに書き込むのは、firmware.hexファイルです。

### スケッチの書き込み
スケッチの書込には、uploadコマンドを使います。

```bash
$ ino upload -m atmega328
Searching for stty ... /bin/stty
途中省略
Writing | ################################################## | 100% 0.56s

avrdude: 1076 bytes of flash written
avrdude: verifying flash memory against .build/atmega328/firmware.hex:
avrdude: load data flash data from input file .build/atmega328/firmware.hex:
avrdude: input file .build/atmega328/firmware.hex contains 1076 bytes
avrdude: reading on-chip flash data:

Reading | ################################################## | 100% 0.46s

avrdude: verifying ...
avrdude: 1076 bytes of flash verified

avrdude: safemode: Fuses OK

avrdude done.  Thank you.
```

無事書き込みが終了すると、ボードのLEDが点滅します。

### 設定ファイル
buildやupload時にモデル名を設定するのは面倒です。

プロジェクトファイルにino.ini、あるいはホームディレクトリに~/.inorcを作成し、 以下のようにモデル名やシリアルポートをセットすることができます。

```bash
$ cat ino.ini
[build]
board-model = atmega328

[upload]
board-model = atmega328
serial-port = /dev/tty.usbserial-A7006T8Y

[serial]
serial-port = /dev/tty.usbserial-A7006T8Y
```

これで、単にbuild, uploadとコマンドを入力するだけで済みます。

```bash
$ ino build
```

### シリアル通信
inoでは、Arduino IDEのシリアルモニターの代わりにserialコマンドが用意されています。

Quick startに習って、スケッチを以下の様に変更します。

```bash
$ cat src/sketch.ino
```

```C++
void setup()
{
    Serial.begin(9600);
}

void loop()
{
    Serial.println(millis());
    delay(1000);
}
```

ビルドして、serialコマンドを実行すると以下の様に出力されます。 途中で止める時には、Ctrl-A, Ctrl-Xを入力します。

```bash
$ ino build
出力省略
$ ino upload
出力省略
$ ino serial
Searching for Serial monitor (picocom) ... /opt/local/bin/picocom
picocom v1.7

port is        : /dev/tty.usbserial-A7006T8Y
flowcontrol    : none
baudrate is    : 9600
parity is      : none
databits are   : 8
escape is      : C-a
local echo is  : no
noinit is      : no
noreset is     : no
nolock is      : yes
send_cmd is    : sz -vv
receive_cmd is : rz -vv
imap is        :
omap is        :
emap is        : crcrlf,delbs,

Terminal ready
0
999
1999
3000
4000
5000

ここで、Ctrl-A Ctrl-Xを入力して停止

Thanks for using picocom
```
