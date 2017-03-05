package midas
package top

import rocketchip._
import rocket.{XLen, UseVM, UseAtomics, UseCompressed, FPUKey}
import diplomacy.LazyModule
import config.Parameters
import util.{GeneratorApp, ParsedInputNames}
import DefaultTestSuites._
import java.io.File

class MidasTop(implicit p: Parameters) extends BaseTop
  with PeripheryBootROM
  with PeripheryFesvr
  with PeripheryMasterAXI4Mem
  with PeripheryCounter
  with HardwiredResetVector
  with MidasPlexMaster
{
  override lazy val module = new MidasTopModule(this, () => new MidasTopBundle(this))
}

class MidasTopBundle[+L <: MidasTop](_outer: L) extends BaseTopBundle(_outer)
  with PeripheryBootROMBundle
  with PeripheryFesvrBundle
  with PeripheryMasterAXI4MemBundle
  with PeripheryCounterBundle
  with HardwiredResetVectorBundle
  with MidasPlexMasterBundle
{
  override def cloneType = (new MidasTopBundle(_outer) {
    override val mem_axi4 = outer.mem_axi4.map(_.bundleOut.cloneType).toList.headOption
  }).asInstanceOf[this.type]
}

class MidasTopModule[+L <: MidasTop, +B <: MidasTopBundle[L]](_outer: L, _io: () => B)
  extends BaseTopModule(_outer, _io)
  with PeripheryBootROMModule
  with PeripheryFesvrModule
  with PeripheryMasterAXI4MemModule
  with PeripheryCounterModule
  with HardwiredResetVectorModule
  with MidasPlexMasterModule

trait HasGenerator extends GeneratorApp {
  def getGenerator(targetNames: ParsedInputNames, params: Parameters) =
    LazyModule(Class.forName(targetNames.fullTopModuleClass)
      .getConstructor(classOf[Parameters])
      .newInstance(params)
      .asInstanceOf[LazyModule]).module

  override lazy val names: ParsedInputNames = {
    require(args.size == 7, "Usage: sbt> run [midas | strober | replay] " +
      "TargetDir TopModuleProjectName TopModuleName ConfigProjectName ConfigNameString platform")
    ParsedInputNames(
      targetDir = args(1),
      topModuleProject = args(2),
      topModuleClass = args(3),
      configProject = args(4),
      configs = args(5))
  }

  lazy val targetParams = getParameters(names)
  lazy val targetGenerator = getGenerator(names, targetParams)
}

trait HasTestSuites {
  val rv64RegrTestNames = collection.mutable.LinkedHashSet(
      "rv64ud-v-fcvt",
      "rv64ud-p-fdiv",
      "rv64ud-v-fadd",
      "rv64uf-v-fadd",
      "rv64um-v-mul",
      // "rv64mi-p-breakpoint",
      // "rv64uc-v-rvc",
      "rv64ud-v-structural",
      "rv64si-p-wfi",
      "rv64um-v-divw",
      "rv64ua-v-lrsc",
      "rv64ui-v-fence_i",
      "rv64ud-v-fcvt_w",
      "rv64uf-v-fmin",
      "rv64ui-v-sb",
      "rv64ua-v-amomax_d",
      "rv64ud-v-move",
      "rv64ud-v-fclass",
      "rv64ua-v-amoand_d",
      "rv64ua-v-amoxor_d",
      "rv64si-p-sbreak",
      "rv64ud-v-fmadd",
      "rv64uf-v-ldst",
      "rv64um-v-mulh",
      "rv64si-p-dirty")

  val rv32RegrTestNames = collection.mutable.LinkedHashSet(
      "rv32mi-p-ma_addr",
      "rv32mi-p-csr",
      "rv32ui-p-sh",
      "rv32ui-p-lh",
      "rv32uc-p-rvc",
      "rv32mi-p-sbreak",
      "rv32ui-p-sll")

  def addTestSuites(params: Parameters) {
    val xlen = params(XLen)
    val vm = params(UseVM)
    val env = if (vm) List("p","v") else List("p")
    params(FPUKey) foreach { case cfg =>
      if (xlen == 32) {
        TestGeneration.addSuites(env.map(rv32ufNoDiv))
      } else {
        TestGeneration.addSuite(rv32udBenchmarks)
        TestGeneration.addSuites(env.map(rv64ufNoDiv))
        TestGeneration.addSuites(env.map(rv64udNoDiv))
        if (cfg.divSqrt) {
          TestGeneration.addSuites(env.map(rv64uf))
          TestGeneration.addSuites(env.map(rv64ud))
        }
      }
    }
    if (params(UseAtomics))    TestGeneration.addSuites(env.map(if (xlen == 64) rv64ua else rv32ua))
    if (params(UseCompressed)) TestGeneration.addSuites(env.map(if (xlen == 64) rv64uc else rv32uc))
    val (rvi, rvu) =
      if (xlen == 64) ((if (vm) rv64i else rv64pi), rv64u)
      else            ((if (vm) rv32i else rv32pi), rv32u)

    TestGeneration.addSuites(rvi.map(_("p")))
    TestGeneration.addSuites((if (vm) List("v") else List()).flatMap(env => rvu.map(_(env))))
    TestGeneration.addSuite(benchmarks)
    TestGeneration.addSuite(new RegressionTestSuite(if (xlen == 64) rv64RegrTestNames else rv32RegrTestNames))
  }
}

object MidasTopGenerator extends HasGenerator with HasTestSuites {
  val longName = names.topModuleProject
  val testDir = new File(names.targetDir)
  def midasParams = Parameters.root((args.last match {
    case "zynq"     => new ZynqConfig
    case "catapult" => new CatapultConfig
  }).toInstance)
  // implicit val p = Parameters.root((new ZynqConfigWithMemModel).toInstance)

  override def addTestSuites = super.addTestSuites(params)
  args.head match {
    case "midas" =>
      midas.MidasCompiler(targetGenerator, testDir)(midasParams)
    case "strober" =>
      midas.MidasCompiler(targetGenerator, testDir)(
        midasParams alterPartial ({ case midas.EnableSnapshot => true }))
    case "replay" =>
      strober.replay.Compiler(targetGenerator, testDir)
  }
  generateTestSuiteMakefrags
  generateParameterDump
}
