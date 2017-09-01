##################
#   Parameters   #
##################

PROJECT ?= midas.top
DESIGN ?= MidasTop

TARGET_PROJECT ?= midas.top
TARGET_CONFIG ?= RocketChip2GExtMem
# TARGET_CONFIG ?= DefaultBOOM2GExtMem
# TARGET_CONFIG ?= SmallBOOM2GExtMem

PLATFORM ?= zynq
#PLATFORM ?= f1
PLATFORM_PROJECT ?= midas.top
ifeq ($(PLATFORM), f1)
PLATFORM_CONFIG ?= F1Config
else
PLATFORM_CONFIG ?= ZynqConfig
# PLATFORM_CONFIG ?= ZynqConfigWithMemModel
endif

STROBER ?=
MACRO_LIB ?=
SAMPLE ?=
DRIVER ?=

sample = $(if $(SAMPLE),$(abspath $(SAMPLE)),$(DESIGN).sample)
benchmark = $(notdir $(basename $(sample)))

# Additional argument passed to VCS/verilator simulations
EMUL ?= verilator

# CML arguments used in RTL simulation
include Makefrag-args

base_dir = $(abspath .)
simif_dir = $(base_dir)/midas/src/main/cc
driver_dir = $(base_dir)/src/main/cc
generated_dir ?= $(base_dir)/generated-src/$(PLATFORM)/$(TARGET_CONFIG)
output_dir ?= $(base_dir)/output/$(PLATFORM)/$(TARGET_CONFIG)

driver_h = $(shell find $(driver_dir) -name ".h")
midas_h = $(shell find $(simif_dir) -name "*.h")
midas_cc = $(shell find $(simif_dir) -name "*.cc")
emul_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_emul midas_top fesvr/midas_tsi fesvr/midas_fesvr endpoints/serial endpoints/uart))
replay_h = $(simif_dir)/sample/sample.h $(wildcard $(simif_dir)/replay/*.h)
replay_cc = $(simif_dir)/sample/sample.cc $(wildcard $(simif_dir)/replay/*.cc)
emul_v = $(base_dir)/midas/src/main/verilog/emul_$(PLATFORM).v
replay_v = $(base_dir)/midas/src/main/verilog/replay.v

SBT ?= sbt
SBT_FLAGS ?= -J-Xmx2G -J-Xss8M -J-XX:MaxPermSize=256M

src_path = src/main/scala
submodules = . chisel firrtl barstools/macros midas \
	rocket-chip rocket-chip/hardfloat boom testchipip sifive-blocks
chisel_srcs = $(foreach submodule,$(submodules),$(shell find $(base_dir)/$(submodule)/$(src_path) -name "*.scala"))

shim := FPGATop

verilog = $(generated_dir)/$(shim).v
header = $(generated_dir)/$(DESIGN)-const.h

default: $(verilog)
verilog: $(verilog)
compile: $(verilog)

include Makefrag-plsi
macro_lib = $(if $(MACRO_LIB),$(technology_macro_lib),)

# Should publish firrtl first
publish: $(shell find $(base_dir)/firrtl/src/main/scala -name "*.scala")
	cd firrtl && $(SBT) $(SBT_FLAGS) publishLocal
	touch $@

$(verilog): $(chisel_srcs) publish $(macro_lib)
	$(SBT) $(SBT_FLAGS) \
	"run $(if $(STROBER),strober,midas) $(patsubst $(base_dir)/%,%,$(dir $@)) \
	$(PROJECT) $(DESIGN) $(TARGET_PROJECT) $(TARGET_CONFIG) $(PLATFORM_PROJECT) $(PLATFORM_CONFIG) $(macro_lib)"

$(header): $(verilog)

ifneq ($(filter run% %.run %.out %.vpd %.vcd,$(MAKECMDGOALS)),)
-include $(generated_dir)/$(PROJECT).d
endif

disasm := 2>
which_disasm := $(shell which spike-dasm 2> /dev/null)
ifneq ($(which_disasm),)
        disasm := 3>&1 1>&2 2>&3 | $(which_disasm) $(DISASM_EXTENSION) >
endif

######################
# Veriltor Emulation #
######################
verilator = $(generated_dir)/V$(DESIGN)
verilator_debug = $(generated_dir)/V$(DESIGN)-debug

$(verilator) $(verilator_debug): export CXXFLAGS := $(CXXFLAGS) -I$(RISCV)/include
$(verilator) $(verilator_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(verilator): $(verilog) $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
	$(MAKE) -C $(simif_dir) verilator PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(verilator_debug): $(verilog) $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
	$(MAKE) -C $(simif_dir) verilator-debug PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

verilator: $(verilator)
verilator-debug: $(verilator_debug)

######################
#   VCS Emulation    #
######################
vcs = $(generated_dir)/$(DESIGN)
vcs_debug = $(generated_dir)/$(DESIGN)-debug

$(vcs) $(vcs_debug): export CXXFLAGS := $(CXXFLAGS) -I$(VCS_HOME)/include -I$(RISCV)/include
$(vcs) $(vcs_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(vcs): $(verilog) $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(emul_v)
	$(MAKE) -C $(simif_dir) vcs PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(vcs_debug): $(verilog) $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(emul_v)
	$(MAKE) -C $(simif_dir) vcs-debug PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

vcs: $(vcs)
vcs-debug: $(vcs_debug)

######################
#  Emulation Rules   #
######################
$(output_dir)/%.run: $(output_dir)/% $(EMUL)
	cd $(dir $($(EMUL))) && \
	./$(notdir $($(EMUL))) $< +sample=$<.sample +max-cycles=$(timeout_cycles) $(SW_SIM_ARGS) \
	2> /dev/null 2> $@ && [ $$PIPESTATUS -eq 0 ]

$(output_dir)/%.out: $(output_dir)/% $(EMUL)
	cd $(dir $($(EMUL))) && \
	./$(notdir $($(EMUL))) $< +sample=$<.sample +max-cycles=$(timeout_cycles) $(SW_SIM_ARGS) \
	$(disasm) $@ && [ $$PIPESTATUS -eq 0 ]

$(output_dir)/%.vpd: $(output_dir)/% $(EMUL)-debug
	cd $(dir $($(EMUL)_debug)) && \
	./$(notdir $($(EMUL)_debug)) $< +sample=$<.sample +waveform=$@ +max-cycles=$(timeout_cycles) $(SW_SIM_ARGS) \
	$(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

######################
#   FPGA Simulation  #
######################
$(output_dir)/$(DESIGN).chain: $(verilog)
	mkdir -p $(output_dir)
	$(if $(wildcard $(generated_dir)/$(DESIGN).chain),cp $(generated_dir)/$(DESIGN).chain $@,)

$(PLATFORM) = $(output_dir)/$(DESIGN)-$(PLATFORM)

$(PLATFORM): $($(PLATFORM)) $(output_dir)/$(DESIGN).chain

# Compile Frontend Server
host = $(if $(filter zynq, $(PLATFORM)),arm-xilinx-linux-gnueabi,)
fesvr_dir = $(base_dir)/riscv-fesvr

$(fesvr_dir)/build/libfesvr.so: $(wildcard $(fesvr_dir)/fesvr/*.cc) $(wildcard $(fesvr_dir)/fesvr/*.h)
	mkdir -p $(fesvr_dir)/build
	cd $(fesvr_dir)/build && $(fesvr_dir)/configure --host=$(host)
	$(MAKE) -C $(fesvr_dir)/build

$(output_dir)/libfesvr.so: $(fesvr_dir)/build/libfesvr.so
	mkdir -p $(output_dir)/build
	cp $< $@

ifeq ($(PLATFORM),zynq)
# Compile Driver
zynq_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_zynq midas_top fesvr/midas_tsi fesvr/midas_fesvr endpoints/serial endpoints/uart))

$(zynq): $(verilog) $(header) $(zynq_cc) $(driver_h) $(midas_cc) $(midas_h) $(output_dir)/libfesvr.so
	mkdir -p $(output_dir)/build
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) zynq PLATFORM=zynq DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) DRIVER="$(zynq_cc)" \
	CXX="$(host)-g++" \
	CXXFLAGS="$(CXXFLAGS) -I$(RISCV)/include" \
	LDFLAGS="$(LDFLAGS) -L$(output_dir) -lfesvr -Wl,-rpath,/usr/local/lib"

# Generate Bitstream
BOARD     ?= zc706_MIG
board_dir := $(base_dir)/midas-$(PLATFORM)/$(BOARD)
boot_bin := fpga-images-$(BOARD)/boot.bin
proj_name = midas_$(BOARD)_$(TARGET_CONFIG)

$(board_dir)/src/verilog/$(TARGET_CONFIG)/$(shim).v: $(verilog)
	$(MAKE) -C $(board_dir) clean DESIGN=$(TARGET_CONFIG)
	mkdir -p $(dir $@)
	cp $< $@

$(output_dir)/boot.bin: $(board_dir)/src/verilog/$(TARGET_CONFIG)/$(shim).v
	mkdir -p $(output_dir)
	$(MAKE) -C $(board_dir) $(boot_bin) DESIGN=$(TARGET_CONFIG)
	cp $(board_dir)/$(boot_bin) $@

$(output_dir)/midas_wrapper.bit: $(board_dir)/src/verilog/$(TARGET_CONFIG)/$(shim).v
	mkdir -p $(output_dir)
	$(MAKE) -C $(board_dir) bitstream DESIGN=$(TARGET_CONFIG)
	cp $(board_dir)/$(proj_name)/$(proj_name).runs/impl_1/midas_wrapper.bit $@

bitstream: $(output_dir)/midas_wrapper.bit
fpga: $(output_dir)/boot.bin

# This omits the boot.bin for use on the cluster
endif

ifeq ($(PLATFORM),f1)
# Compile Driver
f1_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_f1 midas_top fesvr/midas_tsi fesvr/midas_fesvr endpoints/serial endpoints/uart))

$(f1): $(verilog) $(header) $(f1_cc) $(driver_h) $(midas_cc) $(midas_h) $(output_dir)/libfesvr.so
	mkdir -p $(output_dir)/build
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) f1 PLATFORM=f1 DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) DRIVER="$(f1_cc)" \
	CXX="$(host)-g++" \
	CXXFLAGS="$(CXXFLAGS) -D SIMULATION_XSIM -I$(RISCV)/include -I$(base_dir)/riscv-fesvr" \
	LDFLAGS="$(LDFLAGS) -L$(output_dir) -lfesvr -Wl,-rpath,/usr/local/lib"

f1-fpga: $(verilog) $(header) $(f1_cc) $(driver_h) $(midas_cc) $(midas_h) $(output_dir)/libfesvr.so
	mkdir -p $(output_dir)/build
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) f1 PLATFORM=f1 DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) DRIVER="$(f1_cc)" \
	CXX="$(host)-g++" \
	CXXFLAGS="$(CXXFLAGS) -I$(RISCV)/include -I$(base_dir)/riscv-fesvr -I$(base_dir)/../../platforms/f1/aws-fpga/sdk/userspace/include" \
	LDFLAGS="$(LDFLAGS) -L$(output_dir) -lfesvr -lfpga_mgmt -lrt -lpthread -Wl,-rpath,/usr/local/lib"

endif

######################
#   Sample Replays   #
######################

script_dir = $(base_dir)/midas/src/main/resources/replay
replay_sample = $(script_dir)/replay-samples.py
estimate_power = $(script_dir)/estimate-power.py

$(generated_dir)/$(DESIGN).v: $(chisel_srcs) publish $(macro_lib)
	$(SBT) $(SBT_FLAGS) "run replay $(patsubst $(base_dir)/%,%,$(dir $@)) \
	$(PROJECT) $(DESIGN) $(TARGET_PROJECT) $(TARGET_CONFIG) $(PLATFORM_PROJECT) $(PLATFORM_CONFIG) $(macro_lib)"

compile-replay: $(generated_dir)/$(DESIGN).v

vcs_rtl = $(generated_dir)/$(DESIGN)-rtl
$(vcs_rtl): $(generated_dir)/$(DESIGN).v $(replay_cc) $(replay_h) $(replay_v)
	$(MAKE) -C $(simif_dir) $@ DESIGN=$(DESIGN) GEN_DIR=$(dir $<) REPLAY_BINARY=$@

vcs-rtl: $(vcs_rtl)

replay-rtl: $(vcs_rtl)
	cd $(dir $<) && ./$(notdir $<) +sample=$(sample) +verbose \
	+waveform=$(output_dir)/$(benchmark)-replay.vpd \
	$(disasm) $(output_dir)/$(benchmark)-replay.out && [ $$PIPESTATUS -eq 0 ]

$(output_dir)/%.sample: $(output_dir)/%.out

$(output_dir)/%-replay.vpd: $(vcs_rtl) $(output_dir)/%.sample
	cd $(dir $<) && ./$(notdir $<) +sample=$(word 2, $^) +verbose \
	+waveform=$@ $(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

# Find match points
fm_match = $(base_dir)/midas/src/main/resources/replay/fm-match.py
fm_macro = $(base_dir)/midas/src/main/resources/replay/fm-macro.py
macros = $(generated_dir)/$(DESIGN).macros.v
match_file = $(generated_dir)/$(DESIGN).match
# FIXME: --impl $(TECHNOLOGY_VERILOG_SIMULATION_FILES)
$(match_file): $(syn_match_points) $(syn_svf_txt) $(fm_match) $(fm_macro)
	cd $(generated_dir) && \
	$(fm_match) --match $@ --report $< --svf $(word 2, $^) && \
	$(fm_macro) --match $@ --paths $(generated_dir)/$(DESIGN).macros.path \
	--ref $(macros) --impl $(macros)

match: $(match_file)

# FIMXE: formality doesn't match large srams..
TECHNOLOGY_VERILOG_SIMULATION_FILES := $(filter-out %sram.v, $(TECHNOLOGY_VERILOG_SIMULATION_FILES))

replay-rtl-pwr: $(vcs_rtl) $(syn_verilog)
	$(replay_sample) --sim $< --sample $(sample) --dir $(output_dir)/$@
	$(estimate_power) --design $(DESIGN) --sample $(sample) --make syn-pwr PRIMETIME_RTL_TRACE=1 \
	--output-dir $(output_dir)/$@ --obj-dir $(OBJ_SYN_DIR) --trace-dir $(TRACE_SYN_DIR)

# Replay with Post-Synthesis
# FIXME: TARGET_VERILOG="$< $(TECHNOLOGY_VERILOG_SIMULATION_FILES)" 
$(generated_dir)/$(DESIGN)-syn: $(syn_verilog) $(test_bench) $(replay_cc) $(replay_h)
	$(MAKE) -C $(simif_dir) $@ DESIGN=$(DESIGN) GEN_DIR=$(generated_dir) \
	TARGET_VERILOG="$< $(macros) $(TECHNOLOGY_VERILOG_SIMULATION_FILES)" \
	REPLAY_BINARY=$@ VCS_FLAGS="+nospecify +evalorder"

vcs-syn: $(generated_dir)/$(DESIGN)-syn

replay-syn: $(generated_dir)/$(DESIGN)-syn $(match_file)
	$(replay_sample) --sim $< --match $(word 2, $^) --sample $(sample) --dir $(output_dir)/$@
	$(estimate_power) --design $(DESIGN) --sample $(sample) --make syn-pwr \
	--output-dir $(output_dir)/$@ --obj-dir $(OBJ_SYN_DIR) --trace-dir $(TRACE_SYN_DIR)

mostlyclean:
	rm -rf $(verilator) $(verilator_debug) $(vcs) $(vcs_debug) $($(PLATFORM)) $(output_dir)

clean:
	rm -rf $(generated_dir) $(output_dir)

.PHONY: verilog compile verilator verilator-debug vcs vcs-debug
.PHONY: $(PLATFORM) f1-fpga fpga mostlyclean clean
.PHONY: vcs-rtl replay-rtl vcs-syn replay-syn vcs-par replay-par

.PRECIOUS: $(output_dir)/%.vpd $(output_dir)/%.out $(output_dir)/%.run
