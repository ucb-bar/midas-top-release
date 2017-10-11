# MidasTop
An example use of midas with rocket-chip as the target design.

## <a name = "start"></a> Get Started

    $ git clone https://github.com/ucb-bar/midas-top.git
    $ cd midas-top
    # Initialize the submodules
    $ ./setup.sh

With the following `make` commands, you can enable RTL state snapshotting with `STROBER=1`. Also, `MACRO_LIB=1` is required for power modeling to transform technology-independent macro blocks to technology-dependent macro blocks. We also used [the RocketChip-specific implementations in MIDAS](https://github.com/ucb-bar/midas-release/tree/release/target-specific/rocketchip).

## <a name = "compile"></a> Generate Verilog for FPGA Simulation

First of all, define your own chip in [src/main/scala](src/main/scala), like any other RocketChip SoC projects (if necessary).

To compile RocketChip, run:

    $ make compile [DESIGN=<your design>] [CONFIG=<your config>] [STROBER=1] [MACRO_LIB=1]

`DESIGN` and `CONFIG` are `MidasTop` and `DefaultExampleConfig` by default, respectively.

## <a name = "emulation"></a> Run Verilator/VCS tests

You may want to test FPGA simulators before running them in FPGA. To compile Verilator/VCS for testing, run:

    $ make <verilog | vcs | verilog-debug | vcs-debug> [DESIGN=<your design>] [CONFIG=<your config>]
    
To run tests, specify a `EMUL` parameter, which is verilator otherwise. For example, to run the benchmark tests:
    
    $ make run-bmark-tests [EMUL = <verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    
You can also run an individual test as follows:

    # Without waveform
    $ make <abspath to root directory>/output/<platform>/<your config>/<test_name>.out [EMUL=<verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    # With waveform
    $ make <abspath to root directory>/output/<platform>/<your config>/<test_name>.vpd [EMUL=<verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
 
## <a name = "fpga"></a> Run FPGA Simulation

### Xilinx Zynq

For the following commands, the default board is the `zc706_MIG`, but you can also select it with `BOARD` in the `make` command. The output files are generated in `output/zynq/<your config>/`.

For ZC706 with MIG, we generate bitstream `midas_wrapper.bit`, which will be loaded through JTAG:

    $ make bitstream [STROBER=1] [BOARD=<zc706_MIG>] [DESIGN=<your design>] [CONFIG=<your config>]

To generate `boot.bin`, which will be copied to the SD card, run:

    $ make fpga [STROBER=1] [BOARD=<zc706|zedboard|zybo>] [DESIGN=<your design>] [CONFIG=<your config>]

To compile the simulation driver, run:

    $ make zynq [STROBER=1] [DESIGN=<your design>] [CONFIG=<your config>]

This will produce the following files:
* `MidasTop-zynq`: driver executable.
* `MidasTop.chain`: scan chain information (Strober only)
* `libfesvr.so`: riscv-fesvr library.

`MidasTop-zynq`, `MidasTop.chain` should be copied to the same working directory on the ARM host of the Xilinx Zynq board, while `libfesvr.so` must be copied to `/usr/local/lib`.

Finally, we are ready to run a MIDAS simulation on the FPGA. `MidasTop-zynq` has the same command line interface as `fesvr-zynq`.

    $ ./MidasTop-zynq +mm_MEM_LATENCY=<DRAM latency> [+sample=<sample file>] pk <binary file>

To boot linux, you'll need a bbl instance with your desired payload. A ramdisk image with all of the relevant files and an appropriate init script.

    $ ./MidasTop-zynq +mm_MEM_LATENCY=<DRAM latency> [+sample=<sample file>] <bbl-instance>

The FPGA flow is largely adopted from [ucb-bar/fpga-zynq](https://github.com/ucb-bar/fpga-zynq.git), so refer to it for more platform-specific information.

## <a name = "replay"></a> Replay Samples in RTL/Gate-level Simulation

Once Strober energy modeling is enabled in the previous steps, random RTL state snapshots are generated at the end of simulation (`MidasTop.sample` by default). To generate Verilog for sample replays, run:

    $ make compile-replay [DESIGN=<your design>] [CONFIG=<your config>] [STROBER=1] [MACRO_LIB=1]
    
This makes sure the RTL to be replayed is the same as the RTL simulated in FPGA. To compile RTL simulation, run:

    $ make vcs-rtl [DESIGN=<your design>] [CONFIG=<your config>]
    
Sample snapshots are replayed in RTL simulation as follows:

    $ make replay-rtl SAMPLE=<sample file> [DESIGN=<your design>] [CONFIG=<your config>]

For power modeling, `MACRO_LIB=1` is required in this step as well as the previous steps. The following commands interact with [HAMMER](https://github.com/ucb-bar/hammer.git) to run proper CAD tools. For power estimation with RTL simulation, run:

    $ make replay-rtl-pwr SAMPLE=<sample file> [DESIGN=<your design>] [CONFIG=<your config>]
    
To compile gate-level simulation with a post-synthesis design, run:

    $ make vcs-syn [DESIGN=<your design>] [CONFIG=<your config>]
    
For power estimation with post-synthesis gate-level simulation, run:

    $ make replay-syn SAMPLE=<sample file> [DESIGN=<your design>] [CONFIG=<your config>]
    
## <a name = "integration"></a> Integration Tests
First, add your own tests in `src/main/test` if necessary. Launch `sbt` with:

    $ make sbt
    
For individual tests, run:

    > testOnly <test suite name>
    
For integration tests, just run:

    > test
