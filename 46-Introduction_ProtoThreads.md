---
created: 2026-10-25T16:50
updated: 2026-10-30T20:44
---
# 46-ProtoThreadsの紹介
45-VibeCodingで紹介したボードにProtoThreadsを使って疑似的なスレッド処理をOpenCodeとのVibe Codingで実装しました。

WCH-LinkEのシリアル通信の入力用に１本結線を増やしていいます。
<img src="Files/images/46/WCH-LinkE-Setting.jpg" width="400">

最初に「タクトボタンを押している間のみLEDを点灯するコードを作って」と言って出力されたコードが以下のコードです。
```C++
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

最終的にOpenCodeでAIと一緒に作成したコードは、以下にあります。
- <a href="https://github.com/take-pwave/letsArduino/blob/master/code/46/src/main.cpp">main.cpp</a>

これを見て頂くと、最初にAIが提示してきたコードよりも構造がすっきりして、処理の流れを理解しやすいコードになっていることを分かってもらえると思います。

AIは、コードの意味を理解しているのではなく、**多くの例から漠然とこのようにコーディングするのだと解釈している**のだということを分かって使えば、こんなにも分かりやすいコードを作ってくれることをお伝えしたいです。

私としては、以下の点に注意してコードを見て欲しいです。
- buttonThreadのチャタリング防止のデバウンス処理の後、タクトスイッチを離すまで待つ、PT_WAIT_UNTILでスレッドが切り替わっているのに、コード上はawait処理で待っているように見えている点です。
- serialThreadで入力がない場合には、すぐにスレッドを切り替えているPT_WAIT_UNTILの部分です。
- このようにProtoThreadsライブラリの提供するマクロの場所で明示的にスレッドが切り替わっていることもこのライブラリの特徴と言えます。

以下は、OpenCodeが最後にまとめた記事です。

## Protothreads 入門 — 基本と、実際に遭遇した落とし穴

組み込み・イベント駆動向けの軽量コルーチンライブラリ **Protothreads**（Adam Dunkels 作、Contiki 由来）を、UIAPduino（CH32V003）上で実際にデバウンス付きボタン入力を書くまでに突き詰めた過程をまとめた。後半ではボタンとシリアル入力の**2スレッド並行**に踏み込み、ブロックしそうな API の扱いまでを扱う。

動作確認環境:

| 項目 | 値 |
| --- | --- |
| MCU / ボード | CH32V003F4P6 / UIAPduino Pro Micro CH32V003 v1.4 |
| フレームワーク | Arduino（openwch）+ PlatformIO 6.2.0 |
| ライブラリ | `gitlab-airbornemint/Protothreads` 1.4.0-arduino.beta.1 |
| コンパイラ | riscv-wch-elf-g++ 12.2.0（`-Os -flto -Wall`） |

---

## 1. Protothreads とは

**スタックを持たないコルーチン**だ。1つの関数を「ブロック箇所でいったん関数を抜け、次回の呼び出しでその場から再開する」という動きを、C のプリプロマクロだけで実現する。

- メモリはスレッド1つにつき **2〜4バイト**（`struct pt` のみ）
- 割り込みや RTOS が要らない、**協調的（cooperative）**な多重化
- スタックを保存しないため、ウェイト中に**ローカル変数が消える**（後述の最重要ルール）

RTOS や本物のスレッドが過剰で、コールバック地獄は避けたい、という中間の用途に合う。

---

## 2. 基本構文

### 2.1 `PT_THREAD` の戻り値は `void` ではない（`char`）

```c
#define PT_THREAD(name_args) char name_args    /* pt.h */
```

`static PT_THREAD(foo(struct pt *pt))` は展開すると **`static char foo(struct pt *pt)`** になる。`void PT_THREAD(...)` と書くと `void char ...` になって構文エラーになる。

戻り値は**アプリのデータではなくスケジューラ状態**で、4つしかない。

| 定数 | 値 | 意味 |
| --- | --- | --- |
| `PT_WAITING` | 0 | ウェイトでブロック中 |
| `PT_YIELDED` | 1 | `PT_YIELD` で譲った |
| `PT_EXITED` | 2 | `PT_EXIT` で終了 |
| `PT_ENDED` | 3 | `PT_END` に到達 |

判定側は数値で見ているので、**独自の値を `return` すると壊れる**。

```c
#define PT_SCHEDULE(f) ((f) < PT_EXITED)   /* 2未満 = 実行中 */
```

`return 42;` を書けばコンパイルは通るが、`42 < 2` が偽なので「まだ実行中」が「終了した」と誤判定される。アプリのデータを外に出すなら、引数のポインタかファイルスコープ変数を使う。

### 2.2 `PT_BEGIN` / `PT_END`

```cpp
static PT_THREAD(worker(struct pt *pt))
{
    PT_BEGIN(pt);
    /* ... */
    PT_END(pt);
}
```

このライブラリは `lc.h` が **`lc-addrlabels.h`**（GCC の computed goto / `goto *s`）を include している。昔の `switch/case` 版（`lc-switch.h`）ではない点に注意 — `switch` 文の中にプロトスレッドのウェイトを書けない、という古い制約はこちらでは効かない。

`PT_END` の中身は実質次のとおり。

```c
lc = NULL; return PT_ENDED;
```

**つまり `PT_END` に到達すると、次の呼び出しは `PT_BEGIN` から最初から再実行される。**

### 2.3 ウェイトの2種類

| | `PT_WAIT_UNTIL(pt, cond)` | `PT_SLEEP(pt, N)` |
| --- | --- | --- |
| 待つもの | **条件** `cond` が真になること | **時間** N ms |
| チェック | 毎スケジュール（`cond` を再評価） | 満了時に1回 |
| 自前の遅延 | なし | あり |
| 用途 | **状態変化を待つ** | **固定時間待つ** |

どちらも `loop()` に制御を返すので、**他スレッドを止めない**（「ブロック」といってもシステムは止まらない）。

`PT_SLEEP` はこのライブラリでは `pt-sleep.h` にしかない。**`PT_WAIT_MS` は存在しない**（別実装の派生版にあるマクロ）。

なお `PT_SLEEP` は内部で `millis() - t > N` 判定なので、**厳密に N より大きく待つ**（`millis()` の分解能 1ms なら実効 N+1ms）。指定値ちょうどでは待たない。

---

## 3. 最重要ルール — ウェイトをまたぐ変数は `static`

> Because protothreads do not save the stack context across a blocking call, **local variables are not preserved when the protothread blocks.** … if in doubt, **do not use local variables inside a protothread!**
> — `pt-1.4-refman.pdf` §1.4

ウェイト箇所では `return PT_WAITING` で**関数を抜ける**。スタックが巻き戻るので、自動変数は消え、再開時には**未初期化のスタック領域を読む**ことになる。

```cpp
static PT_THREAD(worker(struct pt *pt))
{
    static bool flag;   /* ✅ ウェイトをまたぐ → static 必須 */
    bool raw;           /* ❌ そのままではウェイト後に不定値 */

    PT_BEGIN(pt);

    raw = readSensor();
    PT_SLEEP(pt, 20);         /* ここで return PT_WAITING → raw は消える */
    if (raw) { ... }          /* ← 不定値を読む */

    flag = readSensor();
    PT_SLEEP(pt, 20);
    if (flag) { ... }         /* ← 保持される */

    PT_END(pt);
}
```

対処は2つ（公式の推奨）。

1. **`static` を付ける** — 非再入ならこちら。公式サンプル `codelock_thread` も `static int keys;`
2. 再入が必要なら**明示的な状態オブジェクト**に退避して渡す

### 3.1 コンパイラは警告してくれない

論文（`dunkels05protothreads.pdf` §1.3.4）はこう書く。

> **Many optimizing C compilers, including gcc, are able to detect** if an automatic variable is unsafely used after a blocking statement. Typically a warning is produced, stating that the variable in question "**might be used uninitialized in this function**".

実測した結果、**この環境では一度も出ない**。

| 条件 | uninitialized 警告 |
| --- | --- |
| `-O0` / `-O1` / `-O2` / `-O3` / `-Os` × `lc-addrlabels` / `lc-switch` | **全組合せ 0件** |
| 実機ビルドそのまま（`-Os -flto -Wall`） | **0件** |

論文は2005年当時の GCC の挙動で、GCC 12.2.0 では再現しない。**コンパイラ頼みはできず、規約を自覚的に守るしかない。**

### 3.2 「たまたま動く」危険性

バグに気づいた経緯が象徴的だった。

```
A 単体実行          → raw のスタロットが偶然保持され 正しく棄却
A + スタック汚し    → raw = 0 に化け 誤確定
```

単体では動く。しかし実機では `millis()` の割り込みやコアの処理がスタックを踏み戻るため、**タイミング次第で誤動作する**。目に見えない不定値なので、テストをすり抜けやすい。

### 3.3 `cond` の中にも効く

`PT_WAIT_UNTIL(pt, cond)` の `cond` は**再開後に再評価**される。そこで参照する変数も同じルールの対象になる。

```cpp
static char worker(struct pt *p, int pin)
{
    bool cond;                          /* 宣言のみ・初期化なし */
    PT_BEGIN(p);
    cond = (digitalRead(pin) == LOW);   /* 代入は goto で飛び越えられる */
    PT_WAIT_UNTIL(p, cond);             /* ← 再開時に cond を読む */
    PT_END(p);
}
```

実測：

```
A(生の式 digitalRead(pin)==LOW)  退出 = 40ms  ← 正解
B(cond に自動変数)               退出 =  5ms  ← 1回目で不正に抜ける
```

`cond` に書いてよいのは **引数・ファイルスコープ変数・関数呼び出し**だけ。引数が安全なのは、`PT_SCHEDULE` や `loop()` が**毎回呼び出し直すため入口で再代入される**から。

なお、宣言を `PT_BEGIN` より前に置くと、初期化は毎回実行されるので安全になる（`PT_BEGIN` の後だと `goto` で飛ばれる）。

---

## 4. `PT_END` 到達と `for(;;)` の関係

`for(;;)` の中にウェイトがあると、`return PT_WAITING` が唯一の出口になるため **`PT_END` には一切到達しない**。逆に `for(;;)` を外すと毎回到達し、スレッドが再起動する。

だから **`for(;;)` は「繰り返し」ではなく「ワンタイム初期化を1回だけ走らせる」ための構造**になる（繰り返し自体は `loop()` が担っている）。

実測（10ms のみのノイズ入力・本来無視すべき）:

| 構成 | LED ちらつき |
| --- | --- |
| `for` あり＋初期化をスレッド内に置く | 0 回（正解） |
| **`for` なし＋初期化をスレッド内に置く** | **2 回（誤り・デバウンス無効化）** |
| `for` なし＋初期化を置かない | 0 回（正解） |

`for` を外す場合、初期化行を**削除または `setup()` へ移動**しないと、再起動のたびに `stable = digitalRead(...)` が走って瞬間値にリセットされ、デバウンスの確定分岐に一度も到達しなくなる。

初期化が不要なら `for` なしの線形構成が最も素直。ファイルスコープの `static bool stable;` はゼロ初期化＝「離し・LED消灯」が既に正解なので、**初期化行自体が不要になることが多い**。

---

## 5. デバウンスの設計

### 5.1 方式 — 待ち後に再読込して確定

タイムスタンプ比較（`millis() - changedAt >= 20`）ではなく、**変化を検出 → 20ms 待つ → その時点でもう一度読み込み、同じ状態なら確定**、とする方式。

- シーケンスが直線的で、必要な状態が `stable` だけに減る
- 検証：押下時は確定1回のみ、10msノイズは棄却（LED ちらつき0回）

### 5.2 押下側と離し側は「別々の遷移」を守る

```
while (!b->stable) DEBOUNCED_READ(...);   ← 押下確定の間だけ走る
digitalWrite(LED, HIGH);
...                                        ← ここから先は押下側デバウンスは稼働しない
```

**デバウンスは「1回通過したら以後ずっと有効なフィルタ」ではなく、「毎回の読み取りで行う確認処理」**。押下が確定した瞬間から押下側のループは回らなくなるので、押下中の入力は後段の読み取りが丸ごと受け止める。

### 5.3 リリースチャタは次サイクルの `DEBOUNCED_READ` で弾ける

離し検出を素の `PT_WAIT_UNTIL` にしても、残ったバウンスは次のサイクルの押下側 `DEBOUNCED_READ` に渡る。

1. バウンス中の LOW をサンプル → `stable(false)` と異なるので 20ms 待つ
2. 再読込するが**チャタは既に終わっている** → HIGH
3. サンプル値と不一致 → **棄却** → `stable` は false のまま維持

成立条件は **「チャタ継続 < `DEBOUNCE_MS`（≒20ms）」**。実測の境界：

| チャタ継続時間 | 離しをデバウンスで確定する方式 | 離しを素の条件待ちはなしにした方式 |
| --- | --- | --- |
| 5 / 10 / 15 / 20 / 25 / 30ms | 0/80 誤点滅 | **0/80 誤点滅** |
| 40ms | 0/80 | 16/80 |
| 60ms | 5/80 | 37/80 |

（80通り = 離し開始8種 × チャタ周期10種）

実タクトスイッチのチャタは概ね **1〜10ms** なので、余裕で安全圏。**離し側を素の条件待ちにした方が、コードは単純で、消灯も速くなる**（130ms → 105ms）。

ただし素の条件待ちにした場合、`b->stable` は更新されないので **`b->stable = false;` を手で入れないと無限ループ（MCUハング）**になる。これは**削除できない必須行**。

### 5.4 既知の限界 — 押下中のノイズ

押下中に一度だけ HIGH が混入するケースは、素の条件待ちだと**その場で「離した」と誤判定**する。

| | LED 反転 |
| --- | --- |
| 押下側・離し側ともデバウンス | **2 回（正解）** |
| 離し側を素の条件待ちはなし | **4 回（誤り・点滅）** |

リリースチャタとは性質が異なる（押下が本物に成立しているため、次サイクルのデバウンスは「まだ押されている」と正しく判定してしまう）。配線が長く EMI を拾う環境なら考慮する価値がある。

---

## 6. 読み込みにデバウンスを内包するマクロ

`DEBOUNCED_READ(pt, pin, pStable, pDb)` のように「読み込み1行で済ませる」ものを作るには、3つの制約がある。

### 6.1 マクロでしか作れない

関数にすると、`PT_WAIT_UNTIL` の `return PT_WAITING` が**ヘルパー関数から抜ける**だけで、スレッドは一切サスペンドしない。**プロトスレッドは呼ばれた関数からは yield できない。**

### 6.2 マクロ1展開につきウェイトは1箇所まで

`LC_SET` のラベルは `__LINE__` ベースで、マクロ内の複数ウェイトが**すべて呼び出し行番号**になる。

```
error: redefinition of label 'LC_LABEL38'
```

異なる待ち時間（無変化5ms / 変化時20ms）が必要なら、**三項演算子で待ち時間を実行時決定し、1箇所に統合**して回避する。

```cpp
PT_WAIT_UNTIL(pt, (millis() - t)
              > ((changed) ? DEBOUNCE_MS : POLL_MS));
```

### 6.3 待ちが隠れる分、呼び出し側が `static` 違反を作りやすい

マクロの中にウェイトがあると、その場所がコードから見えなくなる。呼び出し側で

```cpp
bool before = stable;      /* 見た目上は前ウエイト */
DEBOUNCED_READ(...);       /* ← ここに待ちが隠れている */
if (stable != before) ...  /* before がウェイトをまたぐ */
```

という誤りを書きやすい。**マクロ内部の状態はマクロ側で保持し、呼び出し側には触れさせない設計**にする。

---

## 7. 展開サイトとインスタンスの関係（複数ピン）

`static` やマクロで持つ一時状態は、**「展開サイト＝コード位置」単位**で共有される。

| 構成 | 結果 |
| --- | --- |
| スレッド**関数**ごとにマクロを展開 | 各自が独立に動作 ✅ |
| 同じ関数を複数の `struct pt` で駆動 | 一時状態が衝突し **あるインスタンスが飢餓（0回実行）・結果が誤る** ❌ |

実測（pin10=押下 / pin11=離し の2インスタンス）:

```
共有サイト (1関数 / 2インスタンス):  完了回数 A=28回 / B=0回,  stable11 = 0 (誤り)
別サイト   (関数を分けた):           完了回数 A=28回 / B=28回, stable11 = 1 (正解)
```

原因は、A がループのたびに共有された `_t = millis()` を書き換え、B の `millis() - _t` が常に0で待機が解けないこと。

### 解決 — 状態を構造体で渡す

`struct pt` と一時状態を1つの構造体に束ねてポインタで渡せば、**1つのスレッド関数・1つの展開サイトで複数インスタンスを処理でき**、再入可能になる。

```
1つのスレッド関数 / 3インスタンス（状態は構造体で渡す）
  pin10 (40ms押下) → stable=1 確定= 65ms  正解=1
  pin11 (80ms押下) → stable=1 確定=105ms  正解=1
  pin12 (押さない) → stable=0              正解=0
```

ピンが増えたら `struct Button` の配列にすればよい。

---

## 8. ボタン制御スレッドの完成形

```cpp
#include <Arduino.h>
#include <pt.h>

static const uint8_t       LED_PIN     = 2;   /* D2  = PC0 = 内蔵LED */
static const uint8_t       BUTTON_PIN  = 10;  /* D10 = PD0 = タクトボタン */
static const unsigned long POLL_MS     = 5;   /* 変化なし時のポーリング周期 [ms] */
static const unsigned long DEBOUNCE_MS = 20;  /* 変化時の確認待ち [ms] */

/* デバウンスで使う一時状態。ウェイトをまたぐため自動変数に置けない。
   インスタンスごとに用意することで複数ピンでも干渉しない。 */
struct Debounce {
    bool          raw;   /* サンプルした生の状態 (true = 押下) */
    unsigned long t;     /* サンプル時刻 */
};

/* ボタン1個分の状態。ピンが増えたらこの構造体の配列にする。 */
struct Button {
    struct pt       pt;
    uint8_t         pin;
    bool            stable;   /* 確定した押下状態 (初期値 false = 離し) */
    struct Debounce db;
};

/* デバウンスを「読み込み」に内包したマクロ
   - 呼ばれた関数からは yield できないためマクロでなければならない
   - ラベルが __LINE__ ベースなので 1展開につきウェイトは1箇所まで
   - ウェイトをまたぐ値は pDb 経由で保持。static も自動変数も使わない */
#define DEBOUNCED_READ(pt, pin, pStable, pDb) do {                  \
    (pDb)->raw = (digitalRead(pin) == LOW);                         \
    (pDb)->t   = millis();                                          \
    PT_WAIT_UNTIL(pt, (millis() - (pDb)->t)                            \
                      > (((pDb)->raw != *(pStable)) ? DEBOUNCE_MS : POLL_MS)); \
    if ((pDb)->raw != *(pStable)                                    \
        && (digitalRead(pin) == LOW) == (pDb)->raw)                 \
        *(pStable) = (pDb)->raw;                                    \
} while (0)

static struct Button button = { {}, BUTTON_PIN, false, {} };

static PT_THREAD(buttonThread(struct Button *b))
{
    PT_BEGIN(&b->pt);

    /* 1. 押下が確定するまで待つ
       離しの残チャタも次サイクルのこの DEBOUNCED_READ が棄却する
       (成立条件: ﾁｬﾀ継続 < DEBOUNCE_MS ≒ 20ms。実タクトは1〜10ms) */
    while (!b->stable)
        DEBOUNCED_READ(&b->pt, b->pin, &b->stable, &b->db);
    digitalWrite(LED_PIN, HIGH);

    /* 2. 押されている間はここで待つ (離しを検出するまで)
       押下は確定済みなので単純な条件待ちでよく、デバウンスは不要 */
    PT_WAIT_UNTIL(&b->pt, digitalRead(b->pin) != LOW);

    /* ★必須: stable を復帰させないと、次回の while(!b->stable) が
       一切回らず、LED の点滅と PT_END 到達の無限ループになる */
    b->stable = false;
    digitalWrite(LED_PIN, LOW);

    /* 3. 「1回押して離した」がここで成立する (アクションを置く場所) */

    PT_END(&b->pt);   /* 再起動して次の押下待ちへ */
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
    /* ここに複数のスレッドを追記できる（協調スレッドなので相互にブロックしない）。
       複数ボタンは struct Button の配列にして
       for (auto &b : buttons) buttonThread(&b); と書く
       （状態構造体方式なので 1関数で複数インスタンスを処理できる）。 */
    buttonThread(&button);
}
```

構成の要点：

- **`for(;;)` なし** — 繰り返しは `loop()` が担う。ワンタイム初期化も不要（`stable` はゼロ初期化＝離しが正解）
- **状態は構造体** — 1関数で複数ピンにスケール可能
- **押下はデバウンス付き、離しは条件待ち** — 意図がコード上に明示される

このあとシリアル入力スレッドを足して2スレッド並行にした形が §9。

---

## 9. 2スレッドの並行 — シリアル入力の追加

`buttonThread` に加えて、シリアルから1文字を受け取り**大小文字を反転して送り返す** `serialThread` を追加した。狙いは2つ —「複数スレッドが実際に並行する様子」と「ブロックしそうな API をどう扱うか」。

### 9.1 このコアの罠 — `Serial.available()` が使えない

Arduino 流の定番は次だが、**この環境では成立しない**。

```cpp
PT_WAIT_UNTIL(pt, Serial.available() > 0);   /* ❌ 永遠に真にならない */
c = Serial.read();
```

openwch コアの `HardwareSerial::available()` は**未実装で常に `-1` を返す**（`cores/arduino/HardwareSerial.cpp:148`）。`-1 > 0` は永遠に偽なので、スレッドは**1文字も読めずに待ち続ける**。

一方 `Serial.read()` は**非ブロッキング**だ。内部の `uart_getc()`（`cores/arduino/ch32/uart.c:515`）が RXNE（受信データレジスタにデータ有）を確認し、データが無ければ `-1` を返す。

→ 「読んでも `-1` なら待ち続ける」ことで、**入力有無の確認と読み込みが1つの条件式で兼ねられる**。

> 「`Serial.read()` はブロック IO だから先に入力確認を」という前提も、このコアでは逆だった。**API がブロックするかどうかは実装を読んでから**決めること。

### 9.2 実装

```cpp
struct SerialIn {
    struct pt pt;
    int c;   /* Serial.read() の戻り値。-1 = 未受信 / 0〜255 = 受信文字 */
};

static struct SerialIn serialIn = { {}, -1 };

static PT_THREAD(serialThread(struct SerialIn *s))
{
    PT_BEGIN(&s->pt);

    /* 1. 入力が来るまで待つ（読んでも -1 なら待つ = 入力確認を兼ねる） */
    PT_WAIT_UNTIL(&s->pt, (s->c = Serial.read()) >= 0);

    /* 2. ここに到達すれば s->c に1文字入っている。この間ウェイトは無い */
    if (s->c >= 'a' && s->c <= 'z')
        s->c -= ('a' - 'A');   /* a-z → A-Z */
    else if (s->c >= 'A' && s->c <= 'Z')
        s->c += ('a' - 'A');   /* A-Z → a-z */
    Serial.write((uint8_t)s->c);

    PT_END(&s->pt);   /* 再起動して次の1文字待ちへ */
}
```

規約の適用箇所:

- **読み込んだ文字を `int c` として構造体に置く** — 「読む」がウェイトの**条件式の中**にあるため、再開のたびに評価される。ここに自動変数を持たせれば §3.3 の `cond` 違反そのものになる。引数 `s` が安全なのは §3.3 と同じ理屈（毎回 `loop()` から呼ばれ直し、入口で再代入される）。
- 変換と `Serial.write()` はウェイトを挟まないので、確定した直後の `s->c` をそのまま使える。
- `struct pt` とデータを同じ構造体に束ねているので、§7 の「展開サイトの共有」問題も同じ方式で回避できる。

### 9.3 なぜボタンが遅延しないか

```cpp
void loop()
{
    buttonThread(&button);
    serialThread(&serialIn);
}
```

| | 待っている間 | 実処理 |
| --- | --- | --- |
| `buttonThread` | `PT_WAIT_UNTIL` で `loop()` に制御を返す | 確定判定と `digitalWrite` のみ |
| `serialThread` | 同上（`read()` が `-1` を返した瞬間） | 1文字の変換と `write` のみ |

両スレッドとも**待ちは必ず `loop()` に返し、実処理は一瞬**で終わる。だから文字を打っている間もボタンは毎回スケジュールされ、逆も同じ。

逆に、次のどれかを入れるとボタン応答が壊れる。

- **ブロックする読み込み**（データが来るまで戻らない `read` / `available` 相当の実装）
- **スレッド内の `delay()` や `while` の素待ち**（`PT_WAIT_UNTIL` が無いループ）
- **実行時間が長い処理**（その分だけ他スレッドが止まる。分割するか状態を切る）

`PT_SLEEP` も `loop()` に返るので他スレッドは止まらないが、**返るまでの間は同じ挙動**になる点は共通する。

### 9.4 検証

| ケース | 期待 | 結果 |
| --- | --- | --- |
| `abc123XYZ` を入力 | `ABC123xyz` | ✅ |
| 入力なしの間、ボタン操作 | LED 即応 | ✅ 遅延なし |
| 文字入力と同時にボタン操作 | 両方が動作 | ✅ |

ハード側の注意: `Serial` は USART1（TX = PD5/D15、RX = PD6/D16）。**PD5 は TX と USB プルアップ制御の兼用ピン**なので、シリアルを使う間は rv003usb の USB 通信は使えない。また `setup()` で `Serial.begin(115200)` を呼ばないと動かない。

---

## 10. 検証結果まとめ

### 挙動

| ケース | LED 反転 | 押下完了 | 結果 |
| --- | --- | --- | --- |
| 押下40〜100ms（チャタ付き） | 2回（点灯65ms / 消灯105ms） | 1回 | ✅ |
| 10msノイズのみ（押していない） | 0回 | 0回 | ✅ |
| 押下中70〜75msノイズ | 4回 | 2回 | ⚠️ 既知の限界 |

### リソース

| 構成 | RAM | Flash |
| --- | --- | --- |
| LED点滅（Protothreads なし） | 472 (23.0%) | 6528 (39.8%) |
| デバウンス（`for` あり・ファイル static） | 488 (23.8%) | 6472 (39.5%) |
| **完成形（`for` なし・構造体・離し=条件待ち）** | **488 (23.8%)** | **6708 (40.9%)** |
| +シリアルスレッド（2スレッド並行・大小文字反転） | 496 (24.2%) | 6836 (41.7%) |

スレッドを1つ足すコストは **RAM 8 bytes / Flash 128 bytes**（`struct pt` とロジック分）。16KB 中 9.3KB 空き。読みやすさ（線形の3段階、`PT_WAIT_UNTIL` が意図を明示、不変条件のコメント）を優先する十分な対価。

---

## 11. チェックリスト

新しい `PT_THREAD` を書くとき:

- [ ] ウェイトをまたいで使う変数は `static` / 構造体 / 引数のどれか（**自動変数は禁止**）
- [ ] `PT_WAIT_UNTIL` の `cond` 内も同じ（生の関数呼び出し・引数・グローバルのみ）
- [ ] 独自の値を `return` していないか（戻り値はスケジューラ状態）
- [ ] `for(;;)` を外すなら、スレッド内のワンタイム初期化も外したか
- [ ] マクロにウェイトを2つ以上書いていないか（`__LINE__` 衝突）
- [ ] 複数インスタンスは1関数を共有していないか（状態構造体で分離）
- [ ] `b->stable = false;` のような状態復帰行を消していないか（ハングの原因）
- [ ] スレッド内にブロックする API（`delay()`、待ち受け読み込み、実行時間の長い処理）を入れていないか（止まるのはその間だけでも他スレッドが止まる）
- [ ] 待ちに使う API の戻り値・挙動を実装で確認したか（このコアは `Serial.available()` が常に `-1`）

---

## 参考

- Protothreads 公式: <https://dunkels.com/adam/pt/>
- リファレンスマニュアル: `pt-1.4-refman.pdf`（§1.4 Local variables が最重要）
- 論文: Adam Dunkels et al., *Protothreads: Simplifying Event-Driven Programming of Memory-Constrained Embedded Systems* (2005) — §1.3.4 Limitations
- 本プロジェクトの注意点: `AGENTS.md` → 「Protothreads の注意点」
