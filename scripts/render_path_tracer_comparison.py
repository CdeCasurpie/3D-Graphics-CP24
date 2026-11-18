#!/usr/bin/env python3
"""Render Task 15's measured CSV results as a submission-friendly PNG.

Requires Pillow. Does not run the renderer or invent missing measurements.
Usage: python scripts/render_path_tracer_comparison.py PATH_TO_LAB_RUN
"""
import argparse
import csv
import math
from pathlib import Path
import statistics

from PIL import Image, ImageDraw, ImageFont


def font(size, bold=False):
    filename = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    liberation = "LiberationSans-Bold.ttf" if bold else "LiberationSans-Regular.ttf"
    for candidate in (filename, f"/usr/share/fonts/liberation/{liberation}",
                      f"/usr/share/fonts/truetype/liberation2/{liberation}"):
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            pass
    return ImageFont.load_default(size=size)


def measurements(root, mode):
    with (root / mode / "benchmark.csv").open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    fields = ("width", "height", "samples", "bounces", "mode", "seed", "cpu_threads")
    assert rows, f"No measurements: {mode}"
    assert all(tuple(row[k] for k in fields) == tuple(rows[0][k] for k in fields)
               for row in rows), "Mismatched render settings"
    assert rows[0]["mode"] == mode
    times = {}
    for backend in ("CPU", "CUDA"):
        selected = [row for row in rows if row["backend"] == backend]
        values = [float(row["wall_seconds"]) for row in selected]
        assert len(values) >= 3 and all(math.isfinite(v) and v > 0 for v in values)
        assert len({row["trial"] for row in selected}) == len(selected)
        times[backend] = statistics.median(values)
    assert {row["trial"] for row in rows if row["backend"] == "CPU"} == {
        row["trial"] for row in rows if row["backend"] == "CUDA"
    }, "CPU/GPU trial sets differ"
    return rows[0], times


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    root = args.run
    assert (root / "SUCCESS.txt").is_file(), "Run must finish successfully"
    data = [measurements(root, mode) for mode in ("direct", "path")]
    log = (root / "lab_run.log").read_text()
    assert "CUDA path-tracer validation PASSED." in log
    assert log.count("ERROR SUMMARY: 0 errors") >= 2, "Sanitizer evidence missing"
    gpu = next(line.split(": ", 1)[1].split(" | ")[0] for line in log.splitlines()
               if line.startswith("CUDA device 0: "))
    cpu = next(line.split(":", 1)[1].strip() for line in log.splitlines()
               if line.startswith(("Nombre del modelo:", "Model name:")))
    settings = data[0][0]
    assert all(item[0][key] == settings[key] for item in data
               for key in ("width", "height", "samples", "seed", "cpu_threads"))
    image = Image.new("RGB", (1600, 970), "#101827")
    draw = ImageDraw.Draw(image)
    white, muted, accent = "#f1f5f9", "#b8c4d6", "#6ee7b7"
    draw.text((70, 50), "TASK 15  /  SIMPLE PATH TRACER", font=font(24, True), fill=accent)
    draw.text((70, 100), "CPU vs CUDA - measured render time", font=font(42, True), fill=white)
    draw.text((70, 173), f"CPU: {cpu}  |  {settings['cpu_threads']} workers", font=font(25), fill=muted)
    draw.text((70, 215), f"GPU: {gpu}", font=font(25), fill=muted)
    draw.text((70, 270),
              f"{settings['width']} x {settings['height']}  |  {settings['samples']} samples/pixel"
              "  |  Same lab computer  |  Median of 3 trials",
              font=font(25), fill=white)
    x = [70, 590, 920, 1260, 1530]
    top = 340
    draw.rounded_rectangle((70, top, 1530, top+250), radius=18, fill="#1d2a40")
    for i, title in enumerate(("Render mode", "CPU (seconds)", "CUDA (seconds)", "Speedup")):
        draw.text((x[i]+22, top+22), title, font=font(25, True), fill=muted)
    for row, (settings, times) in enumerate(data):
        y = top+88+row*80
        draw.line((90, y-10, 1510, y-10), fill="#35445e", width=1)
        mode = "Direct light (1 surface)" if row == 0 else f"Path tracing ({settings['bounces']} surfaces)"
        values = (mode, f"{times['CPU']:.6f} s", f"{times['CUDA']:.6f} s",
                  f"{times['CPU']/times['CUDA']:.2f}x")
        for col, value in enumerate(values):
            draw.text((x[col]+22, y+7), value, font=font(26, col == 3),
                      fill=accent if col == 3 else white)
    notes = [
        "Time = render wall time; CUDA includes allocation, uploads, readback and cleanup.",
        "Excludes warm-up, scene creation, PNG encoding and file I/O. No BVH / k-d tree.",
        "CPU/CUDA linear-RGB validation passed. Compute Sanitizer: 0 errors.",
        "Final submission image: 960 x 720, 256 samples/pixel, 5 surfaces (separate render).",
        "Benchmark settings above are identical across backends; final-image time is not compared.",
    ]
    for i, line in enumerate(notes):
        draw.text((70, 640+i*43), line, font=font(23), fill=muted)
    draw.text((70, 910), f"Source: {root.name}/{{direct,path}}/benchmark.csv",
              font=font(20), fill=accent)
    destination = root / "comparison.png"
    image.save(destination)
    print(destination)


if __name__ == "__main__":
    main()
