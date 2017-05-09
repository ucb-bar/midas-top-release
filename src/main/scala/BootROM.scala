package midas
package top

import chisel3._
import diplomacy.LazyModule
import uncore.tilelink2._
import uncore.devices._
import coreplex._
import rocketchip._

trait PeripheryBootROM {
  this: TopNetwork {
    val uartConfigs: Seq[sifive.blocks.devices.uart.UARTConfig]
  } =>
  val coreplex: MidasTopPlatform

  private val bootrom_address = 0x1000
  private val bootrom_size = 0x1000

  // Currently RC emits a config string that uses a name for offchip memory
  // that does not comform to PK's expectation. So we prepend what we want.
  private lazy val configString = {
    val sb = new StringBuilder
    sb append "uart {\n"
    uartConfigs.zipWithIndex map { case (c, i) =>
      sb append "  0 {\n"
      sb append "  addr 0x%x;\n".format(c.address)
      sb append "  };\n"
    }
    sb append "};\n"
    sb append "ram {\n"
    sb append "  0 {\n"
    sb append "  addr 0x%x;\n".format(p(ExtMem).base)
    sb append "  size 0x%x;\n".format(p(ExtMem).size)
    sb append "  };\n"
    sb append "};\n"
    sb append coreplex.configString
    val configString = sb.toString

    println(s"\nBIANCOLIN'S HACK: Generated Configuration String\n${configString}")
    _root_.util.ElaborationArtefacts.add("cfg", configString)
    configString
  }

  private lazy val bootrom_contents = GenerateBootROM(p, bootrom_address, configString)
  val bootrom = LazyModule(new TLROM(bootrom_address, bootrom_size, bootrom_contents, true, peripheryBusConfig.beatBytes))
  bootrom.node := TLFragmenter(peripheryBusConfig.beatBytes, cacheBlockBytes)(peripheryBus.node)
}

trait PeripheryBootROMBundle {
  this: TopNetworkBundle {
    val outer: PeripheryBootROM
  } =>
}

trait PeripheryBootROMModule {
  this: TopNetworkModule {
    val outer: PeripheryBootROM
    val io: PeripheryBootROMBundle
  } =>
}

