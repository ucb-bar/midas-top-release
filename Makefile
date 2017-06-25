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
#PLATFORM ?= catapult
PLATFORM_PROJECT ?= midas.top
ifeq ($(PLATFORM),catapult)
PLATFORM_CONFIG ?= CatapultConfig
else
PLATFORM_CONFIG ?= ZynqConfig
# PLATFORM_CONFIG ?= ZynqConfigWithMemModel
endif

STROBER ?=
DRIVER ?=
SAMPLE ?=
# Additional argument passed to VCS/verilator simulations
EMUL ?= verilator
SW_SIM_ARGS ?= +mm_LATENCY=10
#SW_SIM_ARGS ?= +dramsim +mm_writeLatency=20 +mm_readLatency=20 +mm_writeMaxReqs=8 +mm_readMaxReqs=8

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

SBT ?= sbt
SBT_FLAGS ?= -J-Xmx2G -J-Xss8M -J-XX:MaxPermSize=256M

src_path = src/main/scala
submodules = . chisel firrtl midas rocket-chip rocket-chip/hardfloat boom testchipip sifive-blocks
chisel_srcs = $(foreach submodule,$(submodules),$(shell find $(base_dir)/$(submodule)/$(src_path) -name "*.scala"))

shim := $(shell echo $(PLATFORM)| cut -c 1 | tr [:lower:] [:upper:])$(shell echo $(PLATFORM)| cut -c 2-)Shim

verilog = $(generated_dir)/$(shim).v
$(verilog): $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) \
	"run $(if $(STROBER),strober,midas) $(patsubst $(base_dir)/%,%,$(dir $@)) \
	$(PROJECT) $(DESIGN) $(TARGET_PROJECT) $(TARGET_CONFIG) $(PLATFORM_PROJECT) $(PLATFORM_CONFIG)"
verilog: $(verilog)

header = $(generated_dir)/$(DESIGN)-const.h
$(header): $(verilog)

compile: $(verilog)

ifneq ($(filter run% %.run %.out %.vpd %.vcd,$(MAKECMDGOALS)),)
-include $(generated_dir)/$(PROJECT).d
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
verilator = $(generated_dir)/V$(DESIGN)
verilator_debug = $(generated_dir)/V$(DESIGN)-debug

$(verilator) $(verilator_debug): export CXXFLAGS := $(CXXFLAGS) -I$(RISCV)/include
$(verilator) $(verilator_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(verilator): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
	$(MAKE) -C $(simif_dir) verilator PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(verilator_debug): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
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

$(vcs): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
	$(MAKE) -C $(simif_dir) vcs PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(vcs_debug): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h)
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
#   Smaple Replays   #
######################

$(generated_dir)/$(DESIGN).v: $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) "run replay $(patsubst $(base_dir)/%,%,$(dir $@)) \
	$(PROJECT) $(DESIGN) $(TARGET_PROJECT) $(TARGET_CONFIG) $(PLATFORM_PROJECT) $(PLATFORM_CONFIG)"

compile-replay: $(generated_dir)/$(DESIGN).v

vcs_replay = $(generated_dir)/$(DESIGN)-replay

$(vcs_replay): $(generated_dir)/$(DESIGN).v $(midas_cc) $(midas_h)
	$(MAKE) -C $(simif_dir) vcs-replay PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) GEN_DIR=$(dir $<)

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
$(output_dir)/$(DESIGN).chain: $(verilog)
	mkdir -p $(output_dir)
	$(if $(wildcard $(generated_dir)/$(DESIGN).chain),cp $(generated_dir)/$(DESIGN).chain $@,)

$(PLATFORM) = $(output_dir)/$(DESIGN)-$(PLATFORM)

$(PLATFORM): $($(PLATFORM)) $(output_dir)/$(DESIGN).chain

# Compile Frontend Server
host = $(if $(filter zynq, $(PLATFORM)),arm-xilinx-linux-gnueabi,)
so = $(if $(filter catapult, $(PLATFORM)),.dll,.so)
fesvr_dir = $(base_dir)/riscv-fesvr

ifeq ($(PLATFORM),catapult)
$(fesvr_dir)/build/libfesvr$(so): CXX := g++
$(fesvr_dir)/build/libfesvr$(so): CXXFLAGS :=
endif
$(fesvr_dir)/build/libfesvr$(so): $(wildcard $(fesvr_dir)/fesvr/*.cc) $(wildcard $(fesvr_dir)/fesvr/*.h)
	mkdir -p $(fesvr_dir)/build
	cd $(fesvr_dir)/build && $(fesvr_dir)/configure --host=$(host)
	$(MAKE) -C $(fesvr_dir)/build

$(output_dir)/libfesvr$(so): $(fesvr_dir)/build/libfesvr$(so)
	mkdir -p $(output_dir)/build
	cp $< $@

ifeq ($(PLATFORM),zynq)
# Compile Driver
zynq_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_zynq midas_top fesvr/midas_tsi fesvr/midas_fesvr endpoints/serial endpoints/uart))

$(zynq): $(header) $(zynq_cc) $(driver_h) $(midas_cc) $(midas_h) $(output_dir)/libfesvr$(so)
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
proj_name = midas_$(BOARD)_$(CONFIG)

$(board_dir)/src/verilog/$(CONFIG)/$(shim).v: $(verilog) $(generated_dir)/$(DESIGN).macros.v
	$(MAKE) -C $(board_dir) clean DESIGN=$(CONFIG)
	mkdir -p $(dir $@)
	cp $< $@
	cat $(word 2, $^) >> $@

$(output_dir)/boot.bin: $(board_dir)/src/verilog/$(CONFIG)/$(shim).v
	mkdir -p $(output_dir)
	$(MAKE) -C $(board_dir) $(boot_bin) DESIGN=$(CONFIG)
	cp $(board_dir)/$(boot_bin) $@

$(output_dir)/midas_wrapper.bit: $(board_dir)/src/verilog/$(CONFIG)/$(shim).v
	mkdir -p $(output_dir)
	$(MAKE) -C $(board_dir) bitstream DESIGN=$(CONFIG)
	cp $(board_dir)/$(proj_name)/$(proj_name).runs/impl_1/midas_wrapper.bit $@

bitstream: $(output_dir)/midas_wrapper.bit
fpga: $(output_dir)/boot.bin

# This omits the boot.bin for use on the cluster
endif

ifeq ($(PLATFORM),catapult)
# Compile midas-fesvr in cygwin only
fesvr_files = channel midas_fesvr mmap_fesvr
fesvr_h = $(addprefix $(driver_dir)/fesvr/, $(addsuffix .h, $(fesvr_files)))
fesvr_o = $(addprefix $(output_dir)/build/, $(addsuffix .o, $(fesvr_files)))
$(fesvr_o): $(output_dir)/build/%.o: $(driver_dir)/fesvr/%.cc $(fesvr_h)
	mkdir -p $(output_dir)/build
	g++ -I$(fesvr_dir) -std=c++11 -D__addr_t_defined -c -o $@ $<

fesvr = $(output_dir)/midas-fesvr
$(fesvr): $(fesvr_o) $(output_dir)/libfesvr$(so)
	g++ -L$(output_dir) -Wl,-rpath,$(output_dir) -lfesvr -o $@ $(fesvr_o)

fesvr: $(fesvr)

# Compile Driver
catapult_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_catapult midas_top fesvr/channel endpoints/serial endpoints/uart))

$(catapult): $(header) $(catapult_cc) $(driver_h) $(midas_cc) $(midas_h) $(fesvr) $(DRIVER)
	mkdir -p $(output_dir)/build
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) catapult PLATFORM=catapult DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) CXX=cl AR=lib \
	DRIVER="$(catapult_cc) $(DRIVER)" \
	CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
endif

mostlyclean:
	rm -rf $(verilator) $(verilator_debug) $(vcs) $(vcs_debug) $($(PLATFORM)) $(output_dir)

clean:
	rm -rf $(generated_dir) $(output_dir)

.PHONY: verilog compile verilator verilator-debug vcs vcs-debug $(PLATFORM) fpga mostlyclean clean

.PRECIOUS: $(output_dir)/%.vpd $(output_dir)/%.out $(output_dir)/%.run
