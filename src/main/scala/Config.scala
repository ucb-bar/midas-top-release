package midas
package top

import config.{Parameters, Config}
import dram_midas._

class ZynqConfigWithMemModel extends Config(new WithLBPipe ++ new ZynqConfig)

class WithLBPipe extends Config((site, here ,up) => {
  case MemModelKey => Some((p: Parameters) => new MidasMemModel(
    new LatencyPipeConfig(new BaseParams(maxReads = 16, maxWrites = 16)))(p))
})

class NoDebug extends Config((site, here, up) => {
  case rocket.UseDebug => false
})

class DefaultExampleConfig extends Config(new NoDebug ++ new rocketchip.BaseConfig)
/*
class SmallBOOMConfig extends Config(new NoBrPred ++ new NoDebug ++ new boom.SmallBOOMConfig)

class NoBrPred extends Config((site, here, up) => {
  case boom.EnableBranchPredictor => false
})
*/
class RocketChip1GExtMem extends Config(new rocketchip.WithExtMemSize(0x40000000L) ++ new rocketchip.BaseConfig)
