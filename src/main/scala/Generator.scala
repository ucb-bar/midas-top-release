
package MidasTop

import Chisel._
import cde._
import rocketchip._
import uncore.tilelink2.LazyModule
import strober.{StroberCompiler, ZynqShim, SimWrapper}
import java.io.File

class MidasTop(p: Parameters) extends ExampleTop(p) {
  override lazy val module = new ExampleTopModule(p, this, new ExampleTopBundle(p, _))
}

case class MidasParsedInputNames(
    hostPlatform: String,
    hostPlatformConfig: String)

trait HasMidasGeneratorUtilites extends HasGeneratorUtilities {
  def getGenerator(project:String, className: String, params: Parameters) =
    LazyModule(Class.forName(s"$project.$className")
      .getConstructor(classOf[cde.Parameters])
      .newInstance(params)
      .asInstanceOf[LazyModule]).module

  def getConfig(project: String, configs: String): Config = {
    getConfig(ParsedInputNames(targetDir = "", topProject = "", topModuleClass = "",
      configProject = project, configs = configs))
  }
  def getParameters(project: String, configs:String): Parameters =
    getParameters(getConfig(project, configs))
}


trait Generator extends App with HasMidasGeneratorUtilites {
  lazy val (targetNames: ParsedInputNames, midasNames: MidasParsedInputNames) = {
    require(args.size == 7, "Usage: sbt> " + 
      "run TargetDir TopModuleProjectName TopModuleName ConfigProjectName ConfigNameString HostPlatformName HostPlatformConfig")
    val targetNames = ParsedInputNames(
      targetDir = args(0),
      topProject = args(1),
      topModuleClass = args(2),
      configProject = args(3),
      configs = args(4))
    val midasNames = MidasParsedInputNames(
      hostPlatform = args(5),
      hostPlatformConfig = args(6)
    )
    (targetNames, midasNames)
  }

  lazy val targetParams = getParameters(targetNames)
  lazy val targetGenerator = getGenerator(targetNames.topProject, targetNames.topModuleClass, targetParams)
  lazy val hostParams = getParameters("MidasTop", midasNames.hostPlatformConfig)
}

object MidasTopGenerator extends Generator {
  val testDir = new File(targetNames.targetDir)
  val sArgs = Array("--targetDir", targetNames.targetDir)

  //TODO: Do this better..
  midasNames.hostPlatform match {
    case "Sim" => StroberCompiler compile (
      sArgs,
      SimWrapper(targetGenerator)(hostParams),
      false // snapshot
    )
    case "Zynq" => StroberCompiler compile (
      sArgs,
      ZynqShim(targetGenerator)(hostParams),
      false // snapshot
    )
  }
}
