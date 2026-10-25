---
created: 2026-09-10T20:49
updated: 2026-10-25T16:26
tags:
  - VibeCoding
  - OpenCode
  - brew
  - conda
  - GitBash
---
## OpenCodeを使ってVibe Codingを試してみた
OpenCodeを使ってVibe Codingを試してみました。最初に作業ディレクトリに置いてあるのは、AGENTS.mdというOpenCodeのプロジェクト運用指示書のみです。

これだけでOpenCodeとユーザのやり取りがどのように進行するかを見ていきましょう。

## Windowsの場合の変更点
インストールに必要なもの
- python
	- wingetでpython 3.12をインストール
- Git Bash
- PlatformIO
	- pythonのインストールが完了すると自動的にインストールを開始した
- C:\proj\VibeCodingにプロジェクトを作成

## 大前提
AGENTS.mdに細かな指示を埋め込んだ。Macではbrewとuvを使って環境を整え、Windowsでは、Git Bashを使います。

## Visual Studio Codeを使った指示
ユーザ：
```
LEDを点滅するスケッチを作りたいです。どのようにすればよいかおしえてください。
PlatformIOのプロジェクトをこのディレクトリに作成し、スケッチの雛形を作って。
```

OpenCode：
```
PlatformIOプロジェクトの作成が完了しました。
作成されたファイル:
- platformio.ini - UIAPduino用の設定
- src/main.cpp - LED点滅スケッチ（D2 = 内蔵LED）
- include/ - ヘッダーファイル用ディレクトリ
- lib/ - プライベートライブラリ用ディレクトリ
スケッチの内容:
- D2ピン（内蔵LED）を1秒間隔で点滅させます
ビルドやアップロードが必要な場合はお知らせください。
```

ユーザ：
```
点滅間隔を変更するにはどうすればよいですか
```

OpenCode：
```
src/main.cppのdelay(1000)の値を変更します。単位はミリ秒です。
// 例: 500ミリ秒（0.5秒）間隔
delay(500);
// 例: 2秒間隔
delay(2000);
```

ユーザ：
```
変更しました。実行してみてください
```

OpenCode：
```
変更内容を確認してからビルド＆アップロードします。

500ミリ秒に変更されています。ビルド＆アップロードします。

$ pio run --target upload
Processing UIAPduino_Pro_Micro_CH32V003_v1dot4 (platform: ch32v; board: UIAPduino_Pro_Micro_CH32V003_v1dot4; framework: noneos-sdk)
以下省略

ビルド＆アップロードが成功しました。LEDが500ミリ秒間隔で点滅しています。
```

ユーザ：
```
タクトスイッチを 10番ピンとGNDに接続しました。特に抵抗は付けていません。
タクトスイッチを押した時だけLEDが点灯するにはスケッチをどのように変更すればよいですか。
ステップ・バイ・ステップで教えて
```

OpenCode：
```
ステップ・バイ・ステップで説明します。
ステップ1: タクトスイッチの初期化
setup()内でD10ピンを入力として設定し、内蔵プルアップ抵抗を有効にします。
#define SWITCH_PIN 10

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);  // 内蔵プルアップで HIGH になる
}
ステップ2: メインループで状態を読み取り
loop()内でスイッチの状態を読み取り、押されたらLEDを点灯させます。
void loop() {
  if (digitalRead(SWITCH_PIN) == LOW) {
    digitalWrite(LED_PIN, HIGH);  // スイッチ押下 → LED点灯
  } else {
    digitalWrite(LED_PIN, LOW);   // スイッチ開放 → LED消灯
  }
  delay(50);  // チャタリング防止
}
補足
- INPUT_PULLUP: 内蔵プルアップ抵抗を使い、スイッチ未押下時にHIGH、押下時にGNDに接続されてLOWになります
- delay(50): チャタリング（スイッチのバネによる一瞬の断続）を防止するための簡易的なデバウンス処理です
```

ユーザ：
```
チャタリング防止とはなんですか。
```

OpenCode：
```
スイッチの接点が押した時に数ミリ秒の間に何度も開閉し、一瞬で複数回の入力が発生する現象です。
物理的なスイッチの接点が弾むことで起き、MCUから見ると「押した→離した→押した→離した…」と高速で繰り返されます。
delay(50)で一定時間待ち、最初の入力だけを有効にする簡易的な対策です。
```

ユーザ：
```
これで一度動かしてみて
```

OpenCode：
```
まだ点滅スケッチのままです。スイッチ対応に修正してアップロードします。

# Wrote src/main.cpp
#include <Arduino.h>

#define LED_PIN 2
#define SWITCH_PIN 10

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(SWITCH_PIN) == LOW) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(50);
}

$ pio run --target upload
Processing UIAPduino_Pro_Micro_CH32V003_v1dot4 (platform: https://github.com/Community-PIO-CH32V/platform-ch32v.git; board: UIAPduino_Pro_Micro_CH32V003_v1dot4; framework: arduino)
途中省略
[wch_riscv.cpu.0] Target successfully examined.
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
========================= [SUCCESS] Took 4.50 seconds =========================

アップロード完了。D10のスイッチを押すとLEDが点灯し、離すと消灯します。
```

### VScodeの画面とUIAPduino
OpencodeをVSCodeのプラグインから操作しているスナップショットです。

<img src="images/45/VibeCoding−VScode.png" width="650">

WCH-LinkEと接続している様子です。

<img src="images/45/UIAPduino+WCH−LinkE.jpg" width="500">

- <a href="code/45/AGENTS.md">AGENTS.md</a>

## 発見
WCH-LinkEのシリアル通信は、おそらく921600ボーが最大です。これはWCH-LinkEがHIDとしてUSBに接続していることが要因として考えられます。
シリアル通信の実験で、CH32V003の内蔵HSIで、+0.9%の個体差を確認することができました。

### Mac mini M4のollamaでも試す
最後に、ローカルLLMでどこまでできるかをollamaを使って試してみました。

少し遅い感じはしますが、きちんと処理を実行し、シリアルの時間測定とテストスクリプトの作成までできました。