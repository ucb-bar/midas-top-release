package midas
package top

import dram_midas._

import config.{Parameters, Config}
import tile._
import rocket._
import coreplex._
import uncore.tilelink._
import uncore.coherence._
import uncore.agents._
import uncore.devices.NTiles

class ZynqConfigWithMemModel extends Config(new WithLBPipe ++ new ZynqConfig)
class ZynqConfig extends Config(new midas.ZynqConfig)

class WithLBPipe extends Config((site, here ,up) => {
  case MemModelKey => Some((p: Parameters) => new MidasMemModel(
    new LatencyPipeConfig(new BaseParams(maxReads = 16, maxWrites = 16)))(p))
})

class MidasTopConfig extends Config((site, here, up) => {
  //L2 Memory System Params
  case AmoAluOperandBits => site(XLen)
  case NAcquireTransactors => 7
  case L2StoreDataQueueDepth => 1
  case BuildRoCC => Nil

  case CacheBlockOffsetBits => chisel3.util.log2Up(site(CacheBlockBytes))
  case RocketTilesKey => up(RocketTilesKey) map (
    tile => tile.copy(core = tile.core.copy(useDebug = false)))
  case TLKey("L1toL2") =>
    val nTiles = site(NTiles)
    val useMEI = nTiles <= 1
    val dir = new NullRepresentation(nTiles)
    TileLinkParameters(
      coherencePolicy = if (useMEI) new MEICoherence(dir) else new MESICoherence(dir),
      nManagers = site(BankedL2Config).nBanks + 1 /* MMIO */,
      nCachingClients = 1,
      nCachelessClients = 1,
      maxClientXacts = List(
        // L1 cache
        site(RocketTilesKey).head.dcache.get.nMSHRs + 1 /* IOMSHR */,
        // RoCC
        if (site(BuildRoCC).isEmpty) 1 else site(RoccMaxTaggedMemXacts)).max,
      maxClientsPerPort = if (site(BuildRoCC).isEmpty) 1 else 2,
      maxManagerXacts = site(NAcquireTransactors) + 2,
      dataBeats = (8 * site(CacheBlockBytes)) / site(XLen),
      dataBits = (8 * site(CacheBlockBytes))
    )
})

class DefaultExampleConfig extends Config(new MidasTopConfig ++ new WithNBigCores(1) ++ new rocketchip.BaseConfig)
/*
class SmallBOOMConfig extends Config(new NoBrPred ++ new MidasTopConfig ++ new boom.SmallBOOMConfig)

class NoBrPred extends Config((site, here, up) => {
  case boom.EnableBranchPredictor => false
})
*/
class RocketChip1GExtMem extends Config(new rocketchip.WithExtMemSize(0x40000000L) ++ new DefaultExampleConfig)
class RocketChip2GExtMem extends Config(new rocketchip.WithExtMemSize(0x80000000L) ++ new DefaultExampleConfig)
