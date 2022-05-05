import chisel3._
import org.scalatest._
import chiseltest._

class BinOpsTest extends FlatSpec with ChiselScalatestTester {

    behavior of "BinOps"

    it should "２項演算（＆）" in {
        test(new BinOpsTop())  { c => 

            c.io.a.poke(false.B)
            c.io.b.poke(false.B)
            c.io.c.expect(false.B)
            c.clock.step()

            c.io.a.poke(false.B)
            c.io.b.poke(true.B)
            c.io.c.expect(false.B)
            c.clock.step()

            c.io.a.poke(true.B)
            c.io.b.poke(false.B)
            c.io.c.expect(false.B)
            c.clock.step()

            c.io.a.poke(true.B)
            c.io.b.poke(true.B)
            c.io.c.expect(true.B)
            c.clock.step()
        }
    }    
}