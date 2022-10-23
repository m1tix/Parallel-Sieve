#!/usr/bin/env python3
import matplotlib.pyplot as plt
import csv


def getMean(g):
    mean = []
    inp = []
    stdev = []
    with open(g, "r") as f:
        normaldat = csv.reader(f)
        for i, line in enumerate(normaldat):
            if i != 0:
                mean.append(float(line[1]))
                inp.append(int(line[0][7:]))
                stdev.append(float(line[2]))

    return (mean, stdev, inp)


if __name__ == "__main__":
    files = {
        "data/normal.csv": ["char array odd numbers", "red"],
        "data/bitpack.csv": ["bitpack uint32", "blue"],
        "data/mult6.csv": ["char array 6 multiples", "green"],
        "../parallel/fast1.csv": ["parallel", "purple"]
    }
    for g in files:
        mean, stdev, inp = getMean(g)
        plt.errorbar(inp, mean, yerr=stdev, c=files[g][1], label=files[g][0])
    plt.title(r"Average time spent sieving $n$ numbers")
    plt.ticklabel_format(axis="both", style="sci")
    plt.xlabel(r"$n$")
    plt.ylabel(r"time ($s$)")
    plt.legend()
    plt.show()


