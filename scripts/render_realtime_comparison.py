#!/usr/bin/env python3
"""Make a Task 16 FPS comparison PNG from a successful lab run (requires Pillow)."""
import argparse
import csv
import math
from pathlib import Path
import re
import statistics

from PIL import Image, ImageDraw, ImageFont


def font(size, bold=False):
    for name in (f"DejaVuSans{'-Bold' if bold else ''}.ttf",
                 f"/usr/share/fonts/truetype/dejavu/DejaVuSans{'-Bold' if bold else ''}.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default(size=size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    root = args.run
    if not (root / "SUCCESS.txt").is_file():
        parser.error("Run must have completed successfully")
    settings, summaries, metrics, offsets = {}, {}, {}, {}
    pattern = r"Triangles=(\d+) \| (\d+)x(\d+) \| samples/frame=(\d+) \| bounces=(\d+) \| seed=(\d+)"
    for backend in ("embree", "optix"):
        summaries[backend] = (root / backend / "summary.txt").read_text()
        match = re.search(pattern, summaries[backend])
        if not match:
            parser.error(f"Missing render settings: {backend}")
        settings[backend] = tuple(int(value) for value in match.groups())
        with (root / backend / "benchmark.csv").open(newline="") as stream:
            rows = list(csv.DictReader(stream))
        values = [float(row["wall_seconds"]) for row in rows]
        if not values or not all(math.isfinite(t) and t > 0 for t in values):
            parser.error(f"Invalid timings: {backend}")
        offsets[backend] = [(row["frame"], row["sample_offset"]) for row in rows]
        metrics[backend] = (len(values) / sum(values), 1 / statistics.median(values),
                            statistics.median(values) * 1000, len(values))
    if settings["embree"] != settings["optix"] or offsets["embree"] != offsets["optix"]:
        parser.error("CPU/GPU workload settings or sample sequences differ")
    _, width, height, spp, bounces, seed = settings["embree"]
    log = (root / "lab_run.log").read_text()
    cpu = next((line.split(":", 1)[1].strip() for line in log.splitlines()
                if line.startswith(("Model name:", "Nombre del modelo:"))), "See lab hardware log")
    gpu = summaries["optix"].splitlines()[0].split(" | ")[1]
    image = Image.new("RGB", (1600, 960), "#101827")
    draw = ImageDraw.Draw(image)
    white, muted, accent = "#f1f5f9", "#b8c4d6", "#6ee7b7"
    draw.text((65, 45), "TASK 16 / REAL-TIME PATH TRACING", font=font(25, True), fill=accent)
    draw.text((65, 98), "CPU Embree vs GPU OptiX - measured FPS", font=font(41, True), fill=white)
    draw.text((65, 170), f"{width} x {height} | {spp} sample/frame | {bounces} bounces | seed {seed}",
              font=font(25), fill=muted)
    draw.text((65, 212), f"CPU: {cpu}  |  GPU: {gpu}", font=font(24), fill=muted)
    x = [65, 590, 890, 1200]
    draw.rounded_rectangle((65, 290, 1535, 590), radius=18, fill="#1d2a40")
    for index, title in enumerate(("Backend", "Mean FPS", "Median-frame FPS", "Median frame")):
        draw.text((x[index] + 18, 320), title, font=font(24, True), fill=muted)
    for row, backend in enumerate(("embree", "optix")):
        mean, median, ms, count = metrics[backend]
        y = 395 + row * 90
        draw.line((85, y - 15, 1515, y - 15), fill="#35445e")
        name = "CPU - Intel Embree 4" if backend == "embree" else "GPU - NVIDIA OptiX"
        for column, text in enumerate((name, f"{mean:.2f}", f"{median:.2f}", f"{ms:.3f} ms")):
            draw.text((x[column] + 18, y), text, font=font(28, True), fill=white)
    ratio = metrics["optix"][1] / metrics["embree"][1]
    draw.text((65, 630), f"OptiX / Embree median-frame FPS: {ratio:.2f}x", font=font(32, True), fill=accent)
    notes = [
        f"{metrics['embree'][3]} measured frames/backend; warm-up and BVH/pipeline builds excluded.",
        "Headless render wall timing; GPU includes RGB readback. Not UI/display FPS.",
        "Diffuse, mirror/specular, and dielectric glass; shared scene and sampling.",
        "See the MP4 camera demo for actual interactive viewer FPS and resolution.",
    ]
    for index, text in enumerate(notes):
        draw.text((65, 700 + index * 42), text, font=font(24), fill=muted)
    draw.text((65, 895), f"Source: {root.name}/{{embree,optix}}/benchmark.csv", font=font(20), fill=accent)
    destination = root / "comparison.png"
    image.save(destination)
    print(destination)


if __name__ == "__main__":
    main()
