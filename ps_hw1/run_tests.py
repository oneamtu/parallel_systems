#!/usr/bin/env python3
import os
from subprocess import check_output
import re
from time import sleep
import pickle

ALGO_MAP = {
    0: "parallel_block_sequential_sum",
    1: "parallel_block_parallel_sum",
    2: "parallel_tree_sum",
}

def run_check():
    # must include 0
    THREADS = [0, 2, 6, 15]
    LOOPS = [10]
    INPUTS = ["seq_64_test.txt", "seq_63_test.txt", "8k.txt"]
    ALGOS = [0, 1, 2]
    OPTS = ["", "-s"]

    print("Running tests..")

    for inp in INPUTS:
        for loop in LOOPS:
            for opt in OPTS:
                for algo in ALGOS:
                    for thr in THREADS:
                        cmd = "./bin/prefix_scan -o temp/temp-{}.txt -n {} -i tests/{} -l {} -a {} {}".format(
                            thr, thr, inp, loop, algo, opt)
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

def run_exp_1(loop, spin):
    THREADS = [2 * i for i in range(0, 17)]
    #  THREADS = [2 * i for i in range(0, 2)]
    #  LOOPS = [1]
    INPUTS = ["seq_64_test.txt", "1k.txt", "8k.txt", "16k.txt"]
    ALGOS = [0, 1, 2]

    print("Running experiment 1..")

    csvs = []
    for inp in INPUTS:
        for algo in ALGOS:
            csv = [ALGO_MAP[algo], "{}/{}".format(inp, loop)]

            for thr in THREADS:
                time = 0

                for i in range(0, 4):
                    cmd = "./bin/prefix_scan -o temp.txt -n {} -i tests/{} -l {} -a {} {}".format(
                        thr, inp, loop, algo, spin)
                    out = check_output(cmd, shell=True).decode("ascii")
                    m = re.search("time: (.*)", out)
                    if m is not None:
                        time = time + int(m.group(1))

                csv.append(time/4)

            csvs.append(csv)
            sleep(0.5)

    header = ["algorithm", "input"] + [x for x in THREADS]

    pickle.dump([header] + csvs, open("results/exp_1-{}{}.pickle".format(loop, spin), 'wb'))

    exp_1_file = open("results/exp_1-{}{}.csv".format(loop, spin), "w")
    exp_1_file.write(", ".join(header))
    exp_1_file.write("\n")
    for csv in csvs:
        exp_1_file.write(", ".join(csv))
        exp_1_file.write("\n")

def run_exp_2(spin):
    THREADS = [0, 8]
    LOOPS = [10, 50, 100, 500, 1000, 5000, 10000, 50000, 100000]
    #  LOOPS = [10, 100, 1000, 10000]
    #  THREADS = [2 * i for i in range(0, 2)]
    #  LOOPS = [1]
    INPUTS = ["16k.txt"]
    ALGOS = [0, 1, 2]

    print("Running experiment 2..")

    csvs = []
    for inp in INPUTS:
        for algo in ALGOS:
            for thr in THREADS:
                csv = [ALGO_MAP[algo], "{}/{}".format(inp, thr)]

                for loop in LOOPS:
                    time = 0

                    for i in range(0, 4):
                        cmd = "./bin/prefix_scan -o temp.txt -n {} -i tests/{} -l {} -a {}  {}".format(
                            thr, inp, loop, algo, spin)
                        out = check_output(cmd, shell=True).decode("ascii")
                        m = re.search("time: (.*)", out)
                        if m is not None:
                            time = time + int(m.group(1))

                    csv.append(time)
                csvs.append(csv)
                sleep(0.5)

    header = ["algorithm", "input"] + [loop for loop in LOOPS]

    pickle.dump([header] + csvs, open("results/exp_2{}.pickle".format(spin), 'wb'))

    exp_1_file = open("results/exp_2{}.csv".format(spin), "w")
    exp_1_file.write(", ".join(header))
    exp_1_file.write("\n")
    for csv in csvs:
        exp_1_file.write(", ".join(csv))
        exp_1_file.write("\n")

run_check()
run_exp_1(10000, "")
run_exp_1(10, "")
run_exp_2("")
run_exp_1(10, "-s")
run_exp_2("-s")
