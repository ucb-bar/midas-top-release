CXX=riscv64-unknown-linux-gnu-g++
CXXFLAGS=-static -O2

hpmcounters/hpmcounters/hpmcounters: hpmcounters/hpm_counters.cxx
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<
