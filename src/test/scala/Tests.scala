package midas
package top

import rocketchip._
import TestGeneration._
import diplomacy.LazyModule
import config.{Config, Parameters}
import scala.concurrent.{Future, Await, ExecutionContext}
import scala.sys.process.{stringSeqToProcess, ProcessLogger}
import scala.reflect.ClassTag
import java.io.{File, FileWriter}

abstract class MidasTopTestSuite(
    platformConfig: Config,
    targetConfig: Config,
    simulationArgs: String,
    snapshot: Boolean = false,
    plsi: Boolean = false,
    lib: Option[File] = None,
    N: Int = 5) extends org.scalatest.FlatSpec with HasTestSuites {
  import scala.concurrent.duration._
  import ExecutionContext.Implicits.global

  val platformParams = Parameters.root(platformConfig.toInstance) alterPartial Map(midas.EnableSnapshot -> snapshot)
  lazy val param = Parameters.root(targetConfig.toInstance)
  lazy val platformName = platformParams(midas.Platform).toString.toLowerCase
  lazy val targetConfigName = targetConfig.getClass.getSimpleName
  lazy val platformConfigName = platformConfig.getClass.getSimpleName
  lazy val makeArgs = Seq(s"PLATFORM=$platformName", "DESIGN=MidasTop",
    s"TARGET_CONFIG=$targetConfigName", s"PLATFORM_CONFIG=$platformConfigName",
    s"SW_SIM_ARGS=${simulationArgs}")

  val genDir = new File(new File("generated-src", platformName), targetConfigName) ; genDir.mkdirs
  val outDir = new File(new File("output", platformName), targetConfigName) ; outDir.mkdirs

  val replayBackends = "rtl" +: (if (plsi) Seq("syn") else Seq())

  lazy val design = LazyModule(new MidasTop()(param)).module
  val chirrtl = firrtl.Parser parse (chisel3.Driver emit (() => design))
  lib match {
    case Some(f) if !f.exists =>
      (Seq("make", s"${f.getAbsolutePath}") ++ makeArgs).!
    case _ =>
  }
  midas.MidasCompiler(chirrtl, design.io, genDir, lib)(platformParams)
  if (platformParams(midas.EnableSnapshot))
    strober.replay.Compiler(chirrtl, design.io, genDir, lib)
  addTestSuites(param)

  val makefrag = new FileWriter(new File(genDir, "midas.top.d"))
  makefrag write generateMakefrag
  makefrag.close

  def isCmdAvailable(cmd: String) =
    Seq("which", cmd) ! ProcessLogger(_ => {}) == 0

  def runTest(backend: String, name: String, debug: Boolean) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"${dir}/${name}.%s".format(if (debug) "vpd" else "out"),
         s"EMUL=$backend", s"output_dir=$dir") ++ makeArgs).!
  }

  def runReplay(backend: String, replayBackend: String, name: String) = {
    val dir = (new File(outDir, backend)).getAbsolutePath
    (Seq("make", s"replay-$replayBackend", s"SAMPLE=${dir}/${name}.sample") ++ makeArgs).!
  }

  def runSuite(backend: String, debug: Boolean = false)(suite: RocketTestSuite) {
    // compile emulators
    behavior of s"${suite.makeTargetName} in $backend"
    if (isCmdAvailable(backend)) {
      assert((Seq("make", s"$backend%s".format(if (debug) "-debug" else "")) ++ makeArgs).! == 0)
      val postfix = suite match {
        case s: BenchmarkTestSuite => ".riscv"
        case _ => ""
      }
      val results = suite.names.toSeq sliding (N, N) map { t => 
        val subresults = t map (name =>
          Future(name -> runTest(backend, s"$name$postfix", debug)))
        Await result (Future sequence subresults, Duration.Inf)
      }
      results.flatten foreach { case (name, exitcode) =>
        it should s"pass $name" in { assert(exitcode == 0) }
      }
      replayBackends foreach { replayBackend =>
        if (platformParams(midas.EnableSnapshot) && isCmdAvailable("vcs")) {
          assert((Seq("make", s"vcs-$replayBackend") ++ makeArgs).! == 0) // compile vcs
          suite.names foreach { name =>
            it should s"replay $name in $replayBackend" in {
              assert(runReplay(backend, replayBackend, s"$name$postfix") == 0)
            }
          }
        } else {
          suite.names foreach { name =>
            ignore should s"replay $name in $backend"
          }
        }
      }
    } else {
      ignore should s"pass $backend"
    }
  }
}

abstract class RocketChip2GExtMemTests(
      platformConfig: Config, simulationArgs: String)
    extends MidasTopTestSuite(
      platformConfig,
      new RocketChip2GExtMem,
      simulationArgs,
      snapshot = true
      //, plsi = true
      //, lib = Some(new File("plsi/obj/technology/saed32/plsi-generated/all.macro_library.json"))
      )
{
  bmarkSuites.values foreach runSuite("verilator")
  bmarkSuites.values foreach runSuite("vcs", true)
  // regressionSuites.values foreach runSuite("verilator")
  // regressionSuites.values foreach runSuite("vcs", true)
}
class RocketChip2GExtMemZynqTests extends RocketChip2GExtMemTests(
  new ZynqConfig, "+dramsim +mm_MEM_LATENCY=16")
// class RocketChip2GExtMeZynqTestsWithMemMoel extends RocketChip2GExtMemTests(
//  new ZynqConfigWithMemModel)

abstract class SmallBOOM2GExtMemTests(
      platformConfig: Config, simulationArgs: String)
    extends MidasTopTestSuite(
      platformConfig,
      new SmallBOOM2GExtMem,
      simulationArgs,
      snapshot = true
      //, plsi = true
      //, lib = Some(new File("plsi/obj/technology/saed32/plsi-generated/all.macro_library.json"))
    )
{
  bmarkSuites.values foreach runSuite("verilator")
  bmarkSuites.values foreach runSuite("vcs", true)
}

class SmallBOOM2GExtMemZynqTests extends SmallBOOM2GExtMemTests(
  new ZynqConfig, "+dramsim +mm_MEM_LATENCY=16")
