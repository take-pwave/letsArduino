# AGENTS.md

## Project

このプロジェクトは、PlatformIOを使って、Arduino UIAPduinoのプログラミングをします。
エージェントは、直接プログラムをコーディングするのではなく、ユーザの要望を実現するために必要なアドバイスをします。

## エージェントに依頼したいこと
エージェントは、開発環境の構築とコンパイル、アップロードの処理を行います。
ただし、コンパイルエラーが発生した場合には、エラーの原因を特定し、どのように修正すればよいかアドバイスをすることに留めてください。

### PlatformIO環境の注意点
OSがMacOSの場合、PlatformIOは、brewを使ってインストールしてください。
OSがWindowsの場合、python, git bashをインストールした後に、git bash上でPlatformIOをインストールしてください。

UIApduinoのピン配置について以下を参照してください。
| Arduinoピン番号 | マイコン端子名 (ポート) | 主な機能 / 基板上の用途 | 備考・注意点 |
| :--- | :--- | :--- | :--- |
| **D0** | PA1 | GPIO | |
| **D1** | PA2 | GPIO | |
| **D2** | PC0 | GPIO / **内蔵LED** | オンボードLEDと共有。カットパッド(CUT3)で分離可。 |
| **D3** | PC1 | GPIO / **SDA** (I2C) | Qwiic等で使用。3.3Vでプルアップされています。 |
| **D4** | PC2 | GPIO / **SCL** (I2C) | Qwiic等で使用。3.3Vでプルアップされています。 |
| **D5** | PC3 | GPIO | |
| **D6** | PC4 | GPIO / **A2** (アナログ入力) | PWM出力にも対応。 |
| **D7** | PC5 | GPIO / **SCK** (SPI) | 左右共通ピン（使用時注意） |
| **D8** | PC6 | GPIO / **MOSI** (SPI) | 左右共通ピン（使用時注意） |
| **D9** | PC7 | GPIO / **MISO** (SPI) | 左右共通ピン（使用時注意） |
| **D10** | PD0 | GPIO | |
| **D11** | PD1 | GPIO / **SWDIO** | **書き込み（デバッグ）兼用ピン**。通常GPIOとしての使用は非推奨。 |
| **D12** | PD2 | GPIO / **A3** (アナログ入力) | |
| (なし) | PD3 | **D+** (USB信号) | ソフトウェアUSB（rv003usb）通信に使用。 |
| (なし) | PD4 | **D-** (USB信号) | ソフトウェアUSB（rv003usb）通信に使用。 |
| **D15** | PD5 | GPIO / **TX** (シリアル送信) / **A5** | **USBのプルアップに使用**。GPIOで使うとUSB通信に影響が出ます。 |
| **D16** | PD6 | GPIO / **RX** (シリアル受信) / **A6** | |
| **D17** | PD7 | **RESET** | スイッチを介してGNDに接続されています。 |

UIAPduinoの開発環境は、開発途中のため platform.ini に以下の設定を含めてください。

```
[env]
platform = https://github.com/Community-PIO-CH32V/platform-ch32v.git
upload_protocol = wch-link
debug_tool = wch-link

[env:UIAPduino_Pro_Micro_CH32V003_v1dot4]
board = UIAPduino_Pro_Micro_CH32V003_v1dot4
framework = arduino
```

## Status

### 完成した内容
- PlatformIOプロジェクトの初期化（platformio.ini）
- 内蔵LED（D2）の点滅スケッチ
- タクトスイッチ（D10）でLEDを点灯制御するスケッチ

### リソース使用量（LED点滅スケッチ）
- RAM: 23.0%（472 bytes / 2048 bytes）
- Flash: 39.8%（6528 bytes / 16384 bytes）

### シリアル通信の知見
- WCH-LinkE の UART ブリッジ上限: 921600bps（CDC 経由、公式仕様）
- 921600bps で受信時刻測定すると揺れは ±0.2ms（115200bps では ±4ms）
- CH32V003 内蔵 HSI は約 +0.9% の個体差あり。millis() の経過と実時間に乖離が生じる
- 点滅間隔 250ms 設定時、MCU millis() 差分は 250ms で一定（921600bps）

### 接続状況
- 内蔵LED: D2ピン
- タクトスイッチ: D10ピン ↔ GND（外部抵抗なし、内蔵プルアップ使用）
