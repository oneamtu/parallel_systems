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

def run_exp_1():
    THREADS = [2 * i for i in range(0, 17)]
    LOOPS = [10000]
    #  THREADS = [2 * i for i in range(0, 2)]
    #  LOOPS = [1]
    INPUTS = ["seq_64_test.txt", "1k.txt", "8k.txt", "16k.txt"]
    ALGOS = [0, 1, 2]

    print("Running experiment 1..")

    csvs = []
    for inp in INPUTS:
        for algo in ALGOS:
            for loop in LOOPS:
                csv = [ALGO_MAP[algo], "{}/{}".format(inp, loop)]

                for thr in THREADS:

                    cmd = "./bin/prefix_scan -o temp.txt -n {} -i tests/{} -l {} -a {}".format(
                        thr, inp, loop, algo)
                    out = check_output(cmd, shell=True).decode("ascii")
                    m = re.search("time: (.*)", out)
                    if m is not None:
                        time = m.group(1)
                        csv.append(time)

                csvs.append(csv)
                sleep(0.5)

    header = ["algorithm", "input"] + [str(x) for x in THREADS]

    pickle.dump([header] + csvs, open("results/exp_1.pickle", 'wb'))
    
    exp_1_file = open("results/exp_1.csv", "w")
    exp_1_file.write(", ".join(header))
    exp_1_file.write("\n")
    for csv in csvs:
        exp_1_file.write(", ".join(csv))
        exp_1_file.write("\n")

run_check()
run_exp_1()
