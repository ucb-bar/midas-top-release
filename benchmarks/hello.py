#!/usr/bin/env python

import os
import shutil
from subprocess import check_call

def main():
    env = os.environ.copy()
    env["PATH"] = "%s/bin:%s" % (env["RISCV"], env["PATH"])

    check_call(["make", "-C" "hello"], env=env)
    check_call(["make", "-f", "hpmcounters.mk"], env=env)

    bblvmlinux = "bblvmlinux-hello"

    profile = os.path.join("initramfs", "profile")
    with open(profile, 'w') as _f:
        _f.write("/celio/hpmcounters/hpmcounters &\n")
        _f.write("sleep 1\n")
        _f.write("/celio/hello/hello\n")
        _f.write("killall hpmcounters\n")
        _f.write("while pgrep hpmcounters > /dev/null; do sleep 1; done\n")
        _f.write("poweroff\n")

    os.remove(os.path.join("initramfs", "initramfs.txt"))
    dirs = [os.path.abspath(d) for d in ["hello/hello", "hpmcounters/hpmcounters"]]
    check_call(["make", "-C", "initramfs", "DIRNAMES=" + " ".join(dirs)], env=env)
    shutil.move(os.path.join("initramfs", "bblvmlinux"), bblvmlinux)
    check_call(["spike", bblvmlinux])

if __name__ == '__main__':
    main()
