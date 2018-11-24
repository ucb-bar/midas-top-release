# MidasTop: RocketChip/BOOM-v1 Template for MIDAS/Strober

## Warning: This repository deomstrates FPGA-accelerated simulators and the Strober power modeling for out-dated rocketchip and BOOM-v1. See [FireSim](https://github.com/firesim/firesim) for UCB-BAR's most up-to-date MIDAS-based performance simulation environment without power modeling.

## <a name = "start"></a> Get Started

    $ git clone https://github.com/ucb-bar/midas-top.git
    $ cd midas-top
    # Initialize the submodules
    $ ./setup.sh

With the following `make` commands, you can enable RTL state snapshotting with `STROBER=1`. Also, `MACRO_LIB=1` is required for power modeling to transform technology-independent macro blocks to technology-dependent macro blocks. We also used [the RocketChip-specific implementations in MIDAS](https://github.com/ucb-bar/midas-release/tree/release/target-specific/rocketchip).


## <a name="riscvtools"></a> Build RISC-V Tools

To work with RocketChip in this repo, you need to install the correct version of RISC-V tools pointed by the RocketChip repo. Note that we will install riscv-tools supporing Privileged ISA Spec v 1.9. To help you install the RISC-V tools, `build-riscv-tools.sh` is provided.

First of all, set the `RISCV` environment variable with the path where the RISC-V tools are installed:

    $ export RISCV=<a path to the RISC-V tools>
    
Next, just run the script:

    $ ./build-riscv-tools.sh
    
It will clone `riscv-tools` in `rocket-chip`, and install all necessary tools to play with RocketChip.

## <a name="hammer"></a> Use HAMMER for the Strober Power Modeling

For power modeling, we need to use commercial CAD tools and we expect you have these tools installed in your machine. Instead of manually-written TCL scripts, the ASIC backend flow is driven by automatically-generated TCL scripts by [HAMMER](https://github.com/ucb-bar/hammer.git). To use HAMMER, we need to set proper environment variables.

First, edit `sourceme-hammer.sh` for your tool environment. Before executing `make` commands in the following steps, run:

    $ source sourceme-hammer.sh
    
If you want to use the Strober power modeling but do not correctly set the variables in `sourceme-hammer.sh`, you will see error messages, and therefore, make sure all the variables are correct in your environment. When you run HAMMER at the first time, it will install prerequisite tools, which may take hours.

## <a name = "compile"></a> Generate Verilog for FPGA Simulation

First of all, define your own chip in [src/main/scala](src/main/scala), like any other RocketChip SoC projects (if necessary).

To compile RocketChip, run:

    $ make compile [DESIGN=<your design>] [CONFIG=<your config>] [STROBER=1] [MACRO_LIB=1]

`DESIGN` and `CONFIG` are `MidasTop` and `DefaultExampleConfig` by default, respectively.

## <a name = "emulation"></a> Run Verilator/VCS tests

You may want to test FPGA simulators before running them in FPGA. To compile Verilator/VCS for testing, run:

    $ make <verilog | vcs | verilog-debug | vcs-debug> [DESIGN=<your design>] [CONFIG=<your config>]
    
To run tests, specify a `EMUL` parameter, which is verilator otherwise. For example, to run the benchmark tests:
    
    $ make -j 10 run-bmark-tests [EMUL = <verilog | vcs>] [DESIGN=<your design>] [CONFIG=<your config>]
    
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

`MidasTop-zynq`, `MidasTop.chain` should be copied to the same working directory on the ARM host of the Xilinx Zynq board, while `libfesvr.so` must be copied to `/usr/local/lib`. Also, copy `mem-args.txt` in the base directory to the board. You can tune memory timing parameters by editing `mem-args.txt`.

Finally, we are ready to run a MIDAS simulation on the FPGA. `MidasTop-zynq` has the same command line interface as `fesvr-zynq`.

    $ ./MidasTop-zynq +mm-args=mem-args.txt [+sample=<sample file>] pk <binary file>

To boot linux, [build initramfs images](#benchmarks) and run the following command:

    $ ./MidasTop-zynq +mm-args=mem-args.txt [+sample=<sample file>] <bbl-instance>

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

## <a name = "ccbench"></a> Memory System Validation with ccbench

To validate memory systems, we use the `caches` benchmark in [ccbench](https://github.com/ucb-bar/ccbench/tree/cs152-sp18). First of all, run the following command to make sure this benchmark runs in spike:

    $ make caches-spike
    
It will compile the `caches` benchmark and copy its binary to the `benchmarks` directory. Also, note that `pk` is copied, too. Once you check it's running fine in Spike, we will run it on the FPGA board to validate memory system timing.

Copy `pk`, `caches`, `caches.sh` to the board along with [the simulation driver](#fpga). Then, run the following command,

    $ ./caches.sh ./MidasTop-zynq +mm-args=mem-args.txt

It takes a while to finish and generates `caches.report.txt` in the working directory. Copy `caches.report.txt` from the board to your machine. To plot a graph from the report file, run:

    $ benchmarks/ccbench/caches/run_test.py -r caches.report.txt -o <plot name>.pdf
    
The graph will be generated in `benchmarks/ccbench/caches/plots/<plot name>.pdf`. The following graph is generated from the parameters in `mem-args.txt`:

[caches-plot.pdf](doc/images/caches-plot.pdf)

We can observe L1 cache, L2 cache, and DRAM latencies in this plot.

## <a name = "benchmarks"></a> Running Benchmarks with Linux
We support building initramfs images packing Linux kernel 4.6.2 and BusyBox along with various benchmarks. This step requires [RISC-V tools installed at `$RISCV` in your machine](#riscvtools). Once the tools are ready move to `benchmark` to build linux images:

    $ cd benchmark
    
### Linux Boot, Hello World, and HPM Counters

As a simple example, we build an image that boots linux, prints `hello world`, and collect HPM counter values. You only need to run:

    $ ./hello.py

It will create the image named `bblvmlinux-hello` and run it on Spike. If you want to run the image by yourself, run:

    $ spike bblvmlinux-hello
    
If you want to execute this on the FPGA board, copy the file into the board, and run:

    $ ./MidasTop-zynq +mm-args=mem-args.txt [+sample=<sample file>] bblvmlinux-hello

### The SPEC2006int Benchmark Suite

We also provide the build script for SPEC2006int using [Speckle](https://github.com/ccelio/Speckle). The command is as follows:

    $ ./spec2006 --input [test|ref] [--compile] [--spec_dir <your SPEC2006 source directory>] [--hpmcounters] [--run]
    
* `--input`: input type for SPEC2006int. Only `test` or `ref` is valid. Note that `ref` is **way longer** than `test`.
* `--compile`: compiles initramfs images for a given input type. For each benchmark, the file name is `bblvmlinux-<benchmark>.<input>`. With `--spec_dir` argument, it will compile SEPC2006 using Speckle before generating images. Otherwise, the script assumes the benchmarks are compiled in advance and copied to `Speckle/riscv-spec-<input>`.
* `--spec_dir`: directory containing SPEC2006 source files obtained from [Standard Performance Evaluation Corporation](https://www.spec.org/cpu2006/).
* `--hpmcounters`: include `hpmcounters` in each image. This argument will generates HPM counter traces at the end of execution.
* `--run`: execute images on Spike. You can simply run images without compilation using this script.

## Publications

* Donggyu Kim, Adam Izraelevitz, Christopher Celio, Hokeun Kim, Brian Zimmer, Yunsup Lee, Jonathan Bachrach, and Krste Asanović, **"Strober: Fast and Accurate Sample-Based Energy Simulation for Arbitrary RTL"**, International Symposium on Computer Architecture (ISCA-2016), Seoul, Korea, June 2016. ([Paper](https://dl.acm.org/citation.cfm?id=3001151), [Slides](http://isca2016.eecs.umich.edu/wp-content/uploads/2016/07/2B-2.pdf))
* Donggyu Kim, Christopher Celio, David Biancolin, Jonathan Bachrach, and Krste Asanović, **"Evaluation of RISC-V RTL with FPGA-Accelerated Simulation"**, The First Workshop on Computer Architecture Research with RISC-V (CARRV 2017), Boston, MA, USA, Oct 2017. ([Paper](doc/papers/carrv-2017.pdf), [Slides](https://carrv.github.io/2017/slides/kim-midas-carrv2017-slides.pdf))
