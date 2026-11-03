---
created: 2026-10-25T18:22
updated: 2026-10-27T17:53
---
# ProtoThreads 実行ログ（要約版）

User のプロンプトと、そのプロンプトに対する Assistant の**最終コード**および**最終結果**だけを残した要約。
探索過程・ツール出力・中間試行は削除している。

対象プロジェクト: `/Users/take/proj/Protothreads`
環境: UIAPduino Pro Micro CH32V003 v1.4 / Arduino (openwch) / PlatformIO 6.2.0 / `gitlab-airbornemint/Protothreads`

---

## 1. platformioの環境を整えて

### 最終コード

（作成）`platformio.ini`

```ini
; PlatformIO Project Configuration File
;
; UIAPduino + Protothreads プロジェクト
;

[env]
platform = https://github.com/Community-PIO-CH32V/platform-ch32v.git
upload_protocol = wch-link
debug_tool = wch-link

[env:UIAPduino_Pro_Micro_CH32V003_v1dot4]
board = UIAPduino_Pro_Micro_CH32V003_v1dot4
framework = arduino
monitor_speed = 115200
lib_deps =
    gitlab-airbornemint/Protothreads
```

（作成）`src/main.cpp`

```cpp
#include <Arduino.h>
#include <pt.h>
#include <pt-sleep.h>

// 内蔵LED (D2 / PC0) を Protothreads で点滅させるサンプル
static struct pt ptBlink;
static const uint8_t LED_PIN = 2;  // D2 = 内蔵LED

static PT_THREAD(blinkThread(struct pt *pt))
{
    PT_BEGIN(pt);

    for (;;)
    {
        digitalWrite(LED_PIN, HIGH);
        PT_SLEEP(pt, 500);
        digitalWrite(LED_PIN, LOW);
        PT_SLEEP(pt, 500);
    }

    PT_END(pt);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    PT_INIT(&ptBlink);
}

void loop()
{
    blinkThread(&ptBlink);
}
```

### 結果

- PlatformIO Core 6.2.0 は Homebrew でインストール済み
- ライブラリ名 `dandelionion/Protothreads` では見つからない。正しいのは **`gitlab-airbornemint/Protothreads`**
- このライブラリは Contiki 由来の古典版。**`PT_WAIT_MS` は存在せず**、`pt-sleep.h` の `PT_SLEEP(pt, delay)` を使う
- `PT_SLEEP` は内部で `static` 変数を使うため、**同一スレッド関数を複数インスタンスで使う場合は使えない**
- ビルド **SUCCESS** — RAM 23.8% / Flash 37.9%

---

## 2. アップロードして

### 結果

アップロード成功。WCH-Link 経由でフラッシュ書き込み・検証（**Verified OK**）・リセットまで完了。

---

## 3. つぎに、タクトボタンを押している間、LED を点灯するように修正して、タクトボタンには抵抗は付いていません。

### 最終コード

（作成）`src/main.cpp`

```cpp
#include <Arduino.h>
#include <pt.h>
#include <pt-sleep.h>

//
// タクトボタン押下中だけ内蔵LEDを点灯させる
//
// 配線:
//   タクトボタン   D10 (PD0) --- GND   (外部抵抗なし)
//   内蔵LED        D2  (PC0)  (CUT3で分離可能なオンボードLED)
//
// 抵抗がないため内部プルアップを使い、押下時にLOW / 離すとHIGHになる。
//

static struct pt ptButton;

static const uint8_t LED_PIN    = 2;   // D2  = PC0 = 内蔵LED
static const uint8_t BUTTON_PIN = 10;  // D10 = PD0 = タクトボタン

static const unsigned long DEBOUNCE_MS = 20;  // チャタリング除去時間 [ms]
static const unsigned long POLL_MS     = 5;   // 入力ポーリング周期 [ms]

static PT_THREAD(buttonThread(struct pt *pt))
{
    static bool          lastRaw;    // 直前の生の入力
    static bool          stable;     // デバウンス確定後の状態
    static unsigned long changedAt;  // 生の入力が変化した時刻

    PT_BEGIN(pt);

    lastRaw   = digitalRead(BUTTON_PIN);
    stable    = lastRaw;
    changedAt = millis();

    for (;;)
    {
        bool raw = digitalRead(BUTTON_PIN);

        if (raw != lastRaw)
        {
            // 入力が変化したら計測をやり直す
            lastRaw   = raw;
            changedAt = millis();
        }
        else if (raw != stable && (millis() - changedAt) >= DEBOUNCE_MS)
        {
            // 変化がDEBOUNCE_MS以上続いたら状態を確定
            stable = raw;
        }

        // 押下中はLOWなのでLEDを点灯させる
        digitalWrite(LED_PIN, stable == LOW ? HIGH : LOW);

        PT_SLEEP(pt, POLL_MS);
    }

    PT_END(pt);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);
    PT_INIT(&ptButton);
}

void loop()
{
    buttonThread(&ptButton);
}
```

### 結果

- ボタン接続ピンは **D10 (PD0)**（ユーザー選択）。外部抵抗なし・`INPUT_PULLUP`（CH32V003 Arduino コアは対応）
- デバウンスは 20ms 定着検出、5ms 周期ポーリング
- ビルド **SUCCESS** — RAM 23.8% / Flash 39.7%

---

## 4. buttonThread のforループは無限ループになると思いますが、PT_ENDには到達しますか。

### 結果

**いいえ、到達しません。**

- `PT_END` の中身は実質 `lc = NULL; return PT_ENDED;`
- `for(;;)` 内の `PT_SLEEP` により `return PT_WAITING` が唯一の出口になり、`PT_END` には一切到達しない
- ただし `PT_END` は**必須**。①`PT_BEGIN` が開いた波括弧を閉じる ②`return PT_ENDED;` を提供 ③`PT_INIT(pt)` で状態をリセット — の3役割がある
- `PT_INIT(pt)` が走らないので、**このスレッドは一度開始すると終了も再起動もしない**（永久稼働前提の設計）

---

## 5. void loop() で buttonThread が呼ばれているので、buttonThread 内の for ループは不要だと思いますが、違いますか。

### 結果

おおむね正しい。ただし **`for(;;)` は「スレッド内のワンタイム初期化を1回だけ走らせる」ための構造**（繰り返し自体は `loop()` が担っている）。

マクロ展開の実測:

| 構成 | 初期化の実行回数 | `PT_END` 到達回数 |
|---|---|---|
| `for` あり | 1回 | 0回 |
| `for` なし | 5回 | 4回 |

補足（移植性）: `bool raw = ...` を `PT_BEGIN` より後ろに置く構成は **GCC では通るが clang ではエラー**（indirect goto が変数の初期化をまたぐため）。実際 `riscv-wch-elf-g++` 12.2.0 は素通し、ホストの clang / g++ は両方拒否した。

---

## 6. デバウンスの処理に問題があります。明にPT_SLEEPで20ms待ち、ボタンの状態を再度確認するようにしてください。PT_THREADの中のウェイトにはPT_SLEEPを使うようにしてください。

### 最終コード

（作成）`src/main.cpp`

```cpp
#include <Arduino.h>
#include <pt.h>
#include <pt-sleep.h>

//
// タクトボタン押下中だけ内蔵LEDを点灯させる
//
// 配線:
//   タクトボタン  D10 (PD0) --- GND  (外部抵抗なし → 内部プルアップを使用)
//   内蔵LED       D2  (PC0)
//
// デバウンス方針:
//   状態の変化を検出したら PT_SLEEP で20ms待ち、
//   その後にボタンを再読み込みして、同じ状態が維持されていれば確定とする。
//   ウェイトはすべて PT_SLEEP を使う。
//

static struct pt ptButton;

static const uint8_t LED_PIN    = 2;   // D2  = PC0 = 内蔵LED
static const uint8_t BUTTON_PIN = 10;  // D10 = PD0 = タクトボタン

static const unsigned long POLL_MS     = 5;   // 変化なし時のポーリング周期 [ms]
static const unsigned long DEBOUNCE_MS = 20;  // 確認待ち時間 [ms]

static bool stable;   // 確定した押下状態 (true = 押下中)

static PT_THREAD(buttonThread(struct pt *pt))
{
    bool raw;   // PT_BEGIN より前に宣言 (indirect goto が初期化をまたがないようにする)

    PT_BEGIN(pt);

    // 起動時点の状態を基準にする (for の中なので1回だけ実行される)
    stable = (digitalRead(BUTTON_PIN) == LOW);
    digitalWrite(LED_PIN, stable ? HIGH : LOW);

    for (;;)
    {
        raw = (digitalRead(BUTTON_PIN) == LOW);

        if (raw == stable)
        {
            // 変化なし → 短く待って次のポーリングへ
            PT_SLEEP(pt, POLL_MS);
            continue;
        }

        // 変化を検出した → 20ms 待ってからボタン状態を再確認する
        PT_SLEEP(pt, DEBOUNCE_MS);

        if ((digitalRead(BUTTON_PIN) == LOW) == raw)
        {
            // 20ms 後も同じ状態が続いていればボタン操作として確定
            stable = raw;
            digitalWrite(LED_PIN, stable ? HIGH : LOW);
        }
        // 逆に戻っていればチャタリング(ノイズ)として無視する
    }

    PT_END(pt);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);
    PT_INIT(&ptButton);
}

void loop()
{
    buttonThread(&ptButton);
}
```

### 結果

タイムスタンプ比較をやめ、**変化検出 → PT_SLEEP で20ms待ち → 再読込して確定**方式に変更。

- 40ms から押下（40〜50ms チャタリング）→ 変化検出@40ms → **確定@65ms**（1回のみ、LED点灯）
- 40〜48ms の 10ms ノイズのみ → 変化検出@40ms → **棄却@65ms**（確定0回、状態変化なし）
- ビルド **SUCCESS** — RAM 23.8% / Flash 39.6%

---

## 7. これでforループを使わないよう動くか検証して

### 最終コード

（編集）`src/main.cpp` — `raw` を `static` 化

```cpp
static bool stable;   // 確定した押下状態 (true = 押下中)

static PT_THREAD(buttonThread(struct pt *pt))
{
    // PT_SLEEP のウェイトをまたいで値を保持するため、static が必須。
    // auto 変数だと return PT_WAITING でスタックが消え、再開時に不定値になる。
    static bool raw;   // PT_BEGIN より前に宣言 (indirect goto が初期化をまたがないようにする)
```

### 結果

**検証中に重大なバグを発見した。**

3構成の比較（`raw` を `static` 化した後の正しい結果）:

| 構成 | 10ms ノイズ時の LED ちらつき | 正解 |
|---|---|---|
| A: `for` あり（現行） | 0回 | 0回 ✅ |
| B: `for` を素直に外す | **2回** | 0回 ❌ |
| C: `for` なし＋初期化行も外す | 0回 | 0回 ✅ |

**発見したバグ**: `bool raw` は `PT_SLEEP` のウェイトをまたいで使われているが、`return PT_WAITING` で関数を抜けるため、**復帰時には未初期化のスタック領域を読む**ことになる。

| 実行条件 | `raw` の値 | 結果 |
|---|---|---|
| A 単体実行 | 1（偶然保持） | 正しく棄却 |
| A + 他関数がスタックを汚す | **0（化けた）** | **誤確定** |

→ **`static bool raw;` に修正**。GCC・clang 両方でコンパイル確認済み。修正版をアップロードした。

これは**「たまたま動いていただけ」**であり、`millis()` 割り込みやコアの処理がスタックを踏み戻るタイミング次第で誤動作する。

---

## 8. ProtoThreads のページに PT_THREAD 内の変数について注意点は書いてありませんか。

### 結果

**はい、公式に明記されています。**

> Because protothreads do not save the stack context across a blocking call, **local variables are not preserved when the protothread blocks.** This means that local variables should be used with utmost care — if in doubt, **do not use local variables inside a protothread!**
> — `pt-1.4-refman.pdf` §1.4 "Local variables"

対処法（公式の推奨）:

1. **`static` を付ける** — 非再入ならこちら。公式サンプル `codelock_thread` も `static int keys;`
2. 再入が必要なら**明示的な状態オブジェクト**に退避して渡す

**論文は「GCC が警告する」と書くが、実際にはこの環境では一度も出ない。**

| 条件 | uninitialized 警告 |
|---|---|
| `-O0` / `-O1` / `-O2` / `-O3` / `-Os` × `lc-addrlabels` / `lc-switch` | 全組合せ **0件** |
| 実機ビルドそのまま（`-Os -flto -Wall`） | **0件** |

論文（`dunkels05protothreads.pdf` §1.3.4）の記載は 2005 年当時の GCC の挙動で、riscv-wch-elf-g++ 12.2.0 では再現しない。**コンパイラ頼みはできず、規約を自覚的に守る必要がある。**

---

## 9. この注意点を AGENTS.md にメモして

### 最終コード

（編集）`AGENTS.md` — `## Status` の直前に追加

````markdown
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
````

### 結果

`AGENTS.md` に `## Protothreads の注意点` セクションを追記完了。

---

## 10. PoroThreads のスレッドは戻り値がvoidに限定されていますか。

### 結果

**いいえ、`void` ではなく `char` です。**

```c
#define PT_THREAD(name_args) char name_args    /* pt.h */

#define PT_WAITING 0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3

#define PT_SCHEDULE(f) ((f) < PT_EXITED)   /* 2未満 = 実行中 */
```

- 戻り値は**アプリのデータではなくスケジューラ状態**
- `PT_SCHEDULE(f)` は数値で判定しているため、**独自の値を `return` すると壊れる**
  - `return 42;` → コンパイルは通るが `42 < 2` が偽なので「まだ実行中」が「終了した」と誤判定される
- アプリのデータを外に出すなら、引数のポインタかファイルスコープ変数を使う

---

## 11. デバウンス処理を別のスレッドにするのは良くないですか。

### 結果

**「悪い」ではありませんが、この用途では得がなくコストだけ残ります。** `PT_SPAWN` / `PT_WAIT_THREAD` による階層化は公式機能です。

| | RAM | Flash | 追加警告 | 挙動 |
|---|---|---|---|---|
| インライン（現行） | 488 (23.8%) | 6484 (39.6%) | なし | ✅ |
| デバウンスを別スレッド化 | 496 (24.2%) | 6552 (40.0%) | `-Wdangling-pointer` ×1 | ✅ 同一 |

**最大の理由: 並行性は1つも得られません。** 現行版の `PT_SLEEP` はすでに `loop()` に制御を返している。プロトスレッドは協調的で単一スタックなので、「デバウンス待っている間に他が動く」なら現行版ですでに達成済み。逆に分割版は `PT_WAIT_THREAD` が親をブロックするため、親の自由度はむしろ下がる。

追加コスト:

- 状態変数が倍になる（`candidate` / `confirmed` 追加、どちらもウェイト跨ぎなので `static` 必須 → 適用範囲が広がる）
- `struct pt` がもう1つ（+4 bytes）＋子の `PT_SLEEP` 用 static（+4 bytes）
- `PT_INIT` の管理（`PT_WAIT_THREAD` は手動）
- 「検出 → 20ms待つ → 再確認」という直列の依存関係が2関数に分かれ、可読性が下がる

---

## 12. IOの状態読み込み(digitalReadの箇所) にデバウンスを内包ものが作れないですか。

### 最終コード

（作成）`src/main.cpp` — マクロ内包版

```cpp
#include <Arduino.h>
#include <pt.h>

// デバウンスを「読み込み」に内包したマクロ
//  - プロトスレッドは呼ばれた関数から yield できないため、マクロでなければならない
//  - ラベルが __LINE__ ベースなので 1展開につきウェイトは1箇所まで
//  - ウェイトをまたぐ値は pDb 経由で保持。static も自動変数も使わない
#define DEBOUNCED_READ(pt, pin, pStable, pDb) do {                  \
    (pDb)->raw = (digitalRead(pin) == LOW);                         \
    (pDb)->t   = millis();                                          \
    PT_WAIT_UNTIL(pt, (millis() - (pDb)->t)                         \
                      > (((pDb)->raw != *(pStable)) ? DEBOUNCE_MS   \
                                                    : POLL_MS));    \
    if ((pDb)->raw != *(pStable)                                    \
        && (digitalRead(pin) == LOW) == (pDb)->raw)                 \
        *(pStable) = (pDb)->raw;                                    \
} while (0)

static struct pt ptButton;

static const uint8_t LED_PIN    = 2;
static const uint8_t BUTTON_PIN = 10;

static const unsigned long POLL_MS     = 5;
static const unsigned long DEBOUNCE_MS = 20;

static bool stable;   // デバウンス済みの押下状態

static PT_THREAD(buttonThread(struct pt *pt))
{
    PT_BEGIN(pt);

    stable = (digitalRead(BUTTON_PIN) == LOW);
    digitalWrite(LED_PIN, stable ? HIGH : LOW);

    for (;;)
    {
        DEBOUNCED_READ(pt, BUTTON_PIN, &stable);   // 読み込み1行でデバウンス込み
        digitalWrite(LED_PIN, stable ? HIGH : LOW);
    }

    PT_END(pt);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    digitalWrite(LED_PIN, LOW);
    PT_INIT(&ptButton);
}

void loop()
{
    buttonThread(&ptButton);
}
```

### 結果

**作れるが、3つの制約がある（いずれも実測で確認）。**

1. **マクロでしか作れない** — 関数にすると `PT_WAIT_UNTIL` の `return PT_WAITING` が**ヘルパー関数から抜ける**だけで、スレッドはサスペンドしない
2. **マクロ1展開につきウェイトは1箇所まで** — `LC_SET` のラベルが `__LINE__` ベースで、マクロ内の複数ウェイトがすべて呼び出し行番号になる → `error: redefinition of label 'LC_LABEL38'`。異なる待ち時間は**三項演算子で実行時決定し1箇所に統合**して回避
3. **待ちが隠れる分、呼び出し側が `static` 違反を作りやすい**

挙動検証（従来版と一致）:

| ケース | 確定回数 | LED ちらつき |
|---|---|---|
| 40ms から押下（40〜50ms チャタリング） | 1回 | 1回 ✅ |
| 40〜48ms の 10ms ノイズのみ | 0回 | 0回 ✅ |

ビルド **SUCCESS** — RAM 488 (23.8%) / Flash **6472** (39.5%)（従来比 −12 bytes）

---

## 13. もう一つ確認です。DEBOUNCED_READ マクロ内の _raw, _t は、do whileの内部のみで参照され、外部からは分離されているので合っていますか。

### 結果

**名前の分離は正しく、意図どおりです。ただし「分離」と呼べるのは名前（scope）だけで、ストレージは do-while の中にはありません。**

検証1: マクロ外から `_raw` を参照 → `error: use of undeclared identifier '_raw'`（分離OK）
検証2: 呼び出し側に同名 `_raw` を置く → エラー0件（マクロ側が遮蔽、衝突しない）

実機バイナリのシンボル（決定的）:

```
200000cc 00000001 b _ZZL12buttonThreadP2ptE4_raw   → buttonThread(pt*)::_raw
200000c8 00000004 b _ZZL12buttonThreadP2ptE2_t     → buttonThread(pt*)::_t
```

- mangle の `ZZ...E` = **関数内ブロックスコープの static**（＝①の分離）
- セクション `b` = **`.bss`**（＝スタックではなく、`return PT_WAITING` で消えない）

**もし do-while 内の自動変数（スタック）だったら、ウェイトをまたいだ瞬間に消えて不定値になる。** 分離しつつスタックに置かない、という組み合わせこそが正しい状態。

**落とし穴: 「1つの展開サイト = 1つの変数セット」。** 同じ呼び出し位置を複数の `struct pt` で使うと `_raw` / `_t` を共有する:

| 構成 | ループ完了回数 A / B | pin11 押下後の `stable11` |
|---|---|---|
| 共有サイト（1関数 → 2インスタンス） | 28回 / **0回** | **0（誤り）** |
| 別サイト（関数を分けた） | 28回 / 28回 | 1（正解） |

共有サイトでは **B が完全に飢餓**された。A がループのたびに `_t = millis()` を書き換え、B は `millis() - _t` が常に0で待機が永遠に解けないため。

現行コードは `ptButton` の1インスタンス × 展開サイト1箇所なので問題なし。

---

## 14. ピンが増えれば、スレッドは当然別にします。他のスレッドでDEBOUNCED_READが使えることが大切です。このようにすれば、buttonThread内のforループは不要になると思いますが、違いますか。

### 最終コード

（作成）`src/main.cpp` — 状態構造体方式・`for` なし

```cpp
#include <Arduino.h>
#include <pt.h>

static const uint8_t       LED_PIN     = 2;   // D2  = PC0 = 内蔵LED
static const uint8_t       BUTTON_PIN  = 10;  // D10 = PD0 = タクトボタン
static const unsigned long POLL_MS     = 5;   // 変化なし時のポーリング周期 [ms]
static const unsigned long DEBOUNCE_MS = 20;  // 変化時の確認待ち [ms]

// デバウンスで使う一時状態。ウェイトをまたぐため自動変数(スタック)に置けない。
// インスタンスごとに用意することで、複数ピンでも干渉しない。
struct Debounce {
    bool          raw;   // サンプルした生の状態 (true = 押下)
    unsigned long t;     // サンプル時刻
};

// ボタン1個分の状態。ピンが増えたらこの構造体の配列にする。
struct Button {
    struct pt       pt;
    uint8_t         pin;
    bool            stable;   // 確定した押下状態 (初期値 false = 離し)
    struct Debounce db;
};

// デバウンスを「読み込み」に内包したマクロ
//  - プロトスレッドは呼ばれた関数から yield できないためマクロでなければならない
//  - ラベルが __LINE__ ベースなので 1展開につきウェイトは1箇所まで
//  - ウェイトをまたぐ値は pDb 経由で保持。static も自動変数も使わない
#define DEBOUNCED_READ(pt, pin, pStable, pDb) do {                  \
    (pDb)->raw = (digitalRead(pin) == LOW);                         \
    (pDb)->t   = millis();                                          \
    PT_WAIT_UNTIL(pt, (millis() - (pDb)->t)                         \
                      > (((pDb)->raw != *(pStable)) ? DEBOUNCE_MS   \
                                                    : POLL_MS));    \
    if ((pDb)->raw != *(pStable)                                    \
        && (digitalRead(pin) == LOW) == (pDb)->raw)                 \
        *(pStable) = (pDb)->raw;                                    \
} while (0)

static struct Button button = { {}, BUTTON_PIN, false, {} };

// 繰り返しは loop() が担うので for は不要 (1回の呼び出し = 1回の読み取り)。
// ワンタイム初期化も不要。stable はファイルスコープでゼロ初期化 = 離し状態。
static PT_THREAD(buttonThread(struct Button *b))
{
    PT_BEGIN(&b->pt);

    DEBOUNCED_READ(&b->pt, b->pin, &b->stable, &b->db);
    digitalWrite(LED_PIN, b->stable ? HIGH : LOW);

    PT_END(&b->pt);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(button.pin, INPUT_PULLUP);
    digitalWrite(LED_PIN, LOW);
    PT_INIT(&button.pt);
}

void loop()
{
    buttonThread(&button);
}
```

### 結果

**おっしゃる通り、`for` ループは不要になります。** ただし条件がある。

`for` の要否を3構成で実測:

| 構成 | 10ms ノイズ時の LED ちらつき | 正解 |
|---|---|---|
| `for` あり＋初期化をスレッド内に置く | 0回 | 0回 ✅ |
| `for` なし＋初期化をスレッド内に置く | **2回** | 0回 ❌ |
| `for` なし＋初期化を置かない | 0回 | 0回 ✅ |

→ **初期化行が不要なら `for` も不要。** ファイルスコープの `static bool stable;` はゼロ初期化＝「離し・LED消灯」が既に正解なので、初期化行自体が不要になることが多い。

**ピン複数化について**: 1つのマクロ展開サイトを複数の `struct pt` で駆動すると一時状態が衝突して飢餓する（前項参照）が、**状態を構造体で渡せば1つの関数・1つの展開サイトで複数ピンを処理できる**。3ピンの実測:

```
pin10 (40ms押下) → stable=1 確定= 65ms  正解=1
pin11 (80ms押下) → stable=1 確定=105ms  正解=1
pin12 (押さない) → stable=0              正解=0
```

---

## 15. デバウンスの検証で得た知見もAGENTS.mdに追記して

### 最終コード

（編集）`AGENTS.md` — `## Protothreads の注意点` に4サブセクションを追加

````markdown
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

なお `PT_SLEEP(pt, N)` / `PT_WAIT_UNTIL(pt, elapsed > N)` は**厳密に N より大きい**待機になる（`millis()` の分解能 1ms なら実効 N+1ms）。指定値ちょうどでは待たない点に注意。
````

### 結果

`AGENTS.md` の `## Protothreads の注意点` が計8セクションに拡張された。

---

## 16. buttonThread でPT_WAIT_UNTIL を使ってボタンを押されている間、待つような書き方はできませんか。

### 最終コード

（作成）`src/main.cpp` — `while` による3段階構成

```cpp
static PT_THREAD(buttonThread(struct Button *b))
{
    PT_BEGIN(&b->pt);

    // 1. 押下が確定するまで待つ (離しの状態なら回り続けるだけ)
    while (!b->stable)
        DEBOUNCED_READ(&b->pt, b->pin, &b->stable, &b->db);
    digitalWrite(LED_PIN, HIGH);

    // 2. 押されている間はここで待つ (離しが確定するまで)
    while (b->stable)
        DEBOUNCED_READ(&b->pt, b->pin, &b->stable, &b->db);
    digitalWrite(LED_PIN, LOW);

    // 3. 「1回押して離した」がここで成立する (アクションを置く場所)

    PT_END(&b->pt);   // 再起動して次の押下待ちへ
}
```

### 結果

**使えます。** ただし `PT_SLEEP` とは役割が違う。

- `while` の条件は `b->stable`（構造体）だけを参照するため、**ウェイトをまたぐ自動変数は一切ない**
- 検証: 押下40〜100ms（チャタ付き）→ LED反転2回（点灯70ms / 消灯130ms）、押下完了1回 ✅
- 検証: 10ms ノイズのみ → LED反転**0回**、押下完了0回 ✅

対照実験（デバウンスなし）:

| 方式 | 結果 |
|---|---|
| `PT_WAIT_UNTIL` + 生 `digitalRead` | **click = 2回（誤検出）** — チャタリングで離しを誤判定 |
| `PT_WAIT_UNTIL` の `cond` に自動変数 | **5ms で不正に退出**（正解は40ms） |
| `DEBOUNCED_READ` を `while` で包む | click = 1回（正解） ✅ |

---

## 17. 押されている間待つ時の DEBOUNCED_READ は既にデバウンスを回避しているので、通常のdigitalReadでよいと思います。

### 最終コード

なし（変更せず）

### 結果

**そのまま置き換えると2つの問題が実測で出ます。**

**問題1: `b->stable` が更新されないので無限ループ（MCUハング）**

```
V2 離し待ち=生読み | ケース1(押下40〜100ms)
   → *** ハング(無限ループ) ***   点灯=65  消灯=0  最終LED=1
```

`b->stable = false;` を手で入れれば回るが、**状態機械の不変条件を手動管理する**必要が生じる。

**問題2: 押下中にノイズが入ると誤解除する**

押下中に一度だけ HIGH のノイズ（70〜75ms）が混入するケース:

| | LED 反転 | 経過 |
|---|---|---|
| V1（現行・両段 DEBOUNCED_READ） | **2回** ✅ | 点灯65 → 消灯130（ノイズ無視） |
| V3（離し待ちを生読み＋stable手動リセット） | **4回** ❌ | 点灯65 → 消灯70 → 点灯100 → 消灯105 |

**なぜ「デバウンス済み」が効かないのか**: デバウンスは「1回通過したら以後ずっと有効なフィルタ」ではなく「毎回の読み取りで行う確認処理」。phase 1 のループは押下確定で回らなくなり、押下中の入力は phase 2 の読み取りが丸ごと受け止める。

---

## 18. デバウンス処理は、チャタリング防止のためタクトスイッチ押下時のみ対処すれば良いと思うのですが、押下完了後にもデバウンスを使う理由がなっとくできません。

### 結果

**リリースチャタだけなら、ご認識の通りでした。**

| ケース | V1（両段デバウンス） | V3（離し待ち＝生読み） |
|---|---|---|
| 標準リリースチャタ（100〜110ms） | 反転 **2回** ✅ | 反転 **2回** ✅ |
| ゆっくり離す・長いチャタ（100〜160ms） | 反転 **2回** ✅ | 反転 **2回** ✅ |
| **押下中に70〜75msのノイズ** | 反転 **2回** ✅ | 反転 **4回** ❌ |

**残ったチャタは次サイクルの phase 1 が吸収する**というご指摘のメカニズムが実際に効いている。生読みの方が消灯も速い（V1: 130/185ms → V3: 105ms）。

ただし注意点として、V3 は**最初のバウンスで「離した」判定**しており、まだ接点が不安定な段階で LED が消える（V1=185ms に対し V3=105ms）。LED なら実害は小さいが、クリック計測や長押し時間計測では差が出る。

> この結論は離し開始 100ms・周期 5ms の**1構成だけを見た一般化不足**だった → 次のプロンプトで sweep して確認する。

---

## 19. リリース時のチャタリングは、次のbuttonTreadのDEBOUNCED_READで弾くと思いますが、違いますか。

### 結果

**おっしゃる通りです。** チャタ条件を sweep して境界まで測定した。

誤点滅数（80通り中。80通り = 離し開始8種 × チャタ周期10種。正解は2回 = 点灯1回 + 消灯1回）:

| チャタ継続時間 | V1（両段デバウンス） | V3（離し待ち＝生読み） |
|---|---|---|
| 5 / 10 / 15 / 20 / 25 / 30ms | 0/80 | **0/80** ← 安全 |
| 40ms | 0/80 | 16/80 |
| 60ms | 5/80 | 37/80 |

**現実的な範囲（〜30ms）では生読みで 0/80 通り、完全に安全。** 実タクトスイッチのチャタは概ね 1〜10ms。

弾けるメカニズム:

1. バウンス中の LOW をサンプル → `stable(false)` と異なるので 20ms 待つ
2. 再読込するが**チャタは既に終わっている** → HIGH
3. サンプル値と不一致 → **棄却** → `stable` は false のまま維持

成立条件は **「チャタ継続 < `DEBOUNCE_MS`（≒20ms）」**。

残る2点:

1. **`b->stable = false;` の手動リセットが必須** — 無ければ `while (!b->stable)` が無限ループで MCU ハング
2. **押下中のノイズは別問題** — リリースチャタとは性質が異なる（前項参照）

---

## 20. はい

### 最終コード

（編集）`src/main.cpp` — 離し待ちを `PT_WAIT_UNTIL` に変更

```cpp
static PT_THREAD(buttonThread(struct Button *b))
{
    PT_BEGIN(&b->pt);

    // 1. 押下が確定するまで待つ
    //    デバウンスはここで十分。離しの残チャタも次サイクルのこの DEBOUNCED_READ が
    //    棄却する (成立条件: ﾁｬﾀ継続 < DEBOUNCE_MS ≒ 20ms。実タクトは1〜10ms)。
    while (!b->stable)
        DEBOUNCED_READ(&b->pt, b->pin, &b->stable, &b->db);
    digitalWrite(LED_PIN, HIGH);

    // 2. 押されている間はここで待つ (離しを検出するまで)
    //    押下は確定下は確定済みなので単純な条件待ちでよく、デバウンスは不要。
    PT_WAIT_UNTIL(&b->pt, digitalRead(b->pin) != LOW);

    // ★必須: stable を復帰させないと、次回の while(!b->stable) が
    //   一切回らず、LED の点滅と PT_END 到達の無限ループになる。
    b->stable = false;
    digitalWrite(LED_PIN, LOW);

    // 3. 「1回押して離した」がここで成立する (アクションを置く場所)

    PT_END(&b->pt);   // 再起動して次の押下待ちへ
}
```

### 結果

| ケース | LED 反転 | 押下完了 | 結果 |
|---|---|---|---|
| 押下40〜100ms（チャタ付き） | 2回（点灯65ms / 消灯105ms） | 1回 | ✅ |
| 10ms ノイズのみ | 0回 | 0回 | ✅ |
| 押下中70〜75ms ノイズ | 4回 | 2回 | ⚠️ 既知の限界どおり |

- ビルド **SUCCESS** — RAM 488 (23.8%) / Flash **6708** (40.9%)
- 消灯が 130ms → **105ms** に速くなり、Flash も 76 bytes 減
- **注意すべき不変条件**: `b->stable = false;` は**削除できない必須行**

---

## 21. Flashは、コードの意図を読みやすくするには十分な対価だと思います。気にしません。

### 結果

ご指摘の通り余裕がある。

| 構成 | Flash | 使用率 |
|---|---|---|
| マクロ内包版（`for` あり） | 6472 | 39.5% |
| 現在（離し＝条件待ち） | **6708** | **40.9%** |
| （空き） | 9676 bytes | 59.1% |

今回の一回ではむしろ **76バイト減**（`while` の入れ子が消えた分）。累積では `for` 削除と構造体受け渡しの分 +236 bytes だが、16KB 中 9.6KB 空いており問題ない水準。

---

## 22. これまでの処理を ProtoThreadsの紹介記事としてまとめたいです。Introduction_ProtoThread.md に整理して

### 最終コード

（作成）`Introduction_ProtoThread.md` — 453行

構成:

| 章 | 内容 |
|---|---|
| 1. Protothreads とは | スタックを持たないコルーチン、メモリ2〜4バイト、協調的スケジューリング |
| 2. 基本構文 | `PT_THREAD` の戻り値は `void` ではなく **`char`**（4状態定数・`PT_SCHEDULE` が数値判定）、`PT_BEGIN`/`PT_END`、`PT_WAIT_UNTIL` と `PT_SLEEP` の使い分け |
| 3. 最重要ルール | ウェイト跨ぎ変数は `static`。公式引用 + **GCC が警告しない実測（全最適化 × 全LC実装 = 0件）** + 「たまたま動く」危険性 + `cond` 内にも効く |
| 4. `PT_END` と `for(;;)` | 再起動セマンティクス、`for` は「ワンタイム初期化用」、3構成の実測表 |
| 5. デバウンス設計 | 待ち後に再読込して確定／押下側と離し側は別遷移を守る／リリースチャタは次サイクルで弾ける（境界スイープ表 5〜60ms）／押下中ノイズの既知限界 |
| 6. 内包マクロの制約 | マクロでしか作れない／1展開につきウェイト1箇所／待ちが隠れる落とし穴 |
| 7. 展開サイトとインスタンス | 共有すると飢餓（0回実行）、状態構造体で再入可能に |
| 8. 完成コード | コンパイル検証済みの全コード |
| 9. 検証結果まとめ | 挙動表・リソース推移（472→488 RAM / 6528→6708 Flash） |
| 10. チェックリスト | 新しい `PT_THREAD` を書くときの7項目 |

### 結果

- 記事内の完成コードは**実機ツールチェインでコンパイル確認済み**（既知の `PT_YIELD_FLAG` 警告のみ）
- 本体 `src/main.cpp` との**ロジック差分は0**（コメント表記 `/* */` と `//` の違いのみ）
- 全ての数値を**推測ではなく実測値**で記載。どこまでが実証されてどこからが限界かを明示（例: 離し側素待ちは〜30ms 安全、40ms で崩れる）

---

## 23. 重複しても、追記してください。

### 最終コード

（編集）`AGENTS.md` — 3セクションを追加し `## Protothreads の注意点` を計8セクションに

````markdown
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
````

### 結果

`AGENTS.md` 全体で 191行。ハング要因やスイープ境界が `AGENTS.md` と `Introduction_ProtoThread.md` の両方に残る状態になった。

---

## 24. main.cpp のloop関数のbuttohThreadの箇所に、/* ここに複数のスレッドを追記できる */ 旨のコメントを入れると、今回の実験の意図が伝わると思うのですが、どうでしょう？

### 最終コード

（編集）`src/main.cpp`

```cpp
void loop()
{
    /* ここに複数のスレッドを追記できる（協調スレッドなので相互にブロックしない）。
       複数ボタンは struct Button の配列にして
       for (auto &b : buttons) buttonThread(&b); と書く
       （状態構造体方式なので 1関数で複数インスタンスを処理できる）。 */
    buttonThread(&button);
}
```

### 結果

良い提案として採用。場所が正しい（「どこに何を足すか」が最も知りたい箇所）、設計の要点（状態構造体方式にした最大の理由）が伝わる、協調スレッドであることも明示できる、の3点から賛成。

- `src/main.cpp` に追記し、`Introduction_ProtoThread.md` の完成コードも同期
- ビルド **SUCCESS** — RAM 488 (23.8%) / Flash **6708** (40.9%)
- Flash が変更前と同量なのでコメントはコード生成に影響せずバイナリは同一 → **アップロード不要**
