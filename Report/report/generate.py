from itertools import count
from pathlib import Path
from collections import Counter

BASE = Path(__file__).parent

REPORTS = {
    "float_00_orig.rpt": r"\ref{sec:1a} & Baseline (AutoPipe)",
    "float_01_nopipe.rpt": r"\ref{sec:1a} & Baseline (NoPipe)",
    "float_02_innermost.rpt": r"\ref{sec:1bL3} & L3 Pipelining",
    "float_03_outermost.rpt": r"\ref{sec:1bL1} & L1 Pipelining",
    "float_04_L2.rpt": r"\ref{sec:1bL2} & L2 Pipelining",
}


def normalize(latency):
    if latency.endswith(" ms"):
        lat = float(latency[:-3])
    if latency.endswith(" us"):
        lat = float(latency[:-3]) / 1000
    return f"{lat:0.3f}"


def extract_latency(p):
    if not isinstance(p, Path):
        p = Path(p)

    rpt = p.read_text()
    rpt = rpt[rpt.find("+ Latency") :]
    rpt = rpt[: rpt.find("+ Detail")]
    line = rpt.strip().splitlines()[-2].strip()
    line = [c.strip() for c in line.split("|")[1:-1]]
    line[2] = normalize(line[2])
    line[3] = normalize(line[3])

    return line


def extract_utilization(p):
    if not isinstance(p, Path):
        p = Path(p)

    rpt = p.read_text()
    rpt = rpt[rpt.find("Utilization Estimates") :]
    rpt = rpt[: rpt.find("* DSP")]

    summary = rpt[: rpt.find("+ Detail:")]
    summary = summary[summary.find("Total") :]
    summary = summary.splitlines()[0]
    summary = [c.strip() for c in summary.split("|")[1:-1]]

    instance = rpt[rpt.find("* Instance:") :].strip()
    instance = instance.splitlines()[4:-3]
    instance = [l.split("|")[2].strip() for l in instance]
    instance = Counter(instance)

    assert len(instance) == 3
    assert "CONTROL_BUS_s_axi" in instance
    assert "fadd_32ns_32ns_32_5_full_dsp_1" in instance
    assert "fmul_32ns_32ns_32_4_max_dsp_1" in instance

    summary.append(str(instance["fadd_32ns_32ns_32_5_full_dsp_1"]))
    summary.append(str(instance["fmul_32ns_32ns_32_4_max_dsp_1"]))

    return summary


def update_summary():
    SUMMARY_TEMPLATE = (BASE / "summary.tpl.tex").read_text()

    LAT_LINES = []
    UTL_LINES = []
    for name, profile in REPORTS.items():
        LAT_LINES.append(
            " & ".join(
                [
                    profile,
                    *extract_latency(BASE / name),
                ]
            )
            + r" \\"
        )

        UTL_LINES.append(
            " & ".join(
                [
                    profile,
                    *extract_utilization(BASE / name),
                ]
            )
            + r" \\"
        )

    with open(BASE / "summary.tex", "w") as f:
        f.write(
            SUMMARY_TEMPLATE.replace(
                "{{LATENCY}}", "\n".join(LAT_LINES).strip()
            ).replace("{{UTILIZATION}}", "\n".join(UTL_LINES).strip())
        )


if __name__ == "__main__":
    update_summary()
    # print(extract_utilization("float_04_L2.rpt"))
