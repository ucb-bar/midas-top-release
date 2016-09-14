import sbt._
import Keys._
import java.nio.file.{Paths, Files}

object MidasBuild extends Build {
  override lazy val settings = super.settings ++ Seq(
    scalaVersion := "2.11.7",
    scalacOptions ++= Seq("-deprecation","-unchecked")
  )
  val targetDir = "rocket-chip"

  // Default to a top level submodule if it exists
  def lookUpProject(name: String) = if(Files.exists(Paths.get(name))) file(name) else file(s"$targetDir/$name")

  lazy val chisel    = project in lookUpProject("chisel3")
  lazy val firrtl    = project in lookUpProject("firrtl")
  lazy val cde       = project in lookUpProject("context-dependent-environments") dependsOn chisel
  lazy val interp    = project in lookUpProject("firrtl-interpreter") dependsOn firrtl
  lazy val testers   = project in lookUpProject("chisel-testers") dependsOn (chisel, interp)
  // rocket has a ton of useful library components (notably junctions), and
  // will need to be a part of any midas project, regardless of whether or not
  // it is the target design

  // TODO: look this up through rocketchip
  lazy val hardfloat = project in file("rocket-chip/hardfloat") dependsOn (chisel)

  lazy val rocket    = project in file("rocket-chip") dependsOn (chisel, cde, hardfloat)
  lazy val strober   = project dependsOn (rocket, testers)
  lazy val root      = project in file(".") settings (settings:_*) dependsOn (
    strober, rocket % "compile->compile;test->test")
}
