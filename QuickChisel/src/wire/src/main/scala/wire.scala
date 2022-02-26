import chisel3._
import chisel3.stage._

import java.io.PrintWriter

class Wire extends Module {
    val io = IO(new Bundle {
        val switch = Input(Bool())
        val led = Output(Bool())
    })

    io.led := io.switch
}

object VerilogEmitter extends App {
    val writer = new PrintWriter("target/wire.v")
    writer.write(ChiselStage.emitVerilog(new Wire))
    writer.close()
}