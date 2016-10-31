package MidasTop

import rocketchip._
import TestGeneration._
import diplomacy.LazyModule
import strober.{StroberCompiler, EnableSnapshot}
import cde.{Config, Parameters}
import scala.concurrent.{Future, Await, ExecutionContext}
import scala.sys.process.stringSeqToProcess
import scala.reflect.ClassTag
import java.io.{File, FileWriter}

abstract class MidasTopTestSuite(
    config: Config,
    latency: Int = 8,
    N: Int = 10) extends org.scalatest.FlatSpec with HasTestSuites {
  import scala.concurrent.duration._
  import ExecutionContext.Implicits.global

  implicit val p = Parameters.root((new strober.ZynqConfig).toInstance) /* alter (
    Map(EnableSnapshot -> true)) */
  // implicit val p = Parameters.root((new ZynqConfigWithMemModel).toInstance)

  lazy val param = Parameters.root(config.toInstance)
  lazy val design = LazyModule(new MidasTop(param))

  lazy val designName = design.getClass.getSimpleName
  lazy val configName = config.getClass.getSimpleName
  lazy val makeArgs = Seq(s"DESIGN=$designName", s"CONFIG=$configName")

  val genDir = new File("generated-src") ; genDir.mkdirs
  val outDir = new File("output") ; outDir.mkdirs

  // StroberCompiler(design.module, new File(genDir, configName))
  if (p(EnableSnapshot)) strober.replay.Compiler(
    LazyModule(new MidasTop(param)).module, new File(genDir, configName))
  addTestSuites(param)

  val makefrag = new FileWriter(new File(new File(genDir, configName), "MidasTop.d"))
  makefrag write generateMakefrag
  makefrag.close

  def makeTest(backend: String, name: String, debug: Boolean) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"${dir}/${name}.%s".format(
      if (debug) "vpd" else "out"), s"EMUL=$backend") ++ makeArgs).!
  }

  def makeReplay(backend: String, name: String) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"${dir}/${name}-replay.vpd", s"EMUL=$backend") ++ makeArgs).!
  }

  def runSuite(backend: String, debug: Boolean = false)(suite: RocketTestSuite) {
    assert((Seq("make", s"$backend%s".format(
      if (debug) "-debug" else "")) ++ makeArgs).! == 0) // compile emulators
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
    if (p(EnableSnapshot)) {
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

  // asmSuites.values foreach runSuite("verilator")
  // asmSuites.values foreach runSuite("vcs")
  bmarkSuites.values foreach runSuite("verilator")
  bmarkSuites.values foreach runSuite("vcs")
  regressionSuites.values foreach runSuite("verilator")
  regressionSuites.values foreach runSuite("vcs")
}

class DefaultExampleTests extends MidasTopTestSuite(new DefaultExampleConfig)
