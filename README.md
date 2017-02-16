# MidasTop
An example use of midas with rocket-chip as the target design.

## <a name = "started"></a> Getting Started

    $ git clone https://github.com/ucb-bar/midas-top.git
    $ cd midas-top
    $ git submodule update --init --recursive

## <a name = "compilation"></a> Compilation
First of all, specify your target design in `src/main/scala`, like any other RocketChip SoC project(if necessary).
To compile rocket-chip in Midas or Strober, run:

    $ make compile [STROBER=1] [DESIGN=<your design>] [CONFIG=<your config>]
    
Also, this will compile rocket-chip with Midas with no snapshotting capability by default,
so give `STORBER = 1` to compile rocket-chip for sample-based energy simulation with Strober.
`DESIGN` and `CONFIG` are `MidasTop` and `DefaultExampleConfig` by default respectively,
so pass your design and config through `DESIGN` and `CONFIG`, respectively.

## <a name = "emulation"></a> Emulation
To compile emulators for testing, run:

    $ make <verilog | vcs | verilog-debug | vcs-debug> [DESIGN=<your design>] [CONFIG=<your config>]
    
To run emulation, you have to specify a `EMUL` parameter, which is verilator by default.
For example, to run the benchmark tests:
    
    $ make run-bmark-tests [EMUL = <verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    
You can also run an individual test as follows:

    # Without waveform
    $ make <abspath to root directory>/output/<test_name>.out [EMUL=<verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    # With waveform
    $ make <abspath to root directory>/output/<test_name>.vpd [EMUL=<verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
 
## <a name = "fpga"></a> FPGA Simulation
The fpga flow is largely adopted from [ucb-bar/fpga-zynq](https://github.com/ucb-bar/fpga-zynq.git), so refer to it for more information.

To build an FPGA simulator, run:

    $ make fpga [STROBE=1] [BOARD=<zc706|zynq|zybo>] [DESIGN=<your design>] [CONFIG=<your config>]

This will produce the following files to `output/<CONFIG>`.
* `boot.bin`: boot image for the board. Copy this file to the SD card.
* `midas_wrapper.bit`: bitstream to be loaded through JTAG.
The default board is the ZC706 (using the ARM's memory system). This can be overridden by setting make variable `BOARD`.

We also need to compile the simulation driver (the master program that controls the simulator's execution). To compile the driver, run:

    $ make zynq [STROBER=1] [DESIGN=<your design>] [CONFIG=<your config>]

This will produce the following files in `output/<CONFIG>`:
* `MidasTop-zynq`: driver binary file.
* `MidasTop.chain`: chain meta file. It is only generated when strober is to be used.
* `libfesvr.so`: frontend server library.

`MidasTop-zynq`, `MidasTop.chain` should be copied to the same working directory on the ARM host of the zynq host, while `libfesvr.so` must be copied to `/usr/local/lib`.
For detailed instructions, refer to [ucb-bar/fpga-zynq](https://github.com/ucb-bar/fpga-zynq.git).

Finally, we are ready to run a midas simulation on the FPGA. `MidasTop-zynq` has the same command line interface as `fesvr-zynq`.


    $ ./MidasTop-zynq pk <binary file>

To boot linux, you'll need a bbl instance with your desired payload. A ramdisk image with all of the relevant files and an appropriate init script.

    $ ./MidasTop-zynq <bbl-instance>

## <a name = "replay"></a> Sample Replays
Here, we assume that we get samples either from [emulation tests](emulation) or from [FPGA simulation](fpga)
with [the Strober compilation](compilation). First, we need to compile vcs for sample replays as follows:

    $ make vcs-replay [SAMPLE=<your sample>] [DESIGN=<your design>] [CONFIG=<your config>]
    
You can pass your samples by a `SAMPLE` prameter, and then this command will also replay your samples.

To replay samples from each emulation test, run:

    $ make <abspath to root directory>/output/<test_name>-replay.vpd [EMUL=<verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    
## <a name = "integration"></a> Integration Tests
First, add your own tests in `src/main/test` if necessary. Next, simply run:

    $ sbt test
    
This will run all integration tests in parallel. Or, you can run a single test suite as follows:

    $ sbt testOnly MidasTop.<test suite name>
