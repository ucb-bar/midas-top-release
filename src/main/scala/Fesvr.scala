package midas
package top

import chisel3._
import chisel3.util._
import cde.Parameters
import uncore.tilelink._

class FesvrBundle(implicit p: Parameters) extends TLBundle {
  val addr = UInt(INPUT, p(junctions.PAddrBits))
  val wdata = UInt(INPUT, tlDataBits)
  val rdata = UInt(OUTPUT, tlDataBits)
  val metaIn = UInt(INPUT, width=3)
  val metaOut = UInt(OUTPUT, width=3)
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

trait PeripheryFesvr extends diplomacy.LazyModule {
  implicit val p: Parameters
  val pBusMasters: rocketchip.RangeManager
  pBusMasters.add("fesvr", 1)
}

trait PeripheryFesvrBundle {
  implicit val p: Parameters
  val fesvr = new FesvrBundle()(p alter Map(TLId -> "L1toL2"))
}

trait PeripheryFesvrModule {
  implicit val p: Parameters
  val outer: PeripheryFesvr
  val io: PeripheryFesvrBundle
  val coreplexIO: coreplex.BaseCoreplexBundle

  val (master_idx, _) = outer.pBusMasters.range("fesvr")
  val fesvr = Module(new FesvrModule()(p alter Map(TLId -> "L1toL2")))
  io.fesvr <> fesvr.io.fesvr
  coreplexIO.slave(master_idx) <> fesvr.io.mem
  coreplexIO.debug.req.valid := Bool(false)
  coreplexIO.debug.resp.ready := Bool(false)
}
