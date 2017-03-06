package midas
package top

import Chisel._ // Compatibility mode due to bulk connections
import diplomacy.LazyModule
import uncore.tilelink2._
import uncore.devices._
import coreplex._
import rocket._
import rocketchip._
import config.Parameters

// Platform with no debug module
trait MidasTopPlatform extends CoreplexNetwork {
  val module: MidasTopPlatformModule

  val plic  = LazyModule(new TLPLIC(hasSupervisor, maxPriorities = 7))
  val clint = LazyModule(new CoreplexLocalInterrupter)

  plic.node  := TLFragmenter(cbus_beatBytes, cbus_lineBytes)(cbus.node)
  clint.node := TLFragmenter(cbus_beatBytes, cbus_lineBytes)(cbus.node)

  plic.intnode := intBar.intnode

  lazy val configString = {
    val managers = l1tol2.node.edgesIn(0).manager.managers
    // Use the existing config string if the user overrode it
    _root_.util.ConfigStringOutput.contents.getOrElse(
      rocketchip.GenerateConfigString(p, clint, plic, managers))
  }
}

trait MidasTopPlatformModule extends CoreplexNetworkModule {
  val outer: MidasTopPlatform
  val io: MidasTopPlatformBundle

  // Synchronize the rtc into the coreplex
  val rtcSync = ShiftRegister(io.rtcToggle, 3)
  val rtcLast = Reg(init = Bool(false), next=rtcSync)
  outer.clint.module.io.rtcTick := Reg(init = Bool(false), next=(rtcSync & (~rtcLast)))
}

trait MidasTopPlatformBundle extends CoreplexNetworkBundle {
  val outer: MidasTopPlatform

  val rtcToggle = Bool(INPUT)
  val resetVector = UInt(INPUT, p(rocket.XLen))
}

///

trait MidasTiles extends MidasTopPlatform {
  val module: MidasTilesModule

  val midasTiles = p(RocketConfigs) map (c =>
    LazyModule(new RocketTile(c)(p alterPartial {
      case SharedMemoryTLEdge => l1tol2.node.edgesIn(0)
      case PAddrBits => l1tol2.node.edgesIn(0).bundle.addressBits
    }))
  )

  midasTiles foreach { r =>
    r.masterNodes foreach (l1tol2.node := _)
    r.slaveNode foreach (_ := cbus.node)
  }

  val tileIntNodes = midasTiles map (_ => IntInternalOutputNode())
  tileIntNodes.foreach (_ := plic.intnode)
}

trait MidasTilesBundle extends MidasTopPlatformBundle {
  val outer: MidasTiles
}

trait MidasTilesModule extends MidasTopPlatformModule {
  val outer: MidasTiles
  val io: MidasTilesBundle

  outer.midasTiles.map(_.module).zipWithIndex.foreach { case (tile, i) =>
    tile.io.hartid := UInt(i)
    tile.io.resetVector := io.resetVector
    tile.io.interrupts := outer.clint.module.io.tiles(i)
    tile.io.interrupts.meip := outer.tileIntNodes(i).bundleOut(0)(0)
    tile.io.interrupts.seip.foreach(_ := outer.tileIntNodes(i).bundleOut(0)(1))
  }
}

///

class MidasCoreplex(implicit p: Parameters) extends BaseCoreplex
    with MidasTopPlatform
    with HasL2MasterPort
    with MidasTiles {
  override lazy val module = new MidasCoreplexModule(this, () => new MidasCoreplexBundle(this))
}

class MidasCoreplexBundle[+L <: MidasCoreplex](_outer: L)
    extends BaseCoreplexBundle(_outer)
    with MidasTopPlatformBundle
    with HasL2MasterPortBundle
    with MidasTilesBundle

class MidasCoreplexModule[+L <: MidasCoreplex, +B <: MidasCoreplexBundle[L]](_outer: L, _io: () => B)
    extends BaseCoreplexModule(_outer, _io)
    with MidasTopPlatformModule
    with HasL2MasterPortModule
    with MidasTilesModule

///

trait MidasPlexMaster extends L2Crossbar {
  val module: MidasPlexMasterModule
  val mem: Seq[TLInwardNode]

  val coreplex = LazyModule(new MidasCoreplex)

  coreplex.l2in := l2.node
  socBus.node := coreplex.mmio
  coreplex.mmioInt := intBus.intnode
  mem foreach (_ := coreplex.mem)
}

trait MidasPlexMasterBundle extends L2CrossbarBundle {
  val outer: MidasPlexMaster
}

trait MidasPlexMasterModule extends L2CrossbarModule {
  val outer: MidasPlexMaster
  val io: MidasPlexMasterBundle
}

///

trait HardwiredResetVector extends TopNetwork {
  val module: HardwiredResetVectorModule
  val coreplex: MidasTopPlatform
}

trait HardwiredResetVectorBundle extends TopNetworkBundle {
  val outer: HardwiredResetVector
}

trait HardwiredResetVectorModule extends TopNetworkModule {
  val outer: HardwiredResetVector
  val io: HardwiredResetVectorBundle

  outer.coreplex.module.io.resetVector := UInt(0x1000) // boot ROM
}

trait PeripheryCounter extends TopNetwork {
  val module: PeripheryCounterModule
  val coreplex: MidasTopPlatform
}

trait PeripheryCounterBundle extends TopNetworkBundle {
  val outer: PeripheryCounter
}

trait PeripheryCounterModule extends TopNetworkModule {
  val outer: PeripheryCounter
  val io: PeripheryCounterBundle

  val period = p(rocketchip.RTCPeriod)
  val rtcCounter = RegInit(UInt(0, width = log2Up(period)))
  val rtcWrap = rtcCounter === UInt(period-1)
  rtcCounter := Mux(rtcWrap, UInt(0), rtcCounter + UInt(1))

  outer.coreplex.module.io.rtcToggle := rtcCounter(log2Up(period)-1)
}
