---
created at: 2017-03-19
tags:
  - arduino
  - project
---

# Hello World（Lチカ）に挑戦
## HelloWorldプロジェクトを作成
最初に　inoでHelloWorldプロジェクトを作成します。

inoの環境に他のファイルが紛れ込まないように、最初にプロジェクト用のディレクトリ（helloWorld）を作成します。

inoの作業はこのhelloWorldに入って作業を実行します。

```python
!mkdir helloWorld
```

次に、ino initコマンドでinoプロジェクトを作成します。このとき-t blinkのオプションを追加するとテンプレートからblinkをコピーしてくれます。

注）既にプロジェクトが作成されている場合には、以下の作業は不要です（ファイルがある場合にはino initは失敗します）。

```python
%%bash
cd helloWorld
ino init -t blink
```

上記コマンドで以下のファイルが作成されます。
- src： .inoファイルのディレクトリ
- src/sketch.ino: テンプレートからコピーされたblinkスケッチ
- lib: ライブラリのディレクトリ

```python
! ls -lR helloWorld
```

sketch.inoの内容を確認してみましょう。

```python
! cat helloWorld/src/sketch.ino
```

## スケッチの書き込み
次にスケッチをコンパイルし、Arduinoに書き込むの手順を説明します。

### モデル名を調べる
inoでは使っているArduinoを製品名ではなく、モデル名で指定します。

以下のコマンドでモデル名の一覧を表示します。

```python
! ino list-models
```

### スケッチのビルド
私のArduinoはArduino Duemilanoveなので、-m atmega328とモデルを指定しています。

それでは、スケッチをビルドしてみましょう。

```python
%%bash
cd helloWorld
ino build -m atmega328
```

### スケッチのアップロード（書き込み）
最後にino uploadコマンドでArduinoに書き込みます。

環境設定でシリアルポート2を指定しているので、ubuntuでのデバイスは/dev/ttyS1となります。
ino uploadコマンドのポート指定は、-p /dev/ttyS1で指定します。

```python
%%bash
cd helloWorld
ino upload -m atmega328 -p /dev/ttyS1
```

### スケッチの作成と変更
スケッチの作成と変更は%%writefileを使って行います。

delayの間隔を200に変更してみましょう。

```python
%%writefile helloWorld/src/sketch.ino
#define LED_PIN 13

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
}
```

ビルドして、アップロードします。

```python
%%bash
cd helloWorld
ino build -m atmega328
ino upload -m atmega328 -p /dev/ttyS1
```

## シリアルモニタを使ってみる
jupyterノート上でもシリアルの出力をみることができます（残念ながら入力はできません、どなたかご存じの方はtake.pwave@gmail.comまでお知らせください）。

スケッチを以下の様に変更します。

```python
%%writefile helloWorld/src/sketch.ino
#define LED_PIN 13

void setup(){
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  digitalWrite(13, HIGH);
  delay(1000);
  Serial.println("on");
  digitalWrite(13, LOW);
  delay(1000);
  Serial.println("off");
}
```

ビルドして、アップロードします。

```python
%%bash
cd helloWorld
ino build -m atmega328
ino upload -m atmega328 -p /dev/ttyS1
```

スケッチを以下の様に変更してシリアルからの情報を表示してみましょう。

出力を止めるには、JupyterのKernelメニューからInterruptを選択します。


```python
! ino serial -p /dev/ttyS1
```

とても簡単にArduinoプログラミンが体験できます。みなさんもjupyterで　Arduinoプログラミングに挑戦してみてください。

これからもArduinoのスケッチの作成過程をjupyterのノートブックで共有し、電子工作の楽しさを広めたいと願っています。
