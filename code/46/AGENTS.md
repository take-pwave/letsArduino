# AGENTS.md

## Project

このプロジェクトは、PlatformIOとProtothreadsライブラリを使って、Arduino UIAPduinoのプログラミングをします。

## エージェントに依頼したいこと
エージェントは、開発環境の構築とコンパイル、アップロードの処理を行います。
ただし、コンパイルエラーが発生した場合には、エラーの原因を特定し、どのように修正すればよいかアドバイスをしてください。

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

## Protothreads の注意点

### PT_THREAD 内の変数は static にする（必須）

**ウェイト（`PT_SLEEP` / `PT_WAIT_UNTIL` / `PT_YIELD`）をまたいで使う変数は、必ず `static`（またはファイルスコープ）にする。**

理由: Protothreads はスタックを保存しない。ウェイト箇所では `return PT_WAITING` で関数を抜けるため、auto 変数は消え、再開時には**未初期化のスタック領域を読む**ことになる（不定値・他関数の残骸になり得る）。

公式ドキュメント（`pt-1.4-refman.pdf` §1.4 "Local variables"）にも明記されています:
> Because protothreads do not save the stack context across a blocking call, local variables are not preserved when the protothread blocks. … if in doubt, do not use local variables inside a protothread!

**この環境の GCC は警告を出さないのでコンパイラ頼みはできない。** 論文（dunkels05protothreads.pdf §1.3.4）は「gcc が "might be used uninitialized" を警告する」と書くが、riscv-wch-elf-g++ 12.2.0 では `-O0`/`-O1`/`-O2`/`-O3`/`-Os` × `lc-addrlabels`/`lc-switch` の全組合せで **0件**（実機ビルドの `-Os -flto -Wall` でも0件）。規約を自覚的に守る必要がある。

```cpp
static PT_THREAD(worker(struct pt *pt))
{
    static bool flag;   // ✅ ウェイトをまたいで使う → static 必須
    bool raw;           // ❌ そのままではウェイト後に不定値

    PT_BEGIN(pt);

    raw = readSensor();
    PT_SLEEP(pt, 20);         // ここで return PT_WAITING → raw は消える
    if (raw) { ... }          // ← 不定値を読むことになる

    flag = readSensor();
    PT_SLEEP(pt, 20);
    if (flag) { ... }         // ← 保持される

    PT_END(pt);
}
```

対処法（公式の推奨）:
1. **`static` を付ける** — 非再入ならこちら。公式サンプル `codelock_thread` も `static int keys;` としている。
2. 再入が必要なら、明示的な状態オブジェクトに退避して渡す。

### ウェイトの2種類 — `PT_WAIT_UNTIL` と `PT_SLEEP`

| | `PT_WAIT_UNTIL(pt, cond)` | `PT_SLEEP(pt, N)` |
| --- | --- | --- |
| 待つもの | **条件** `cond` が真になること | **時間** N ms |
| チェック | 毎スケジュール（`cond` を再評価） | 満了時に1回 |
| 用途 | **状態変化を待つ** | **固定時間待つ** |

どちらも `loop()` に制御を返すため**他スレッドを止めない**。「ブロック」といってもシステムは止まらない。

- **このライブラリに `PT_WAIT_MS` はない**（別実装の派生版にあるマクロ）。`PT_SLEEP` は `pt-sleep.h` にしかない。
- `PT_SLEEP(pt, N)` は内部が `millis() - t > N` 判定なので**厳密に N より大きく待つ**（`millis()` 分解能 1ms なら実効 N+1ms）。指定値ちょうどでは待たない。
- `PT_THREAD` の戻り値は `void` ではなく **`char`**（`PT_WAITING=0` / `PT_YIELDED=1` / `PT_EXITED=2` / `PT_ENDED=3`）。`PT_SCHEDULE(f)` は `(f) < PT_EXITED` で判定するので、**独自の値を `return` すると壊れる**。

**`PT_WAIT_UNTIL` の `cond` にも「ウェイト跨ぎ変数」の規約が効く。** `cond` は再開後に再評価されるため、`PT_BEGIN` の後に代入した自動変数を参照すると不定値になる。実測では、生の `digitalRead(pin) == LOW` なら 40ms で正しく退出するのに、自動変数経由だと **5ms で不正に抜ける**。`cond` に書いてよいのは **引数・ファイルスコープ変数・関数呼び出し**だけ（引数は毎回呼び出し直されるため入口で再代入され、安全）。

### `PT_END` 到達と `for(;;)` の要否

`PT_END` の中身は実質 `lc = NULL; return PT_ENDED;` なので、**到達すると次の呼び出しは `PT_BEGIN` から最初から再実行される**。`for(;;)` や `PT_EXIT` が無ければ、`PT_END` には一切到達しない。

- **`for(;;)` は「スレッド内のワンタイム初期化を1回だけ走らせる」ための構造**（繰り返し自体は `loop()` が担っている）
- **`for(;;)` を外す場合、スレッド内の初期化行は削除（または `setup()` へ移動）しないと壊れる**

実測（10ms のみのノイズ入力・本来無視すべき）:

| 構成 | LED ちらつき |
| --- | --- |
| `for` あり＋初期化をスレッド内に置く | 0 回（正解） |
| `for` なし＋初期化をスレッド内に置く | **2 回（誤り・デバウンス無効化）** |
| `for` なし＋初期化を置かない | 0 回（正解） |

初期化が不要なら `for` なしの線形構成が最も素直。ファイルスコープの `static bool stable;` はゼロ初期化＝「離し・LED消灯」が既に正解で、初期化行自体が不要になることが多い。

### 読み込みにデバウンスを内包するマクロを作る制約

`DEBOUNCED_READ` のように「読み込み1行でデバウンスまで済ませる」ものは、次の3制約を守る。

1. **マクロでしか作れない** — 関数にすると `PT_WAIT_UNTIL` の `return PT_WAITING` が**ヘルパー関数から抜ける**だけで、スレッドはサスペンドしない。プロトスレッドは呼ばれた関数から yield できない。
2. **マクロ1展開につきウェイトは1箇所まで** — `LC_SET` のラベルが `__LINE__` ベースで、マクロ内の複数ウェイトがすべて**呼び出し行番号**になるため `redefinition of label` エラー。異なる待ち時間が必要なら、三項演算子で待ち時間を実行時決定し **1箇所に統合**する。
3. **待ちが隠れる分、呼び出し側が `static` 違反を作りやすい** — マクロの直後でウェイトをまたぐ自動変数を読むと不定値になる。マクロ内部の状態はマクロ側で保持し、呼び出し側には触れさせない設計にする。

### 展開サイトとインスタンスの関係（複数ピン）

`static` やマクロで持つ一時状態は**「展開サイト＝コード位置」単位**で共有される。

| 構成 | 結果 |
| --- | --- |
| スレッド**関数**ごとにマクロを展開 | 各自が独立に動作 ✅ |
| 同じ関数を複数の `struct pt` で駆動 | 一時状態が衝突し **あるインスタンスが飢餓（0回実行）・結果が誤る** ❌ |

→ **複数ピンは「状態を構造体で渡す」方式にする。** `struct pt` と一時状態を1つの構造体に束ねてポインタで渡せば、**1つのスレッド関数・1つの展開サイトで複数インスタンスを処理でき**、再入可能になる（実測で3ピン全正解）。`src/main.cpp` はこの方式（`struct Button` / `struct Debounce`）。

### デバウンスは「待ち後に再読込して確定」方式

タイムスタンプ比較（`millis() - changedAt >= 20`）ではなく、**変化を検出 → ウェイト20ms → その時点でもう一度読み込み、同じ状態なら確定**、としている。

- シーケンスが直線的で必要な状態が `stable` だけに減る
- 検証結果: 押下時は確定1回のみ、10msノイズは棄却（LED ちらつき0回）

**押下側と離し側は「別々の遷移」を守っている。** デバウンスは「1回通過したら以後ずっと有効なフィルタ」ではなく「毎回の読み取りで行う確認処理」なので、押下が確定した瞬間から押下側のループは回らなくなり、**その先の入力は後段の読み取りが丸ごと受け止める**。押下が確定した後は素の条件待ちでよく、デバウンスは不要。

### リリースチャタは次サイクルの `DEBOUNCED_READ` で弾ける

離し検出を素の `PT_WAIT_UNTIL(&b->pt, digitalRead(b->pin) != LOW)` にしても、残ったバウンスは**次のサイクルの押下側 `DEBOUNCED_READ`** に渡る:

1. バウンス中の LOW をサンプル → `stable(false)` と異なるので 20ms 待つ
2. 再読込するが**チャタは既に終わっている** → HIGH
3. サンプル値と不一致 → **棄却** → `stable` は false のまま維持

成立条件は **「チャタ継続 < `DEBOUNCE_MS`（≒20ms）」**。実測の境界（80通り = 離し開始8種 × チャタ周期10種）:

| チャタ継続時間 | 離しもデバウンス | 離しは素の条件待ち |
| --- | --- | --- |
| 5〜30ms | 0/80 誤点滅 | **0/80 誤点滅** |
| 40ms | 0/80 | 16/80 |
| 60ms | 5/80 | 37/80 |

実タクトスイッチのチャタは概ね 1〜10ms なので余裕で安全圏。素の条件待ちの方がコードは単純で消灯も速い（130ms → 105ms）。

**ただし素の条件待ちにした場合は `b->stable = false;` が必須。** これが無いと `stable` が `true` のまま永久に変わり、`while (!b->stable)` が1回も回らず、**LED が2回転するたびに `PT_END` する無限ループ（MCUハング）**になる。**削除できない復帰行**としてコメント付きで書くこと。

### 既知の限界 — 押下中のノイズ

押下中に一度だけ HIGH が混入すると、素の条件待ちは**その場で「離した」と誤判定**する（デバウンス付きなら 2回転、素だと **4回転**で点滅）。リリースチャタとは性質が違う — 押下が本物に成立しているため、次サイクルのデバウンスは「まだ押されている」と正しく判定してしまい、事前の解除だけが問題になる。配線が長く EMI を拾う環境では考慮する価値がある。

## Status

### 完成した内容
- PlatformIOプロジェクトの初期化（platformio.ini）
- 内蔵LED（D2）の点滅スケッチ
- タクトスイッチ（D10）でLEDを点灯制御するスケッチ
- シリアル入力1文字の大小文字を反転して送り返す `PT_THREAD`（`src/main.cpp` の `serialThread`）

### リソース使用量（LED点滅スケッチ）
- RAM: 23.0%（472 bytes / 2048 bytes）
- Flash: 39.8%（6528 bytes / 16384 bytes）

### リソース使用量（LED＋ボタン＋シリアル大小文字反転スケッチ）
- RAM: 24.2%（496 bytes / 2048 bytes）
- Flash: 41.7%（6836 bytes / 16384 bytes）

### シリアル通信の知見
- WCH-LinkE の UART ブリッジ上限: 921600bps（CDC 経由、公式仕様）
- 921600bps で受信時刻測定すると揺れは ±0.2ms（115200bps では ±4ms）
- CH32V003 内蔵 HSI は約 +0.9% の個体差あり。millis() の経過と実時間に乖離が生じる
- 点滅間隔 250ms 設定時、MCU millis() 差分は 250ms で一定（921600bps）

#### openwch コアの HardwareSerial の癖（重要）
- **`Serial.available()` は未実装で常に `-1` を返す**（`cores/arduino/HardwareSerial.cpp:148`）。
  `available() > 0` で入力有無を判定すると**永遠に真にならない**ので使えない。
  → 「入力があるか確認してから読む」という定番の書き方はこのコアでは成立しない。
- **`Serial.read()` は非ブロッキング**。内部 `uart_getc()` が RXNE を確認し、
  データが無ければ `-1` を返す（`cores/arduino/ch32/uart.c:515`）。
  → 「読んでも `-1` なら待つ」で入力有無の確認を兼ねられる。
  例: `PT_WAIT_UNTIL(pt, (s->c = Serial.read()) >= 0);`
- つまり **`read()` は入力確認と読み込みを1回で兼ねる**。`available()` と `read()` を
  分けて呼む必要はなく、2回呼ぶと順序の取り違え（確認後に読まれる）も防げない。
- `Serial` = `Serial1`（USART1）、TX = PD5（D15）/ RX = PD6（D16）。
  `setup()` で `Serial.begin()` を呼ばないと動かない。
- **PD5 は TX と USB プルアップ制御の兼用ピン**。シリアルを使う間は rv003usb の USB 通信は使えない。

### 接続状況
- 内蔵LED: D2ピン
- タクトスイッチ: D10ピン ↔ GND（外部抵抗なし、内蔵プルアップ使用）
