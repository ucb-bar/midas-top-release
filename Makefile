base_dir = $(abspath .)
generated_dir = $(base_dir)/generated-src
testbench_dir = $(base_dir)/src/main/cc
simif_dir = $(base_dir)/strober/src/main/cc
output_dir = $(base_dir)/output

testbench_h = $(addprefix $(testbench_dir)/, $(addsuffix .h, midas_top tsi))
testbench_cc = $(addprefix $(testbench_dir)/, $(addsuffix .cc, midas_top_emul tsi))
simif_h = $(wildcard $(simif_dir)/*.h) $(wildcard $(simif_dir)/utils/*.cc)
simif_cc = $(wildcard $(simif_dir)/*.cc) $(wildcard $(simif_dir)/utils/*.cc)

EMUL ?= verilator
DESIGN ?= MidasTop
CONFIG ?= DefaultExampleConfig

SBT ?= sbt
SBT_FLAGS ?=

src_path = src/main/scala
submodules = . rocket-chip rocket-chip/hardfloat rocket-chip/context-dependent-environments \
	testchipip chisel firrtl strober
chisel_srcs = $(foreach submodule,$(submodules),$(shell find $(base_dir)/$(submodule)/$(src_path) -name "*.scala"))

$(generated_dir)/$(CONFIG)/ZynqShim.v: $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) "run $(dir $@) $(DESIGN) $(DESIGN) $(DESIGN) $(CONFIG)"

compile: $(generated_dir)/$(CONFIG)/ZynqShim.v

ifneq ($(filter run% %.run %.out %.vpd %.vcd,$(MAKECMDGOALS)),)
-include $(generated_dir)/$(CONFIG)/$(DESIGN).d
endif

export CXXFLAGS := $(CXXFLAGS) -I$(VCS_HOME)/include -I$(RISCV)/include
export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

timeout_cycles = 100000000
disasm := 2>
which_disasm := $(shell which spike-dasm 2> /dev/null)
ifneq ($(which_disasm),)
        disasm := 3>&1 1>&2 2>&3 | $(which_disasm) $(DISASM_EXTENSION) >
endif

verilator = $(generated_dir)/$(CONFIG)/V$(DESIGN)
verilator_debug = $(generated_dir)/$(CONFIG)/V$(DESIGN)-debug

$(verilator): $(testbench_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) verilator DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(testbench_cc)"

$(verilator_debug): $(testbench_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) verilator-debug DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(testbench_cc)"

verilator: $(verilator)
verilator-debug: $(verilator_debug)

vcs = $(generated_dir)/$(CONFIG)/$(DESIGN)
vcs_debug = $(generated_dir)/$(CONFIG)/$(DESIGN)-debug

$(vcs): $(testbench_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) vcs DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(testbench_cc)"

$(vcs_debug): $(testbench_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) vcs-debug DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(testbench_cc)"

vcs: $(vcs)
vcs-debug: $(vcs_debug)

$(output_dir)/%.run: $(output_dir)/% $(EMUL)
	$($(EMUL)) $< +max-cycles=$(timeout_cycles) 2> /dev/null 2> $@ && [ $$PIPESTATUS -eq 0 ]

$(output_dir)/%.out: $(output_dir)/% $(EMUL)
	$($(EMUL)) $< +max-cycles=$(timeout_cycles) $(disasm) $@ && [ $$PIPESTATUS -eq 0 ]

# TODO: veriltor compilation with --trace is extremley slow...
$(output_dir)/%.vpd: $(output_dir)/% $(vcs_debug)
	$(vcs)-debug $< +waveform=$@ +max-cycles=$(timeout_cycles) $(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

clean:
	rm -rf $(generated_dir) $(output_dir)

.PHONY: compile verilator clean
