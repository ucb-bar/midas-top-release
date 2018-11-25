#!/usr/bin/env python

import os
import shutil
import argparse
from subprocess import Popen, check_call

SPEC_INT = [
    "400.perlbench",
    "401.bzip2",
    "403.gcc",
    "429.mcf",
    "445.gobmk",
    "456.hmmer",
    "458.sjeng",
    "462.libquantum",
    "464.h264ref",
    "471.omnetpp",
    "473.astar",
    "483.xalancbmk"
]

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input-type", dest="input_type", type=str,
                        help='input type (test of ref)', default="test", required=False)
    parser.add_argument("-c", "--compile", dest="compile", action='store_true',
                        help='compile binaries?', default=False)
    parser.add_argument("-r", "--run", dest="run", action='store_true',
                        help='run spike simulation?', default=False)
    parser.add_argument("--spec-dir", dest="spec_dir", help='SPEC2006 source directory')
    parser.add_argument("--hpmcounters", dest="hpmcounters", action='store_true',
                        help='collect hpmcounter values?', default=False)
    args = parser.parse_args()

    assert args.input_type == 'test' or args.input_type == 'ref'

    return args

def compile_spec2006(args):
    env = os.environ.copy()
    env["PATH"] = "%s/bin:%s" % (env["RISCV"], env["PATH"])
    if args.compile and args.spec_dir:
        env["SPEC_DIR"] = args.spec_dir
        # FIXME: this is ugly
        check_call(" ".join([
            "sed",
            """'s/INPUT_TYPE=test/INPUT_TYPE=%s/g'""" % args.input_type,
            "-i", "gen_binaries.sh"
        ]), cwd="Speckle", env=env, shell=True)
        check_call([
            "./gen_binaries.sh", "--compile", "--copy"
        ], cwd="Speckle", env=env)

    if args.compile and args.hpmcounters:
        check_call(["make", "-f", "hpmcounters.mk"], env=env)

    out_dir = os.path.abspath(os.path.join("Speckle", "riscv-spec-%s" % (args.input_type)))
    bblvmlinux_files = []
    for benchmark in SPEC_INT:
        bblvmlinux = "bblvmlinux-" + benchmark + "." + args.input_type
        bblvmlinux_files.append(bblvmlinux)
        if args.compile:
            binary = benchmark.split(".")[1]
            if benchmark == "483.xalancbmk":
                binary = "Xalan"
            benchmark_dir = os.path.join(out_dir, benchmark)
            command_file = os.path.join(
                out_dir, "commands", ".".join([benchmark, args.input_type, "cmd"]))

            with open(command_file, 'r') as _f:
                lines = _f.readlines()

            lines = [line.strip() for line in lines]

            profile = os.path.join("initramfs", "profile")
            with open(profile, 'w') as _f:
                _f.write("cd /celio/%s\n" % (benchmark))
                if args.hpmcounters:
                    _f.write("/celio/hpmcounters/hpmcounters &\n")
                    _f.write("sleep 1\n")
                for line in lines:
                    _f.write("./%s %s\n" % (binary, line))
                if args.hpmcounters:
                    _f.write("killall hpmcounters\n")
                    _f.write("while pgrep hpmcounters > /dev/null; do sleep 1; done\n")
                _f.write("poweroff\n")

            os.remove(os.path.join("initramfs", "initramfs.txt"))
            dirs = " ".join([benchmark_dir] + (
                [os.path.abspath("hpmcounters/hpmcounters")] if args.hpmcounters else []))
            check_call(["make", "-C", "initramfs", "DIRNAMES=" + dirs], env=env)
            shutil.move(os.path.join("initramfs", "bblvmlinux"), bblvmlinux)

    return bblvmlinux_files

def run_spec2006(binaries):
    ps = []
    stdouts = [open(binary + ".out", 'w') for binary in binaries]
    stderrs = [open(binary + ".err", 'w') for binary in binaries]
    for binary, stdout, stderr in zip(binaries, stdouts, stderrs):
        ps.append(Popen(["spike", "./%s" % binary],
                        stdout=stdout, stderr=stderr))

    while any(p.poll() is None for p in ps):
        pass

    assert all(p.poll() == 0 for p in ps)

if __name__ == '__main__':
    args = parse_args()
    bblvmlinux_files = compile_spec2006(args)
    if args.run:
        run_spec2006(bblvmlinux_files)
