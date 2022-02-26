import chisel3._
import org.scalatest._
import chiseltest._

import chiseltest.experimental.TestOptionBuilder._
import chiseltest.internal.WriteVcdAnnotation

class BlinkTest extends FlatSpec with ChiselScalatestTester {

    behavior of "Blink"

    it should "クロック周波数でOn/Offを繰り返す" in {

        // テストのためクロック周波数を5とし、４サイクル分実行する
        val clock_frequency = 5

        test(new Blink(clock_frequency)).withAnnotations(Seq(WriteVcdAnnotation)) { c =>
            // ４サイクル分のOn/Offをセット（期待値に使用）
            val led_cycle = Range(0, 8).map(_ % 2 === 0)
            for (expected_led <- led_cycle) {
                var i = 0
                while(i != clock_frequency) {
                    c.io.led.expect(expected_led.B)
                    c.clock.step()
                    i += 1
                }
            }
        }
    }
}