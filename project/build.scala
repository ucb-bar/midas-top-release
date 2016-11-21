import sbt._
import Keys._

object MidasBuild extends Build {
  override lazy val settings = super.settings ++ Seq(
    organization := "berkeley",
    version      := "1.0",
    scalaVersion := "2.11.7",
    libraryDependencies += "org.scalatest" % "scalatest_2.11" % "2.2.6" % "test"
  )
  val defaultVersions = Map(
    "chisel3" -> "3.0-BETA-SNAPSHOT",
    "firrtl" -> "0.1-BETA-SNAPSHOT")
  lazy val subModSettings = settings ++ Seq(
    fork := true,
    javaOptions ++= (Seq("chisel3", "firrtl") map { dep: String =>
      s"-D${dep}Version=%s".format(sys.props getOrElse (s"${dep}Version", defaultVersions(dep)))
    })
  )

  lazy val firrtl     = project
  lazy val chisel     = project settings subModSettings
  // rocket has a ton of useful library components (notably junctions), and
  // will need to be a part of any midas project, regardless of whether or not
  // it is the target design
  lazy val cde        = project in file("rocket-chip/context-dependent-environments")
  lazy val hardfloat  = project in file("rocket-chip/hardfloat") dependsOn chisel
  lazy val rocket     = project in file("rocket-chip") dependsOn (cde, hardfloat)
  lazy val testchipip = project dependsOn rocket
  lazy val boom       = project dependsOn rocket
  lazy val midas      = project dependsOn (rocket, firrtl)
  lazy val midasmem   = project in file("midas-memory-model") dependsOn midas
  lazy val root       = project in file(".") settings (settings ++ Seq(
    parallelExecution in Test := false)) dependsOn (midasmem, testchipip, boom)
}
