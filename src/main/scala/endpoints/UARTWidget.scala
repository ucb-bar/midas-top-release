package midas
package top
package endpoints

import core._
import widgets._

import chisel3._
import chisel3.util._
import config.Parameters
import sifive.blocks.devices.uart.UARTPortIO

class SimUART extends Endpoint {
  def matchType(data: Data) = data match {
    case channel: UARTPortIO => channel.txd.dir == OUTPUT
    case _ => false
  }
  def widget(p: Parameters) = new UARTWidget()(p)
  override def widgetName = "UARTWidget"
}

class UARTWidgetIO(implicit p: Parameters) extends EndpointWidgetIO()(p) {
  val hPort = Flipped(HostPort(new UARTPortIO))
}

class UARTWidget(div: Int = 542)(implicit p: Parameters) extends EndpointWidget()(p) {
  val io = IO(new UARTWidgetIO)

  val txfifo = Module(new Queue(UInt(8.W), 16))
  val rxfifo = Module(new Queue(UInt(8.W), 16))

  val target = io.hPort.hBits
  val tFire = io.hPort.toHost.hValid && io.hPort.fromHost.hReady && io.tReset.valid
  val stall = !txfifo.io.enq.ready
  val fire = tFire && !stall
  val targetReset = fire & io.tReset.bits
  rxfifo.reset  := reset || targetReset
  txfifo.reset := reset || targetReset

  io.hPort.toHost.hReady := fire
  io.hPort.fromHost.hValid := fire
  io.tReset.ready := fire

  val sTxIdle :: sTxWait :: sTxData :: sTxBreak :: Nil = Enum(UInt(), 4)
  val txState = RegInit(sTxIdle)
  val txData = Reg(UInt(8.W))
  val (txDataIdx, txDataWrap) = Counter(txState === sTxData && fire, 8)
  val (txBaudCount, txBaudWrap) = Counter(txState === sTxWait && fire, div)
  val (txSlackCount, txSlackWrap) = Counter(txState === sTxIdle && target.txd === 0.U && fire, 4)

  switch(txState) {
    is(sTxIdle) {
      when(txSlackWrap) {
        txData  := 0.U
        txState := sTxWait
      }
    }
    is(sTxWait) {
      when(txBaudWrap) {
        txState := sTxData
      }
    }
    is(sTxData) {
      when(fire) {
        txData := txData | (target.txd << txDataIdx)
      }
      when(txDataWrap) {
        txState := Mux(target.txd === 1.U, sTxIdle, sTxBreak)
      }.elsewhen(fire) {
        txState := sTxWait
      }
    }
    is(sTxBreak) {
      when(target.txd === 1.U && fire) {
        txState := sTxIdle
      }
    }
  }

  txfifo.io.enq.bits  := txData
  txfifo.io.enq.valid := txDataWrap

  // TODO: do not handle console inputs for now
  target.rxd := UInt(1, 1)
  rxfifo.io.enq.valid := false.B

  genROReg(txfifo.io.deq.bits, "out_bits")
  genROReg(txfifo.io.deq.valid, "out_valid")
  Pulsify(genWORegInit(txfifo.io.deq.ready, "out_ready", false.B), pulseLength = 1)
  /*
  genWOReg(rxfifo.io.enq.bits, "in_bits")
  Pulsify(genWORegInit(rxfifo.io.enq.valid, "in_valid", false.B), pulseLength = 1)
  genROReg(rxfifo.io.enq.ready, "in_ready")
  */

  genROReg(!tFire, "done")
  genROReg(stall, "stall")

  genCRFile()
}