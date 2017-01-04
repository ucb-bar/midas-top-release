package midas
package top

import cde.{Parameters, Config}
import dram_midas._

class WithLBPipe extends Config(
  (key, _, _) => key match {
    case MemModelKey => Some((p: Parameters) => new MidasMemModel(
      new LatencyPipeConfig(new BaseParams(maxReads = 16, maxWrites = 16)))(p))
  }
)

class ZynqConfigWithMemModel extends Config(new WithLBPipe ++ new ZynqConfig)

class DefaultExampleConfig extends Config(
  new testchipip.WithSerialAdapter ++ new rocketchip.BaseConfig)

class NoBrPred extends Config(
  (key, _, _) => key match {
    case boom.EnableBranchPredictor => false
  }
)

class SmallBOOMConfig extends Config(
  new NoBrPred ++ new testchipip.WithSerialAdapter ++ new boom.SmallBOOMConfig)
