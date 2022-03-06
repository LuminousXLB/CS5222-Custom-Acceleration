#!/usr/bin/env python3

from cProfile import label
from click import style
import pandas as pd
from pathlib import Path
from matplotlib import pyplot as plt

summary = (Path(__file__).parent / "report/float-summary.tex").read_text()

lines = [line for line in summary.splitlines() if r"\ref" in line]


X = lines[: len(lines) // 2]
Y = lines[len(lines) // 2 :]


HEADERS = [
    "Profile",
    "Latency (cycles) min",
    "Latency (cycles) max",
    "Latency (ms) min",
    "Latency (ms) max",
    "Interval (cycles) min",
    "Interval (cycles) max",
    "Pipeline Type",
    "BRAM_18K",
    "DSP",
    "FF",
    "LUT",
    "URAM",
    "fadd",
    "fmul",
]

pool = []
for linex, liney in zip(X, Y):
    cellx = [cell.strip() for cell in linex.split("&")]
    celly = [cell.strip() for cell in liney.split("&")]

    assert cellx[0] == celly[0]
    assert cellx[1] == celly[1]
    celly[-1] = celly[-1].rstrip(r"\ ")

    pool.append(dict(zip(HEADERS, [*cellx[1:], *celly[2:]])))

df = pd.DataFrame(pool)

df = df[
    (df["Profile"].str.find("Amortizing") > -1)
    | (df["Profile"] == r"Partition (\texttt{dim}=2, \texttt{factor}=16)")
][["Profile", "Latency (cycles) min", "BRAM_18K"]]


df = pd.DataFrame(
    {
        "Batch": df["Profile"].apply(
            lambda c: 8 if "Partition" in c else int(c.split("=")[-1][:-1])
        ),
        "Latency": pd.to_numeric(df["Latency (cycles) min"]),
        "BRAM_18K": df["BRAM_18K"].apply(
            lambda c: int(c.split(" ")[-1].strip(r"()\%"))
        ),
    }
)

print(df)


fig, ax = plt.subplots(figsize=(8, 5), dpi=120)
bx = ax.twinx()

l1 = ax.scatter(df["Batch"], df["Latency"], marker="o", label="Latency")
l2 = bx.scatter(
    df["Batch"], df["BRAM_18K"], marker="x", color="orange", label="BRAM_18K"
)

ax.set_xscale("log", base=2)
ax.set_xlabel("Batch Size")
bx.set_xlim([8, 512])

ax.set_yscale("log", base=2)
# ax.set_ylim([2**9, 2**17])
ax.set_ylabel("Latency (cycles)")


bx.set_yscale("log", base=10)
bx.set_ylim([0, 128])
bx.set_ylabel("BRAM_18K Usage (%)")

ax.legend(
    [l1, l2],
    ["Latency", "BRAM_18K"],
    loc="upper center",
    ncol=2,
    bbox_to_anchor=(0.5, 1.12),
    fancybox=True,
)

bx.hlines(y=100, xmin=8, xmax=512, colors="orange", linestyles="dotted")

fig.tight_layout(pad=1.5)

fig.savefig("images/amortizing.png", bbox_inches="tight")
