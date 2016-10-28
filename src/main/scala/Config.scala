package MidasTop

import cde.{Parameters, Config}
import dram_midas._

class WithLBPipe extends Config(
  (key, _, _) => key match {
    case midas_widgets.MemModelKey => Some((p: Parameters) => new MidasMemModel(
      new LatencyPipeConfig(new BaseParams(maxReads = 16, maxWrites = 16)))(p))
  }
)

class ZynqConfigWithMemModel extends Config(new WithLBPipe ++ new strober.ZynqConfig)

class DefaultExampleConfig extends Config(
  new testchipip.WithSerialAdapter ++ new rocketchip.BaseConfig)
