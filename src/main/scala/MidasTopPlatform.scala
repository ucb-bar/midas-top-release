//See LICENSE for license details.

package midas
package top

import Chisel._ // Compatibility mode due to bulk connections
import diplomacy.LazyModule
import uncore.tilelink2._
import uncore.devices._
import coreplex._
import tile._
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
    rocketchip.GenerateConfigString(p, clint, plic, managers)
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
  val resetVector = UInt(INPUT, p(tile.XLen))
}

///

trait MidasTiles extends MidasTopPlatform {
  val module: MidasTilesModule

  private val configs = p(RocketTilesKey)
  private val tileIntNodes = configs map (_ => IntInternalOutputNode())
  tileIntNodes foreach (_ := plic.intnode)

  private def wireInterrupts(x: TileInterrupts, i: Int) {
    x := clint.module.io.tiles(i)
    x.debug := false.B
    x.meip := tileIntNodes(i).bundleOut(0)(0)
    x.seip foreach (_ := tileIntNodes(i).bundleOut(0)(1))
  }

  val tileWires = configs.zipWithIndex map { case (key, i) =>
    // Synchronous Crossing
    val tile = LazyModule(new RocketTile(key)(p alterPartial {
      case TileKey => key
      case BuildRoCC => key.rocc
      case SharedMemoryTLEdge => l1tol2.node.edgesIn(0)
      case PAddrBits => l1tol2.node.edgesIn(0).bundle.addressBits
    }))
    val buffer = LazyModule(new TLBuffer)
    buffer.node :=* tile.masterNode
    l1tol2.node :=* buffer.node
    tile.slaveNode :*= cbus.node
    (io: MidasTilesBundle) => {
      tile.module.io.hartid := UInt(i)
      tile.module.io.resetVector := io.resetVector
      wireInterrupts(tile.module.io.interrupts, i)
    }
  }
}

trait MidasTilesBundle extends MidasTopPlatformBundle {
  val outer: MidasTiles
}

trait MidasTilesModule extends MidasTopPlatformModule {
  val outer: MidasTiles
  val io: MidasTilesBundle
  outer.tileWires foreach (_(io))
}

///

class MidasCoreplex(implicit p: Parameters) extends BaseCoreplex
    with MidasTopPlatform
    with MidasTiles {
  override lazy val module = new MidasCoreplexModule(this, () => new MidasCoreplexBundle(this))
}

class MidasCoreplexBundle[+L <: MidasCoreplex](_outer: L)
    extends BaseCoreplexBundle(_outer)
    with MidasTopPlatformBundle
    with MidasTilesBundle

class MidasCoreplexModule[+L <: MidasCoreplex, +B <: MidasCoreplexBundle[L]](_outer: L, _io: () => B)
    extends BaseCoreplexModule(_outer, _io)
    with MidasTopPlatformModule
    with MidasTilesModule

///

trait MidasPlexMaster extends TopNetwork {
  val module: MidasPlexMasterModule

  val coreplex = LazyModule(new MidasCoreplex)

  coreplex.l2in :=* l2.node
  socBus.node := coreplex.mmio
  coreplex.mmioInt := intBus.intnode

  require(mem.size == coreplex.mem.size)
  (mem zip coreplex.mem) foreach { case (xbar, channel) => xbar.node :=* channel }
}

trait MidasPlexMasterBundle extends TopNetworkBundle {
  val outer: MidasPlexMaster
}

trait MidasPlexMasterModule extends TopNetworkModule {
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
