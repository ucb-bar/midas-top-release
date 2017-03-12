package midas
package top

import config.{Parameters, Config}
import dram_midas._
import rocket._
import coreplex._
import uncore.tilelink._
import uncore.coherence._
import uncore.agents._
import uncore.devices.NTiles
import uncore.util.CacheBlockBytes

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
  case L2DirectoryRepresentation => new NullRepresentation(site(NTiles))

  case CacheBlockOffsetBits => chisel3.util.log2Up(site(CacheBlockBytes))

  case TLKey("L1toL2") =>
    val useMEI = site(NTiles) <= 1
    TileLinkParameters(
      coherencePolicy = (
        if (useMEI) new MEICoherence(site(L2DirectoryRepresentation))
        else new MESICoherence(site(L2DirectoryRepresentation))),
      nManagers = site(BankedL2Config).nBanks + 1 /* MMIO */,
      nCachingClients = 1,
      nCachelessClients = 1,
      maxClientXacts = List(
        // L1 cache
        site(DCacheKey).nMSHRs + 1 /* IOMSHR */,
        // RoCC
        if (site(BuildRoCC).isEmpty) 1 else site(RoccMaxTaggedMemXacts)).max,
      maxClientsPerPort = if (site(BuildRoCC).isEmpty) 1 else 2,
      maxManagerXacts = site(NAcquireTransactors) + 2,
      dataBeats = (8 * site(CacheBlockBytes)) / site(XLen),
      dataBits = (8 * site(CacheBlockBytes))
    )

  case rocket.UseDebug => false
})

class DefaultExampleConfig extends Config(new MidasTopConfig ++ new rocketchip.BaseConfig)
/*
class SmallBOOMConfig extends Config(new NoBrPred ++ new MidasTopConfig ++ new boom.SmallBOOMConfig)

class NoBrPred extends Config((site, here, up) => {
  case boom.EnableBranchPredictor => false
})
*/
class RocketChip1GExtMem extends Config(new rocketchip.WithExtMemSize(0x40000000L) ++ new DefaultExampleConfig)
class RocketChip2GExtMem extends Config(new rocketchip.WithExtMemSize(0x80000000L) ++ new DefaultExampleConfig)
