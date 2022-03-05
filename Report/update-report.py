from pathlib import Path
from collections import Counter


FLOAT_REPORTS = {
    "00-baseline-autopipe": r"\ref{sec:1a} & Baseline (AutoPipe)",
    "01-baseline-nopipe": r"\ref{sec:1a} & Baseline (NoPipe)",
    "02-pipeline-L3": r"\ref{sec:1bL3} & L3 Pipelining",
    "03-pipeline-L2": r"\ref{sec:1bL2} & L2 Pipelining",
    "04-pipeline-L1": r"\ref{sec:1bL1} & L1 Pipelining",
}

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

        assert len(instance) == 3
        assert "CONTROL_BUS_s_axi" in instance
        assert "fadd_32ns_32ns_32_5_full_dsp_1" in instance
        assert "fmul_32ns_32ns_32_4_max_dsp_1" in instance

        summary.append(str(instance["fadd_32ns_32ns_32_5_full_dsp_1"]))
        summary.append(str(instance["fmul_32ns_32ns_32_4_max_dsp_1"]))

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

    FLOAT_REPORT_DIR = BASE / "../zynq/hls/mmult_float/archive"

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
