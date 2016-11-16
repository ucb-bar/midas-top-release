# MidasTop
Top level project demonstrating the use of midas rocket-chip as an example.

## <a name = "started"></a> Getting Started

    $ git clone https://github.com/ucb-bar/midas-top.git
    $ cd midas-top
    $ git submodule update --init --recursive

## <a name = "compilation"></a> Compilation
First of all, define or edit your own design and config in `src/main/scala` if necessary.
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
To synthesize an FPGA simulator, run:

    $ make fpga [STROBE=1] [board=<zc706|zynq|zybo>] [DESIGN=<your design>] [CONFIG=<your config>]

The fpga board is by default zc706, but you can change it by passing a `board` parameter. 
After synthesis, it generates the following files to `output/<CONFIG>`.
* `boot.bin`: boot image for the board. Copy this file to the SD card.
* `midas_wrapper.bit`: bitstream to be loaded through JTAG.

The fpga flow is adopted from [ucb-bar/fpga-zynq](https://github.com/ucb-bar/fpga-zynq.git), so refer to it for more information.

We also need to generate the simulation driver to run FPGA simulation. To compile the driver, run:

    $ make zynq [STROBER=1] [DESIGN=<your design>] [CONFIG=<your config>]

Then, it gnerated the following files to `output/<CONFIG>`:
* `MidasTop-zynq`: driver binary file.
* `MidasTop.chain`: chain meta file. It is only genrated and required for strober.
* `libfesvr.so`: frontend sever library.

We need to copy `MidasTop-zynq`, `MidasTop.chain` to the same directory in the board. Also, `libfesvr.so` should be updated in `usr/local/lib` in the board.
The instructions to copy the files to the board, refer to [ucb-bar/fpga-zynq](https://github.com/ucb-bar/fpga-zynq.git).

Finally, we are ready to run FGPA simulation. `MidasTop-zynq` has the same commandline interface as `fesvr-zynq`. Thus, to run any RISC-V binaries, run:

    $ ./MidasTop-zynq <binary file>

We assume that `MidasTop-zynq` is copied to the home directory. To run `hello` in the home directory, run:

    $ ./MidasTop-zynq ./pk ./hello
    
To boot linux, first you need to get `vmlinux` from [riscv/riscv-linux](https://github.com/riscv/riscv-linux). Then, also copy it to the board, and run:

    $ ./MidasTop-zynq ./bbl ./vmlinux +disk=<disk file>

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
