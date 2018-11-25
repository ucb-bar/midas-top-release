#!/usr/bin/env bash
cd rocket-chip/riscv-tools
git submodule update --init --recursive
./build.sh
make -C riscv-gnu-toolchain/build linux
cd ../../
