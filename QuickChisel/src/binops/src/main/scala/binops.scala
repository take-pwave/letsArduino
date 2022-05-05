import chisel3._
import chisel3.stage._

import java.io.PrintWriter

// 集合体BinOpsIOを定義
class BinOpsIO extends Bundle {
    val a = Input(Bool())
    val b = Input(Bool())
    val c = Output(Bool())
}

// ２項演算（積）を行うBinOpsクラス
class BinOps extends Module {
    val io = IO(new BinOpsIO)

    io.c := io.a & io.b
}

// BinOpsのトップクラス（負論理から正論理に変換）
class BinOpsTop extends Module {
    val io = IO(new BinOpsIO)
    val binops = Module(new BinOps)
    
    // 負論理から正論理への変換
    binops.io.a := ~io.a
    binops.io.b := ~io.b
    io.c := ~binops.io.c
}

object VerilogEmitter extends App {
    val writer = new PrintWriter("target/binops.v")
    writer.write(ChiselStage.emitVerilog(new BinOpsTop))
    writer.close()
}