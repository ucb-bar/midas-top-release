base_dir = $(abspath .)
simif_dir = $(base_dir)/midas/src/main/cc
testbench_dir = $(base_dir)/src/main/cc
generated_dir ?= $(base_dir)/generated-src
output_dir ?= $(base_dir)/output

##################
#   Parameters   #
##################
EMUL ?= verilator
PROJECT ?= midas.top
DESIGN ?= MidasTop
CONFIG ?= DefaultExampleConfig
# CONFIG ?= SmallBOOMConfig
STROBER ?=
SAMPLE ?=
ARGS ?=

testbench_h = $(addprefix $(testbench_dir)/, $(addsuffix .h, midas_top tsi))
emul_cc = $(addprefix $(testbench_dir)/, $(addsuffix .cc, midas_top_emul tsi))
zynq_cc = $(addprefix $(testbench_dir)/, $(addsuffix .cc, midas_top_zynq tsi))
simif_h = $(wildcard $(simif_dir)/*.h) $(wildcard $(simif_dir)/utils/*.cc)
simif_cc = $(wildcard $(simif_dir)/*.cc) $(wildcard $(simif_dir)/utils/*.cc)

SBT ?= sbt
SBT_FLAGS ?=

src_path = src/main/scala
submodules = . rocket-chip rocket-chip/hardfloat rocket-chip/context-dependent-environments \
	testchipip boom chisel firrtl midas
chisel_srcs = $(foreach submodule,$(submodules),$(shell find $(base_dir)/$(submodule)/$(src_path) -name "*.scala"))

$(generated_dir)/$(CONFIG)/ZynqShim.v: $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) "run $(if $(STROBER),strober,midas) $(dir $@) $(PROJECT) $(DESIGN) $(PROJECT) $(CONFIG)"

compile: $(generated_dir)/$(CONFIG)/ZynqShim.v

ifneq ($(filter run% %.run %.out %.vpd %.vcd,$(MAKECMDGOALS)),)
-include $(generated_dir)/$(CONFIG)/$(PROJECT).d
endif

timeout_cycles = 100000000
disasm := 2>
which_disasm := $(shell which spike-dasm 2> /dev/null)
ifneq ($(which_disasm),)
        disasm := 3>&1 1>&2 2>&3 | $(which_disasm) $(DISASM_EXTENSION) >
endif

######################
# Veriltor Emulation #
######################
verilator = $(generated_dir)/$(CONFIG)/V$(DESIGN)
verilator_debug = $(generated_dir)/$(CONFIG)/V$(DESIGN)-debug

$(verilator) $(verilator_debug): export CXXFLAGS := $(CXXFLAGS) -I$(RISCV)/include
$(verilator) $(verilator_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(verilator): $(emul_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) verilator DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(emul_cc)"

$(verilator_debug): $(emul_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) verilator-debug DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(emul_cc)"

verilator: $(verilator)
verilator-debug: $(verilator_debug)

######################
#   VCS Emulation    #
######################
vcs = $(generated_dir)/$(CONFIG)/$(DESIGN)
vcs_debug = $(generated_dir)/$(CONFIG)/$(DESIGN)-debug

$(vcs) $(vcs_debug): export CXXFLAGS := $(CXXFLAGS) -I$(VCS_HOME)/include -I$(RISCV)/include
$(vcs) $(vcs_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(vcs): $(emul_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) vcs DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(emul_cc)"

$(vcs_debug): $(emul_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) vcs-debug DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	TESTBENCH="$(emul_cc)"

vcs: $(vcs)
vcs-debug: $(vcs_debug)

######################
#  Emulation Rules   #
######################
$(output_dir)/%.run: $(output_dir)/% $(EMUL)
	cd $(dir $($(EMUL))) && \
	./$(notdir $($(EMUL))) $< +sample=$<.sample +max-cycles=$(timeout_cycles) $(ARGS) \
	2> /dev/null 2> $@ && [ $$PIPESTATUS -eq 0 ]

$(output_dir)/%.out: $(output_dir)/% $(EMUL)
	cd $(dir $($(EMUL))) && \
	./$(notdir $($(EMUL))) $< +sample=$<.sample +max-cycles=$(timeout_cycles) $(ARGS) \
	$(disasm) $@ && [ $$PIPESTATUS -eq 0 ]

# TODO: veriltor compilation with --trace is extremley slow...
$(output_dir)/%.vpd: $(output_dir)/% $(vcs_debug)
	cd $(dir $(word 2, $^)) && \
	./$(notdir $(word 2, $^)) $< +sample=$<.sample +waveform=$@ +max-cycles=$(timeout_cycles) $(ARGS) \
	$(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

######################
#   Smaple Replays   #
######################

$(generated_dir)/$(CONFIG)/$(DESIGN).v: $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) "run replay $(dir $@) $(PROJECT) $(DESIGN) $(PROJECT) $(CONFIG)"

compile-replay: $(generated_dir)/$(CONFIG)/$(DESIGN).v

vcs_replay = $(generated_dir)/$(CONFIG)/$(DESIGN)-replay

$(vcs_replay): $(generated_dir)/$(CONFIG)/$(DESIGN).v $(simif_cc) $(simif_h)
	$(MAKE) -C $(simif_dir) vcs-replay DESIGN=$(DESIGN) GEN_DIR=$(dir $<)

vcs-replay: $(vcs_replay)
ifdef SAMPLE
	$(vcs_replay) +sample=$(abspath $(SAMPLE)) +verbose \
	+waveform=$(output_dir)/$(DESIGN)-replay.vpd \
	$(disasm) $(output_dir)/$(DESIGN)-replay.out && [ $$PIPESTATUS -eq 0 ]
endif

$(output_dir)/%.sample: $(output_dir)/%.out

$(output_dir)/%-replay.vpd: $(output_dir)/%.sample $(vcs_replay)
	$(vcs_replay) +sample=$(output_dir)/$*.sample +verbose +waveform=$@ $(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

######################
#   FPGA Simulation  #
######################

# Compile Frontend Server
host = arm-xilinx-linux-gnueabi
fesvr_dir = $(base_dir)/riscv-fesvr

$(generated_dir)/libfesvr.so: $(wildcard $(fesvr_dir)/fesvr/*.cc) $(wildcard $(fesvr_dir)/fesvr/*.h)
	mkdir -p $(dir $@)
	cd $(generated_dir) && $(fesvr_dir)/configure --host=$(host)
	$(MAKE) -C $(generated_dir)

$(output_dir)/$(CONFIG)/libfesvr.so: $(generated_dir)/libfesvr.so
	mkdir -p $(dir $@)
	cp $< $@

# Compile Driver
zynq = $(output_dir)/$(CONFIG)/$(DESIGN)-zynq

$(zynq): $(zynq_cc) $(testbench_h) $(generated_dir)/$(CONFIG)/ZynqShim.v $(simif_cc) $(simif_h) $(output_dir)/$(CONFIG)/libfesvr.so
	$(MAKE) -C $(simif_dir) zynq DESIGN=$(DESIGN) GEN_DIR=$(generated_dir)/$(CONFIG) \
	OUT_DIR=$(dir $@) TESTBENCH="$(zynq_cc)" \
	CXX="$(host)-g++" \
	CXXFLAGS="$(CXXFLAGS) -I$(RISCV)/include -DZYNQ" \
	LDFLAGS="$(LDFLAGS) -L$(output_dir)/$(CONFIG) -lfesvr -Wl,-rpath,/usr/local/lib"

$(output_dir)/$(CONFIG)/$(DESIGN).chain: $(generated_dir)/$(CONFIG)/ZynqShim.v
	if [ -a $(generated_dir)/$(CONFIG)/$(DESIGN).chain ]; then \
	  cp $(generated_dir)/$(CONFIG)/$(DESIGN).chain $@; \
	fi

zynq: $(zynq) $(output_dir)/$(CONFIG)/$(DESIGN).chain

# Generate Bitstream
board     ?= zc706
board_dir := $(base_dir)/midas-zynq/$(board)
bitstream := fpga-images-$(board)/boot.bin

$(board_dir)/src/verilog/$(CONFIG)/ZynqShim.v: $(generated_dir)/$(CONFIG)/ZynqShim.v
	$(MAKE) -C $(board_dir) clean DESIGN=$(CONFIG)
	mkdir -p $(dir $@)
	cp $< $@

$(output_dir)/$(CONFIG)/boot.bin: $(board_dir)/src/verilog/$(CONFIG)/ZynqShim.v
	mkdir -p $(dir $@)
	$(MAKE) -C $(board_dir) $(bitstream) DESIGN=$(CONFIG)
	cp $(board_dir)/$(bitstream) $@

$(output_dir)/$(CONFIG)/midas_wrapper.bit: $(board_dir)/src/verilog/$(CONFIG)/ZynqShim.v
	mkdir -p $(dir $@)
	cp -L $(board_dir)/fpga-images-$(board)/boot_image/midas_wrapper.bit $@

fpga: $(output_dir)/$(CONFIG)/boot.bin $(output_dir)/$(CONFIG)/midas_wrapper.bit

mostlyclean:
	rm -rf $(verilator) $(verilator_debug) $(vcs) $(vcs_debug) $(zynq)
	rm -rf $(output_dir)/*.run $(output_dir)/*.out $(output_dir)/*.vpd $(output_dir)/*.sample
	rm -rf $(output_dir)/$(CONFIG)

clean:
	rm -rf $(generated_dir) $(output_dir)

.PHONY: compile verilator verilator-debug vcs vcs-debug zynq fpga mostlyclean clean

.PRECIOUS: $(output_dir)/%.vpd $(output_dir)/%.out $(output_dir)/%.run
