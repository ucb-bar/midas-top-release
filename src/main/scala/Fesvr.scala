package midas
package top

import chisel3._
import chisel3.util._
import rocketchip._
import uncore.tilelink._
import uncore.tilelink2.{TLLegacy, TLHintHandler}
import config.Parameters

class FesvrBundle(implicit p: Parameters) extends TLBundle {
  val addr = Input(UInt(p(rocket.PAddrBits).W))
  val wdata = Input(UInt(tlDataBits.W))
  val rdata = Output(UInt(tlDataBits.W))
  val metaIn = Input(UInt(3.W))
  val metaOut = Output(UInt(3.W))
}

class FesvrModule(implicit p: Parameters) extends TLModule {
  val io = IO(new Bundle {
    val fesvr = new FesvrBundle
    val mem = new ClientUncachedTileLinkIO
  })
  val wr = io.fesvr.metaIn(2)
  val in_valid = io.fesvr.metaIn(1)
  val out_ready = io.fesvr.metaIn(0)
  val blockOffset = tlBeatAddrBits + tlByteAddrBits
  val blockAddr = io.fesvr.addr >> UInt(blockOffset)
  val beatAddr = io.fesvr.addr(blockOffset - 1, tlByteAddrBits)

  io.mem.acquire.bits := Mux(wr, Put(UInt(0), blockAddr, beatAddr, io.fesvr.wdata),
                                 Get(UInt(0), blockAddr, beatAddr))
  io.mem.acquire.valid := in_valid
  io.mem.grant.ready := out_ready

  io.fesvr.rdata := io.mem.grant.bits.data
  io.fesvr.metaOut := Cat(io.mem.grant.bits.hasData(), io.mem.grant.valid, io.mem.acquire.ready)
}

trait PeripheryFesvr extends L2Crossbar {
  val tlLegacy = diplomacy.LazyModule(
    new TLLegacy()(p alterPartial ({ case TLId => "L1toL2" })))
  l2.node := TLHintHandler()(tlLegacy.node)
}

trait PeripheryFesvrBundle extends L2CrossbarBundle {
  implicit val p: Parameters
  val outer: PeripheryFesvr
  val fesvr = new FesvrBundle()(p alterPartial ({ case TLId => "L1toL2" }))
}

trait PeripheryFesvrModule extends L2CrossbarModule {
  val outer: PeripheryFesvr
  val io: PeripheryFesvrBundle
  val fesvr = Module(new FesvrModule()(p alterPartial ({ case TLId => "L1toL2" })))
  io.fesvr <> fesvr.io.fesvr
  outer.tlLegacy.module.io.legacy <>fesvr.io.mem
}
