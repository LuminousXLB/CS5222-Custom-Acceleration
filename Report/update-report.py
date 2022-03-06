#!/usr/bin/env python3

from pathlib import Path
from collections import Counter


FLOAT_REPORTS = {
    "1a-baseline-autopipe": r"\ref{sec:1a}                      & Baseline (AutoPipe)",
    "1a-baseline-nopipe": r"\rowcolor{rowhlt}\ref{sec:1a}       & Baseline (NoPipe)",
    "1b1-pipeline-L3": r"\ref{sec:1bL3}                          & L3 Pipelining",
    "1b2-pipeline-L2": r"\ref{sec:1bL2}                     & L2 Pipelining",
    "1b3-pipeline-L1-1WnR": r"\ref{sec:1bL1}                     & L1 Pipelining (1WnR)",
    "1b3-pipeline-L1-T2P": r"\ref{sec:1bL1}                      & L1 Pipelining (T2P)",
    # "1c0-baseline-ap-l2": r"",
    # "1c1-partition-ap-d1-f2": r"",
    # "1c1-partition-ap-d2-f2": r"",
    # "1c2-partition-ap-d2-f16": r"",
    # "1c2-partition-ap-d2-f32": r"",
    # "1c2-partition-ap-d2-f4": r"",
    # "1c2-partition-ap-d2-f8": r"",
}

# FLOAT_REPORTS = {
# "05-partition-d1-f2": r"\ref{sec:1cDim}                     & Partition (\texttt{dim}=1, \texttt{factor}=2)",
# "05-partition-d2-f2": r"\rowcolor{rowhlt}\ref{sec:1cDim}    & Partition (\texttt{dim}=2, \texttt{factor}=2)",
# "06-partition-d2-f4": r"\ref{sec:1cFac}                     & Partition (\texttt{dim}=2, \texttt{factor}=4)",
# "06-partition-d2-f8": r"\ref{sec:1cFac}                     & Partition (\texttt{dim}=2, \texttt{factor}=8)",
# "06-partition-d2-f16": r"\rowcolor{rowhlt}\ref{sec:1cFac}   & Partition (\texttt{dim}=2, \texttt{factor}=16)",
# "06-partition-d2-f32": r"\ref{sec:1cFac}                    & Partition (\texttt{dim}=2, \texttt{factor}=32)",
# }

BASE = Path(__file__).parent

TEMPLATE_DIR = BASE / "template"
SUMMARY_TEMPLATE = (TEMPLATE_DIR / "summary.tpl.tex").read_text()
LOOP_TEMPLATE = (TEMPLATE_DIR / "loop.tpl.tex").read_text()

OUTPUT_DIR = BASE / "report"


def generate_summary_table(reports, output_fn):
    def extract_latency(rpt_text):
        def normalize(latency):
            if latency.endswith(" ms"):
                lat = float(latency[:-3])
            if latency.endswith(" us"):
                lat = float(latency[:-3]) / 1000
            return f"{lat:0.3f}"

        rpt = rpt_text[rpt_text.find("+ Latency") : rpt_text.find("+ Detail")]
        line = rpt.strip().splitlines()[-2].strip()
        line = [c.strip() for c in line.split("|")[1:-1]]
        line[2] = normalize(line[2])
        line[3] = normalize(line[3])

        return line

    def extract_utilization(rpt_text):
        rpt = rpt_text[rpt_text.find("Utilization Estimates") : rpt_text.find("* DSP")]

        summary = rpt[: rpt.find("+ Detail:")]
        summary = summary[summary.find("Total") :]
        summary = summary.splitlines()[0]
        summary = [c.strip() for c in summary.split("|")[1:-1]]

        instance = rpt[rpt.find("* Instance:") :].strip()
        instance = instance.splitlines()[4:-3]
        instance = [l.split("|")[2].strip() for l in instance]
        instance = Counter(instance)

        summary.append(str(instance["fadd_32ns_32ns_32_5_full_dsp_1"]))
        del instance["fadd_32ns_32ns_32_5_full_dsp_1"]
        summary.append(str(instance["fmul_32ns_32ns_32_4_max_dsp_1"]))
        del instance["fmul_32ns_32ns_32_4_max_dsp_1"]

        del instance["CONTROL_BUS_s_axi"]
        if "urem_4ns_4ns_4_8_1" in instance:
            del instance["urem_4ns_4ns_4_8_1"]

        assert len(instance) == 0

        return summary

    LAT_LINES = []
    UTL_LINES = []

    for _, rpt_path, profile in reports:
        rpt_text = rpt_path.read_text()

        LAT_LINES.append(
            " & ".join(
                [
                    profile,
                    *extract_latency(rpt_text),
                ]
            )
            + r" \\"
        )

        UTL_LINES.append(
            " & ".join(
                [
                    profile,
                    *extract_utilization(rpt_text),
                ]
            )
            + r" \\"
        )

    with open(OUTPUT_DIR / f"{output_fn}.tex", "w") as f:
        f.write(
            SUMMARY_TEMPLATE.replace(
                "{{LATENCY}}", "\n".join(LAT_LINES).strip()
            ).replace("{{UTILIZATION}}", "\n".join(UTL_LINES).strip())
        )


def generate_loop_summary(rpt_path, output_fn):
    rpt_text = rpt_path.read_text()

    rpt = rpt_text[rpt_text.find("* Loop:") :]
    rpt = rpt[: rpt.find("===============")]
    rpt = rpt.strip().splitlines()[5:-1]

    lines = []
    for line in rpt:
        cells = line.split("|")[1:-1]
        cells[0] = r"\texttt{" + cells[0].rstrip(" ").replace("_", r"\_") + "}"
        for i in range(1, 8):
            cells[i] = cells[i].strip()

        lines.append(" & ".join(cells) + r" \\")

    with open(OUTPUT_DIR / f"{output_fn}.tex", "w") as f:
        f.write(LOOP_TEMPLATE.replace("{{}}", "\n".join(lines)))


if __name__ == "__main__":
    # update_summary()
    # update_loop()

    BASE = Path(__file__).parent

    FLOAT_REPORT_DIR = BASE / "../history"

    float_reports = []

    for profile_id, profile in FLOAT_REPORTS.items():
        rpt_path = FLOAT_REPORT_DIR / profile_id / "mmult_hw_csynth.rpt"
        assert rpt_path.is_file()

        generate_loop_summary(rpt_path, f"float-loop-{profile_id}")

        float_reports.append((profile_id, rpt_path, profile))

    generate_summary_table(float_reports, "float-summary")

    # for rpt in sorted(FLOAT_REPORT_DIR.glob("**/mmult_hw_csynth.rpt")):
    #     profile_id = rpt.parent.stem
    #     if profile_id in FLOAT_REPORTS:
    #         profile = FLOAT_REPORTS[profile_id]
    #         print(profile, rpt, sep="\t")
