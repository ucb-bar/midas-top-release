package midas
package top

import rocketchip._
import TestGeneration._
import diplomacy.LazyModule
import cde.{Config, Parameters}
import scala.concurrent.{Future, Await, ExecutionContext}
import scala.sys.process.stringSeqToProcess
import scala.reflect.ClassTag
import java.io.{File, FileWriter}

abstract class MidasTopTestSuite(
    config: Config,
    snapshot: Boolean = false,
    memmodel: Boolean = false,
    latency: Int = 8,
    N: Int = 10) extends org.scalatest.FlatSpec with HasTestSuites {
  import scala.concurrent.duration._
  import ExecutionContext.Implicits.global

  implicit val p = Parameters.root(memmodel match {
    case true => (new ZynqConfigWithMemModel).toInstance
    case false => (new midas.ZynqConfig).toInstance
  }) alter (Map(midas.EnableSnapshot -> snapshot))

  lazy val param = Parameters.root(config.toInstance)
  lazy val configName = config.getClass.getSimpleName
  lazy val makeArgs = Seq("DESIGN=MidasTop", s"CONFIG=$configName")

  val genDir = new File("generated-src", configName) ; genDir.mkdirs
  val outDir = new File("output", configName) ; outDir.mkdirs

  lazy val design = LazyModule(new MidasTop(param)).module
  val chirrtl = firrtl.Parser parse (chisel3.Driver emit (() => design))
  midas.MidasCompiler(chirrtl, design.io, genDir)
  if (p(midas.EnableSnapshot)) strober.replay.Compiler(chirrtl, design.io, genDir)
  addTestSuites(param)

  val makefrag = new FileWriter(new File(genDir, "midas.top.d"))
  makefrag write generateMakefrag
  makefrag.close

  def makeTest(backend: String, name: String, debug: Boolean) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"${dir}/${name}.%s".format(if (debug) "vpd" else "out"),
         s"EMUL=$backend", s"output_dir=$dir") ++ makeArgs).!
  }

  def makeReplay(backend: String, name: String) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"${dir}/${name}-replay.vpd",
         s"EMUL=$backend", s"output_dir=$dir") ++ makeArgs).!
  }

  def runSuite(backend: String, debug: Boolean = false)(suite: RocketTestSuite) {
    // compile emulators
    assert((Seq("make", s"$backend%s".format(if (debug) "-debug" else "")) ++ makeArgs).! == 0)
    behavior of s"${suite.makeTargetName} in $backend"
    val postfix = suite match {
      case s: BenchmarkTestSuite => ".riscv"
      case _ => ""
    }
    val results = suite.names.toSeq sliding (N, N) map { t => 
      val subresults = t map (name =>
        Future(name -> makeTest(backend, s"$name$postfix", debug)))
      Await result (Future sequence subresults, Duration.Inf)
    }
    results.flatten foreach { case (name, exitcode) =>
      it should s"pass $name" in { assert(exitcode == 0) }
    }
    if (p(midas.EnableSnapshot)) {
      assert((Seq("make", "vcs-replay") ++ makeArgs).! == 0) // compile vcs
      val replays = suite.names.toSeq sliding (N, N) map { t =>
        val subresults = t map (name =>
          Future(name -> makeReplay(backend, s"$name$postfix")))
        Await result (Future sequence subresults, Duration.Inf)
      }
      replays.flatten foreach { case (name, exitcode) =>
        it should s"replay $name in vcs" in { assert(exitcode == 0) }
      }
    }
  }
}

class DefaultExampleTests extends MidasTopTestSuite(new DefaultExampleConfig) {
  bmarkSuites.values foreach runSuite("verilator")
  bmarkSuites.values foreach runSuite("vcs", true)
  regressionSuites.values foreach runSuite("verilator")
  regressionSuites.values foreach runSuite("vcs", true)
}
class SmallBOOMTests extends MidasTopTestSuite(new SmallBOOMConfig) {
  bmarkSuites.values foreach runSuite("verilator")
  bmarkSuites.values foreach runSuite("vcs", true)
}
