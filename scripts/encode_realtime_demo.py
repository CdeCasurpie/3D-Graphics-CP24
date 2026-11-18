#!/usr/bin/env python3
"""Encode actual viewer framebuffer captures using their measured timestamps."""
import argparse
import csv
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture_directory", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--remove-frames", action="store_true",
                        help="Remove only the PPMs encoded successfully by this invocation")
    args = parser.parse_args()
    if not shutil.which("ffmpeg"):
        parser.error("ffmpeg is required; capture frames are preserved")
    root = args.capture_directory.resolve()
    with (root / "viewer.csv").open(newline="") as stream:
        rows = {int(row["frame"]): float(row["elapsed_seconds"]) for row in csv.DictReader(stream)}
    frames = sorted(root.glob("frame-*.ppm"))
    if len(frames) < 2:
        parser.error("Need at least two captured framebuffer images")
    entries = [(frame, rows[int(frame.stem.split("-")[1])]) for frame in frames]

    def quoted(path):
        return str(path).replace("'", "'\\''")

    manifest = root / "capture.ffconcat"
    with manifest.open("w") as stream:
        stream.write("ffconcat version 1.0\n")
        for index, (path, timestamp) in enumerate(entries):
            duration = (entries[index + 1][1] - timestamp) if index + 1 < len(entries) else 1 / 15
            if duration <= 0:
                parser.error("Non-increasing capture timestamps")
            stream.write(f"file '{quoted(path)}'\nduration {duration:.9f}\n")
        stream.write(f"file '{quoted(entries[-1][0])}'\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "warning", "-y",
                    "-f", "concat", "-safe", "0", "-i", str(manifest),
                    "-fps_mode", "vfr", "-c:v", "libx264", "-crf", "21",
                    "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(args.output)], check=True)
    if args.remove_frames:
        for path, _ in entries:
            path.unlink()  # Only the explicit files successfully encoded above.
    print(f"Saved {args.output}; actual framebuffer captures with measured timing, not offline frames.")


if __name__ == "__main__":
    main()
