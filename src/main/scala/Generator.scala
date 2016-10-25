package MidasTop

import cde._
import rocketchip._
import testchipip._
import diplomacy.LazyModule
import util.{HasGeneratorUtilities, ParsedInputNames}
import strober.StroberCompiler
import java.io.File

class MidasTop(q: Parameters) extends BaseTop(q)
    with PeripheryBootROM with PeripheryCoreplexLocalInterrupter
    with PeripherySerial with PeripheryMasterMem {
  override lazy val module = new MidasTopModule(p, this, new MidasTopBundle(p))
}

class MidasTopBundle(p: Parameters) extends BaseTopBundle(p)
    with PeripheryBootROMBundle with PeripheryCoreplexLocalInterrupterBundle
    with PeripheryMasterMemBundle with PeripherySerialBundle {
  override def cloneType = new MidasTopBundle(p).asInstanceOf[this.type]
}

class MidasTopModule[+L <: MidasTop, +B <: MidasTopBundle](p: Parameters, l: L, b: => B)
  extends BaseTopModule(p, l, b)
  with PeripheryBootROMModule with PeripheryCoreplexLocalInterrupterModule
  with PeripheryMasterMemModule with PeripherySerialModule
  with HardwiredResetVector with DirectConnection with NoDebug

trait HasMidasGeneratorUtilites extends HasGeneratorUtilities {
  def getGenerator(targetNames: ParsedInputNames, params: Parameters) =
    LazyModule(Class.forName(targetNames.fullTopModuleClass)
      .getConstructor(classOf[cde.Parameters])
      .newInstance(params)
      .asInstanceOf[LazyModule]).module
}


trait Generator extends App with HasMidasGeneratorUtilites {
  lazy val targetNames: ParsedInputNames = {
    require(args.size == 5, "Usage: sbt> " + 
      "run TargetDir TopModuleProjectName TopModuleName ConfigProjectName ConfigNameString")
    ParsedInputNames(
      targetDir = args(0),
      topModuleProject = args(1),
      topModuleClass = args(2),
      configProject = args(3),
      configs = args(4))
  }

  lazy val targetParams = getParameters(targetNames)
  lazy val targetGenerator = getGenerator(targetNames, targetParams)
}

object MidasTopGenerator extends Generator {
  val testDir = new File(targetNames.targetDir)
  implicit val p = cde.Parameters.root((new ZynqConfig).toInstance)
  StroberCompiler(targetGenerator, testDir)
}
