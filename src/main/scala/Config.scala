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
import rocketchip._
import testchipip._
import boom._

class ZynqConfigWithMemModel extends Config(new WithMidasTopEndpoints ++ new WithDDR3FIFOMAS ++ new ZynqConfig)
class ZynqConfig extends Config(new WithMidasTopEndpoints ++ new midas.ZynqConfig)
class F1Config extends Config(new WithMidasTopEndpoints ++ new midas.F1Config)

class WithMidasTopEndpoints extends Config(new Config((site, here, up) => {
  case EndpointKey => up(EndpointKey) ++ core.EndpointMap(Seq(
    new endpoints.SimSerialIO,
    new endpoints.SimUART
  ))
}) ++ new WithSerialAdapter)


// Memory model configurations
class WithLBPipe extends Config((site, here ,up) => {
  case MemModelKey => Some((p: Parameters) => new MidasMemModel(
    new LatencyPipeConfig(new BaseParams(maxReads = 16, maxWrites = 16)))(p))
})

class WithDDR3FIFOMAS extends Config((_,_,_) => {
    case MemModelKey => Some((p: Parameters) => new MidasMemModel(
      new FIFOMASConfig(
        dramKey = DRAMOrganizationKey(maxBanks = 8, maxRanks = 1, maxRows = (1 << 17)),
        baseParams = new BaseParams(maxReads = 16, maxWrites = 16)))(p))
  }
)

class MidasTopConfig extends Config((site, here, up) => {
   case RocketTilesKey => up(RocketTilesKey) map (tile => tile.copy(
     icache = tile.icache map (_.copy(
       nTLBEntries = 32 // TLB reach = 32 * 4KB = 128KB
     )),
     dcache = tile.dcache map (_.copy(
       nTLBEntries = 32 // TLB reach = 32 * 4KB = 128KB
     ))
   ))
 })

class DefaultExampleConfig extends Config(new MidasTopConfig ++
  new WithSerialAdapter ++ new WithoutTLMonitors ++ new WithNBigCores(1) ++ new rocketchip.BaseConfig)
class DefaultBOOMConfig extends Config(new MidasTopConfig ++ new WithSerialAdapter ++ new boom.BOOMConfig)
class SmallBOOMConfig extends Config(new MidasTopConfig ++ new WithSerialAdapter ++ new boom.SmallBoomConfig)

class RocketChip1GExtMem extends Config(new WithExtMemSize(0x40000000L) ++ new DefaultExampleConfig)
class RocketChip2GExtMem extends Config(new WithExtMemSize(0x80000000L) ++ new DefaultExampleConfig)
class SmallBOOM1GExtMem extends Config(new WithExtMemSize(0x40000000L) ++ new SmallBOOMConfig)
class SmallBOOM2GExtMem extends Config(new WithExtMemSize(0x80000000L) ++ new SmallBOOMConfig)
class DefaultBOOM1GExtMem extends Config(new WithExtMemSize(0x40000000L) ++ new DefaultBOOMConfig)
class DefaultBOOM2GExtMem extends Config(new WithExtMemSize(0x80000000L) ++ new DefaultBOOMConfig)
