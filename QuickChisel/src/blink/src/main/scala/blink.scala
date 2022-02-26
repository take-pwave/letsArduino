import chisel3._
import chisel3.stage._

import java.io.PrintWriter

// １秒ごとにOn/Offを繰り返す
class Blink(clk_frequency: Int) extends Module {
    // I/Oを定義：出力ポートled
    val io = IO(new Bundle {
        val led = Output(Bool())
    })

    // LEDの状態を保持
    val r_led_status = RegInit(true.B)

    // １秒分のクロックカウント
    val MAX_CLOCK_COUNT = (clk_frequency - 1).U

    // １秒分のカウンターに必要なビット数を.getWidthで取得
    val r_counter = RegInit(0.U(MAX_CLOCK_COUNT.getWidth.W))

    // カウンターがMAX_CLOCK_COUNTに達したら、LEDの状態を反転させる
    when (r_counter === MAX_CLOCK_COUNT) {
        // LEDの状態反転
        r_led_status := ~r_led_status
        // カウンターのリセット
        r_counter := 0.U
    } otherwise {
        r_counter := r_counter + 1.U
    }

    // LEDの状態を出力信号にセット
    io.led := r_led_status
}

object VerilogEmitter extends App {
    val writer = new PrintWriter("target/blink.v")

    // iCEBreakerのクロックは12MHz
    writer.write(ChiselStage.emitVerilog(new Blink(12000000)))
    writer.close()
}