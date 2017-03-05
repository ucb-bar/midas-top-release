##################
#   Parameters   #
##################
PLATFORM ?= zynq
#PLATFORM ?= catapult
EMUL ?= verilator
PROJECT ?= midas.top
# PROJECT ?= simplenic
DESIGN ?= MidasTop
CONFIG ?= DefaultExampleConfig
# CONFIG ?= SmallBOOMConfig
# CONFIG ?= SimpleNicConfig
STROBER ?=
DRIVER ?=
SAMPLE ?=
ARGS ?= +dramsim

base_dir = $(abspath .)
simif_dir = $(base_dir)/midas/src/main/cc
driver_dir = $(base_dir)/src/main/cc
generated_dir = $(base_dir)/generated-src/$(PLATFORM)/$(CONFIG)
output_dir = $(base_dir)/output/$(PLATFORM)/$(CONFIG)

driver_h = $(wildcard $(driver_dir)/*.h)
emul_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_emul midas_tsi midas_top midas_fesvr serial))
midas_h = $(wildcard $(simif_dir)/*.h) $(wildcard $(simif_dir)/utils/*.h) \
	$(wildcard $(simif_dir)/endpoints/*.h)
midas_cc = $(wildcard $(simif_dir)/*.cc) $(wildcard $(simif_dir)/utils/*.cc) \
	$(wildcard $(simif_dir)/endpoints/*.cc)

SBT ?= sbt
SBT_FLAGS ?=

ifneq ($(SHELL),sh.exe)
src_path = src/main/scala
submodules = . rocket-chip rocket-chip/hardfloat boom chisel firrtl midas $(MIDASTOP_ADDONS)
chisel_srcs = $(foreach submodule,$(submodules),$(shell find $(base_dir)/$(submodule)/$(src_path) -name "*.scala"))
mkdir = mkdir -p $(1)
whitespace = "$(1)"
else
chisel_srcs = $(wildcard $(base_dir)/src/main/scala/*.scala)
mkdir = mkdir.exe -p $(patsubst C:%,%,$(1))
whitespace = $(shell echo $(strip $(1))| sed -e "s/ /\\ /g")
endif

shim := $(shell echo $(PLATFORM)| cut -c 1 | tr [:lower:] [:upper:])$(shell echo $(PLATFORM)| cut -c 2-)Shim

verilog = $(generated_dir)/$(shim).v
$(verilog): $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) \
	"run $(if $(STROBER),strober,midas) $(patsubst $(base_dir)/%,%,$(dir $@)) $(PROJECT) $(DESIGN) $(PROJECT) $(CONFIG) $(PLATFORM)"
verilog: $(verilog)

header = $(generated_dir)/$(DESIGN)-const.h
$(header): $(verilog)

compile: $(verilog)

ifneq ($(filter run% %.run %.out %.vpd %.vcd,$(MAKECMDGOALS)),)
-include $(generated_dir)/$(PROJECT).d
endif

param_file = $(generated_dir)/$(PROJECT).prm
consts_header = $(generated_dir)/$(PROJECT).h

$(param_file): $(verilog)

$(consts_header): $(param_file)
	echo "#ifndef __PARAM_CONST_H__" > $@
	echo "#define __PARAM_CONST_H__" >> $@
	sed -E 's/\(([A-Za-z0-9_]+),([A-Za-z0-9_]+)\)/#define \1 \2L/' $< >> $@
	echo "#endif // __PARAM_CONST_H__" >> $@

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

$(verilator) $(verilator_debug): export CXXFLAGS := $(CXXFLAGS) -I$(RISCV)/include -include $(consts_header)
$(verilator) $(verilator_debug): export LDFLAGS := $(LDFLAGS) -L$(RISCV)/lib -lfesvr -Wl,-rpath,$(RISCV)/lib

$(verilator): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(consts_header)
	$(MAKE) -C $(simif_dir) verilator PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(verilator_debug): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(consts_header)
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

$(vcs): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(consts_header)
	$(MAKE) -C $(simif_dir) vcs PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

$(vcs_debug): $(header) $(emul_cc) $(driver_h) $(midas_cc) $(midas_h) $(consts_header)
	$(MAKE) -C $(simif_dir) vcs-debug PLATFORM=$(PLATFORM) DESIGN=$(DESIGN) \
	GEN_DIR=$(generated_dir) DRIVER="$(emul_cc)"

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

$(output_dir)/%.vpd: $(output_dir)/% $(EMUL)-debug
	cd $(dir $($(EMUL)_debug)) && \
	./$(notdir $($(EMUL)_debug)) $< +sample=$<.sample +waveform=$@ +max-cycles=$(timeout_cycles) $(ARGS) \
	$(disasm) $(patsubst %.vpd,%.out,$@) && [ $$PIPESTATUS -eq 0 ]

######################
#   Smaple Replays   #
######################

$(generated_dir)/$(DESIGN).v: $(chisel_srcs)
	$(SBT) $(SBT_FLAGS) "run replay $(patsubst $(base_dir)/%,%,$(dir $@)) $(PROJECT) $(DESIGN) $(PROJECT) $(CONFIG)"

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
	$(call mkdir,$(output_dir))
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
	$(call mkdir,$(fesvr_dir)/build)
	cd $(fesvr_dir)/build && $(fesvr_dir)/configure --host=$(host)
	$(MAKE) -C $(fesvr_dir)/build

$(output_dir)/libfesvr$(so): $(fesvr_dir)/build/libfesvr$(so)
	$(call mkdir,$(output_dir)/build)
	cp $< $@

ifeq ($(PLATFORM),zynq)
# Compile Driver
zynq_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_zynq midas_tsi midas_top midas_fesvr serial))

$(zynq): $(header) $(zynq_cc) $(driver_h) $(midas_cc) $(midas_h) $(output_dir)/libfesvr$(so)
	$(call mkdir,$(output_dir)/build)
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) zynq PLATFORM=zynq DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) DRIVER="$(zynq_cc)" \
	CXX="$(host)-g++" \
	CXXFLAGS="$(CXXFLAGS) -I$(RISCV)/include" \
	LDFLAGS="$(LDFLAGS) -L$(output_dir) -lfesvr -Wl,-rpath,/usr/local/lib"

# Generate Bitstream
BOARD     ?= zc706
board_dir := $(base_dir)/midas-$(PLATFORM)/$(BOARD)
boot_bin := fpga-images-$(BOARD)/boot.bin
proj_name = midas_$(BOARD)_$(CONFIG)

$(board_dir)/src/verilog/$(CONFIG)/$(shim).v: $(verilog)
	$(MAKE) -C $(board_dir) clean DESIGN=$(CONFIG)
	$(call mkdir,$(dir $@))
	cp $< $@

$(output_dir)/boot.bin: $(board_dir)/src/verilog/$(CONFIG)/$(shim).v
	$(call mkdir,$(output_dir))
	$(MAKE) -C $(board_dir) $(boot_bin) DESIGN=$(CONFIG)
	cp $(board_dir)/$(boot_bin) $@

$(output_dir)/midas_wrapper.bit: $(board_dir)/src/verilog/$(CONFIG)/$(shim).v
	$(MAKE) -C $(board_dir) bitstream DESIGN=$(CONFIG)
	cp $(board_dir)/$(proj_name)/$(proj_name).runs/impl_1/midas_wrapper.bit $@

bitstream: $(output_dir)/midas_wrapper.bit
fpga: $(output_dir)/boot.bin

# This omits the boot.bin for use on the cluster
endif

ifeq ($(PLATFORM),catapult)
# Compile midas-fesvr in cygwin only
ifneq ($(SHELL),sh.exe)
fesvr_files = channel midas_fesvr mmap_fesvr
fesvr_h = $(addprefix $(driver_dir)/,       $(addsuffix .h, $(fesvr_files)))
fesvr_o = $(addprefix $(output_dir)/build/, $(addsuffix .o, $(fesvr_files)))
$(fesvr_o): $(output_dir)/build/%.o: $(driver_dir)/%.cc $(fesvr_h)
	$(call mkdir,$(output_dir)/build)
	g++ -I$(fesvr_dir) -std=c++11 -D__addr_t_defined -c -o $@ $<

fesvr = $(output_dir)/midas-fesvr
$(fesvr): $(fesvr_o) $(output_dir)/libfesvr$(so)
	g++ -L$(output_dir) -Wl,-rpath,$(output_dir) -lfesvr -o $@ $(fesvr_o)

fesvr: $(fesvr)
endif

# Compile Driver
catapult_cc = $(addprefix $(driver_dir)/, $(addsuffix .cc, \
	midas_top_catapult channel midas_top serial \
	$(if $(filter SimpleNIC,$(MIDASTOP_ADDONS)),switch,)))

$(catapult): $(header) $(catapult_cc) $(driver_h) $(midas_cc) $(midas_h) $(fesvr) $(DRIVER)
	$(call mkdir,$(output_dir)/build)
	cp $(header) $(output_dir)/build/
	$(MAKE) -C $(simif_dir) catapult PLATFORM=catapult DESIGN=$(DESIGN) \
	GEN_DIR=$(output_dir)/build OUT_DIR=$(output_dir) CXX=cl AR=lib \
	DRIVER=$(call whitespace,$(catapult_cc) $(DRIVER)) \
	CXXFLAGS=$(call whitespace,$(CXXFLAGS)) LDFLAGS=$(call whitespace,$(LDFLAGS))
endif

mostlyclean:
	rm -rf $(verilator) $(verilator_debug) $(vcs) $(vcs_debug) $($(PLATFORM)) $(output_dir)

clean:
	rm -rf $(generated_dir) $(output_dir)

.PHONY: verilog compile verilator verilator-debug vcs vcs-debug $(PLATFORM) fpga mostlyclean clean

.PRECIOUS: $(output_dir)/%.vpd $(output_dir)/%.out $(output_dir)/%.run
