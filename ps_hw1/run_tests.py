#!/usr/bin/env python3
import os
from subprocess import check_output
import re
from time import sleep

def run_check():
    # must include 0
    THREADS = [0, 2, 15]
    LOOPS = [10]
    INPUTS = ["seq_64_test.txt"]

    print("Running tests..")

    for inp in INPUTS:
        for loop in LOOPS:
            for thr in THREADS:
                cmd = "./bin/prefix_scan -o temp/temp-{}.txt -n {} -i tests/{} -l {}".format(
                    thr, thr, inp, loop)
                print(cmd)
                out = check_output(cmd, shell=True).decode("ascii")

            seq_file = open("temp/temp-0.txt", "r")
            seq_output = seq_file.read()

            for t in THREADS[1:]:
                file = open("temp/temp-{}.txt".format(t), "r")
                output = file.read()
                if seq_output != output:
                    raise BaseException("Results are not consistent/correct! Check {} vs {}".format(
                        seq_file.name, file.name))

def run_exp_1():
    THREADS = [0]
    LOOPS = [10]
    INPUTS = ["seq_64_test.txt"]

    print("Running experiment 1..")

    csvs = []
    for inp in INPUTS:
        for loop in LOOPS:
            csv = ["{}/{}".format(inp, loop)]
            for thr in THREADS:
                cmd = "./bin/prefix_scan -o temp.txt -n {} -i tests/{} -l {}".format(
                    thr, inp, loop)
                out = check_output(cmd, shell=True).decode("ascii")
                m = re.search("time: (.*)", out)
                if m is not None:
                    time = m.group(1)
                    csv.append(time)

            csvs.append(csv)
            sleep(0.5)

    header = ["microseconds"] + [str(x) for x in THREADS]

    print("\n")
    print(", ".join(header))
    for csv in csvs:
        print (", ".join(csv))


run_check()
run_exp_1()
