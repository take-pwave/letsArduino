#include <Arduino.h>
#include <pt.h>

//
//  ・タクトボタンを押している間だけ内蔵LEDを点灯させる
//  ・シリアルから1文字入力し、大小文字を反転して送り返す
//
// 配線:
//   タクトボタン  D10 (PD0) --- GND  (外部抵抗なし → 内部プルアップを使用)
//   内蔵LED       D2  (PC0)
//   シリアル      D15 (PD5) = TX / D16 (PD6) = RX  … WCH-LinkE の UART ブリッジ
//
// 挙動は「押下確定 → 押されている間待つ → 離し確定」の3段階。
// すべて条件待ち (PT_WAIT_UNTIL) で、デバウンス読み込みをその中で回す。
//

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
    PT_WAIT_UNTIL(pt, (millis() - (pDb)->t)                            \
                      > (((pDb)->raw != *(pStable)) ? DEBOUNCE_MS : POLL_MS)); \
    if ((pDb)->raw != *(pStable)                                    \
        && (digitalRead(pin) == LOW) == (pDb)->raw)                 \
        *(pStable) = (pDb)->raw;                                    \
} while (0)

static struct Button button = { {}, BUTTON_PIN, false, {} };

// シリアル入力スレッドの状態。読み込んだ文字はウェイトをまたぐので
// 自動変数(スタック)に置けず、構造体で保持する。
struct SerialIn {
    struct pt pt;
    int c;   // Serial.read() の戻り値。-1 = 未受信 / 0〜255 = 受信文字
};

static struct SerialIn serialIn = { {}, -1 };

// while の条件は b->stable (構造体) を参照するだけ。ウェイトをまたいで問題になる
// 自動変数は持たない。繰り返しは loop() が担うので for も不要。
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
    //    押下は確定済みなので単純な条件待ちでよく、デバウンスは不要。
    PT_WAIT_UNTIL(&b->pt, digitalRead(b->pin) != LOW);

    // ★必須: stable を復帰させないと、次回の while(!b->stable) が
    //   一切回らず、LED の点滅と PT_END 到達の無限ループになる。
    b->stable = false;
    digitalWrite(LED_PIN, LOW);

    // 3. 「1回押して離した」がここで成立する (アクションを置く場所)

    PT_END(&b->pt);   // 再起動して次の押下待ちへ
}

// シリアルから1文字受け取り、大小文字を反転して送り返す
static PT_THREAD(serialThread(struct SerialIn *s))
{
    PT_BEGIN(&s->pt);

    // 1. 入力が来るまで待つ。
    //    このコアの Serial.available() は未実装で常に -1 を返すため使えない。
    //    代わりに read() の戻り値で判断する — 内部の uart_getc() は
    //    RXNE(受信データレジスタにデータ有)を確認してから読み、
    //    データが無ければ -1 を返す非ブロッキング実装になっている。
    //    よって「読んでも -1 なら待ち続ける」= 入力有無の確認を兼ねる。
    //    条件は引数 s とそのフィールドだけを参照しているので、
    //    ウェイト跨ぎに使う自動変数は無い。
    PT_WAIT_UNTIL(&s->pt, (s->c = Serial.read()) >= 0);

    // 2. ここに到達すれば s->c に1文字入っている。この間ウェイトは無い。
    //    大小文字を反転する (英字以外はそのまま)。
    if (s->c >= 'a' && s->c <= 'z')
        s->c -= ('a' - 'A');   // a-z → A-Z
    else if (s->c >= 'A' && s->c <= 'Z')
        s->c += ('a' - 'A');   // A-Z → a-z
    Serial.write((uint8_t)s->c);

    PT_END(&s->pt);   // 再起動して次の1文字待ちへ
}

void setup()
{
    // D15 (PD5) は TX であると同時に USB のプルアップ制御でもあるため、
    // シリアルを使う間は USB (rv003usb) 側の通信は使えない。
    // ボードレートは platformio.ini の monitor_speed と合わせる。
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(button.pin, INPUT_PULLUP);
    digitalWrite(LED_PIN, LOW);
    PT_INIT(&button.pt);
    PT_INIT(&serialIn.pt);
}

void loop()
{
    /* ここに複数のスレッドを追記できる（協調スレッドなので相互にブロックしない）。
       複数ボタンは struct Button の配列にして
       for (auto &b : buttons) buttonThread(&b); と書く
       （状態構造体方式なので 1関数で複数インスタンスを処理できる）。 */
    buttonThread(&button);
    serialThread(&serialIn);
}
