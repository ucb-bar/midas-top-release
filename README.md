# MidasTop
Top level project demonstrating the use of midas rocket-chip as an example.

## <a name = "started"></a> Getting Started

    $ git clone https://github.com/ucb-bar/midas-top.git
    $ cd midas-top
    $ git submodule update --init --recursive

## <a name = "compilation"></a> Compilation
First of all, define or edit your own design and config in `src/main/scala` if necessary.
To compile rocket-chip in Midas or Strober, run:

    $ make compile [STROBER = 1] [DESIGN = <your design>] [CONFIG = <your config>]
    
Also, this will compile rocket-chip with Midas with no snapshotting capability by default,
so give `STORBER = 1` to compile rocket-chip for sample-based energy simulation with Strober.
`DESIGN` and `CONFIG` are `MidasTop` and `DefaultExampleConfig` by default respectively,
so pass your design and config through `DESIGN` and `CONFIG`, respectively.

## <a name = "emulation"></a> Emulation
To compile emulators for testing, run:

    $ make <verilog | vcs | verilog-debug | vcs-debug> [DESIGN = <your design>] [CONFIG = <your config>]
    
To run emulation, you have to specify a `EMUL` parameter, which is verilator by default.
For example, to run the benchmark tests:
    
    $ make run-bmark-tests [EMUL = <verilog | vcs>] [DESIGN = <your design>] [CONFIG = <your config>]
    
You can also run an individual test as follows:

    # Without waveform
    $ make <abspath to root directory>/output/<test_name>.out [EMUL = <verilog | vcs>] [DESIGN = <your design>] [CONFIG = <your config>]
    # With waveform
    $ make <abspath to root directory>/output/<test_name>.vpd [EMUL = <verilog | vcs>] [DESIGN = <your design>] [CONFIG = <your config>]
 
## <a name = "fpga"></a> FPGA Simulation
Coming soon.
 
## <a name = "replay"></a> Sample Replays
Here, we assume that we get samples either from [emulation tests](emulation) or from [FPGA simulation](fpga)
with [the Strober compilation](compilation). First, we need to compile vcs for sample replays as follows:

    $ make vcs-replay [DESIGN = <your design>] [CONFIG = <your config>] [SAMPLE = <your sample>]
    
You can pass your samples by a `SAMPLE` prameter, and then this command will also replay your samples.

To replay samples from each emulation test, run:

    $ make <abspath to root directory>/output/<test_name>-replay.vpd [EMUL = <verilog | vcs>] [DESIGN = <your design>] [CONFIG = <your config>]
    
## <a name = "integration"></a> Integration Tests
First, add your own tests in `src/main/test` if necessary. Next, simply run:

    $ sbt test
    
This will run all integration tests in parallel. Or, you can run a single test suite as follows:

    $ sbt testOnly MidasTop.<test suite name>
