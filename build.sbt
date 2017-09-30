lazy val commonSettings = Seq(
  organization := "berkeley",
  version      := "1.0",
  scalaVersion := "2.11.7",
  libraryDependencies += "org.scalatest" % "scalatest_2.11" % "2.2.6" % "test"
)

lazy val firrtl     = project 
lazy val chisel     = project 
// rocket has a ton of useful library components (notably junctions), and
// will need to be a part of any midas project, regardless of whether or not
// it is the target design
lazy val hardfloat  = project in file("rocket-chip/hardfloat") settings commonSettings dependsOn chisel
lazy val rocketchip = project in file("rocket-chip") settings commonSettings dependsOn hardfloat
lazy val sifiveip   = project in file("sifive-blocks") settings commonSettings dependsOn rocketchip
lazy val testchipip = project settings commonSettings dependsOn rocketchip
lazy val boom       = project settings commonSettings dependsOn rocketchip
lazy val mdf        = RootProject(file("barstools/mdf/scalalib"))
lazy val barstools  = project in file("barstools/macros") settings commonSettings dependsOn (chisel, mdf)
lazy val midas      = project settings commonSettings dependsOn (rocketchip, barstools)
lazy val endpoints  = project in file("midas/addons") settings commonSettings dependsOn (midas, testchipip, sifiveip)
lazy val midasmem   = project in file("midas-memory-model") settings commonSettings dependsOn midas
lazy val midastop   = (project in file(".")) settings commonSettings dependsOn (midasmem, boom, endpoints)
