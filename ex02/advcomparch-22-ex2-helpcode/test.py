#! /usr/bin/env python

import matplotlib
# matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

labels = [
    "Static AlwaysTaken",
    "Static BTFNT",
    "Nbit-8K-4",
    "Pentium-M",
    "Local-History #1",
    "Local-History #2",
    "Global-History #1",
    "Global-History #2",
    "Global-History #3",
    "Global-History #4",
    "Alpha 21264",
    "Tournament #1",
    "Tournament #2",
    "Tournament #3",
    "Tournament #4",
]
values = [
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15
]

together = list(zip(labels, values))

together = sorted(together, key=lambda t: -t[1])

labels, values = list(zip(*together))

plt.barh(labels, values)
plt.show()