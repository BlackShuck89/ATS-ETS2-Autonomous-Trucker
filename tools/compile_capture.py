#!/usr/bin/env python3
"""Compile an Autonomous Trucker JSONL telemetry capture into a compact report.

Uses only the Python standard library. It does not connect to or control the game.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
from collections import defaultdict
from datetime import datetime
from pathlib import Path
from typing import Any

EXPECTED_CHANNELS = [
    "game.time",
    "truck.world.placement",
    "truck.speed",
    "truck.input.steering",
    "truck.input.throttle",
    "truck.input.brake",
    "truck.navigation.distance",
    "truck.navigation.time",
    "truck.navigation.speed.limit",
    "truck.cruise_control",
    "truck.wipers",
    "truck.lblinker",
    "truck.rblinker",
    "truck.light.parking",
    "truck.light.beam.low",
    "truck.light.beam.high",
    "truck.light.hazard",
]


def parse_capture(path: Path) -> tuple[dict[str, Any], list[dict[str, Any]], int]:
    metadata: dict[str, Any] = {}
    events: list[dict[str, Any]] = []
    malformed = 0
    with path.open("r", encoding="utf-8-sig") as source:
        for line in source:
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                malformed += 1
                continue
            if record.get("kind") == "session":
                metadata.update(record)
            elif record.get("kind") == "telemetry":
                events.append(record)
            else:
                malformed += 1
    return metadata, events, malformed


def iso_dt(value: str) -> datetime | None:
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except (TypeError, ValueError):
        return None


def as_number(value: Any) -> float | None:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    number = float(value)
    return number if math.isfinite(number) else None


def compact(value: Any) -> str:
    if isinstance(value, (dict, list)):
        return json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    if value is None:
        return ""
    if isinstance(value, bool):
        return "true" if value else "false"
    return str(value)


def make_outputs(input_path: Path, metadata: dict[str, Any], events: list[dict[str, Any]], malformed: int) -> tuple[Path, Path]:
    stem = input_path.with_suffix("")
    summary_path = stem.with_name(stem.name + "-summary.md")
    samples_path = stem.with_name(stem.name + "-samples.csv")

    events.sort(key=lambda row: row.get("utc", ""))
    per_channel: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for event in events:
        name = event.get("channel")
        if isinstance(name, str):
            key = name if event.get("index") is None else f"{name}[{event['index']}]"
            per_channel[key].append(event)

    columns = sorted(per_channel)
    buckets: dict[str, dict[str, str]] = defaultdict(dict)
    for event in events:
        timestamp = event.get("utc")
        name = event.get("channel")
        if not isinstance(timestamp, str) or not isinstance(name, str):
            continue
        second = timestamp[:19]
        key = name if event.get("index") is None else f"{name}[{event['index']}]"
        buckets[second][key] = compact(event.get("value"))

    with samples_path.open("w", newline="", encoding="utf-8") as destination:
        writer = csv.DictWriter(destination, fieldnames=["utc_second", *columns], extrasaction="ignore")
        writer.writeheader()
        for second in sorted(buckets):
            writer.writerow({"utc_second": second, **buckets[second]})

    times = [iso_dt(event.get("utc")) for event in events]
    valid_times = [value for value in times if value is not None]
    first_time = min(valid_times) if valid_times else None
    last_time = max(valid_times) if valid_times else None

    lines = [
        "# Telemetry capture summary",
        "",
        f"- Source: {input_path.name}",
        f"- Game: {metadata.get('game_id', 'unknown')} ({metadata.get('game_name', 'name unavailable')})",
        f"- SDK API version: {metadata.get('sdk_api_version', 'unknown')}",
        f"- Telemetry events: {len(events):,}",
        f"- Malformed or unrecognized lines: {malformed:,}",
        f"- First sample: {first_time.isoformat() if first_time else 'unavailable'}",
        f"- Last sample: {last_time.isoformat() if last_time else 'unavailable'}",
        "",
        "## Requested channel coverage",
        "",
        "| Channel | Status | Events |",
        "|---|---:|---:|",
    ]
    for channel in EXPECTED_CHANNELS:
        count = len(per_channel.get(channel, []))
        lines.append(f"| {channel} | {'observed' if count else 'not observed'} | {count:,} |")
    extra = [name for name in columns if name.split("[", 1)[0] not in EXPECTED_CHANNELS]
    if extra:
        lines.extend(["", "## Additional channels", ""])
        for channel in extra:
            lines.append(f"- {channel}: {len(per_channel[channel]):,} events")

    position_events = per_channel.get("truck.world.placement", [])
    positions = []
    for event in position_events:
        value = event.get("value")
        if isinstance(value, dict) and isinstance(value.get("position"), dict):
            p = value["position"]
            if all(as_number(p.get(axis)) is not None for axis in ("x", "y", "z")):
                positions.append(tuple(float(p[axis]) for axis in ("x", "y", "z")))
    if positions:
        xs, ys, zs = zip(*positions)
        lines.extend([
            "",
            "## Position coverage",
            "",
            f"- Position updates: {len(positions):,}",
            f"- World X span: {min(xs):.2f} to {max(xs):.2f}",
            f"- World Y span: {min(ys):.2f} to {max(ys):.2f}",
            f"- World Z span: {min(zs):.2f} to {max(zs):.2f}",
            "- This capture logs truck positions; it does not itself contain the full road route geometry.",
        ])

    lines.extend(["", "## Numeric channel ranges", "", "| Channel | Min | Mean | Max |", "|---|---:|---:|---:|"])
    for channel, records in sorted(per_channel.items()):
        numbers = [number for number in (as_number(record.get("value")) for record in records) if number is not None]
        if numbers:
            lines.append(
                f"| {channel} | {min(numbers):.4g} | {statistics.fmean(numbers):.4g} | {max(numbers):.4g} |"
            )

    lines.extend([
        "",
        "## Notes for route-data review",
        "",
        "- Navigation distance, navigation time, and route speed limit are logged when the game reports them.",
        "- A missing channel may be unavailable in the current game/API version or unavailable in that driving situation.",
        "- This tool reports captured values only; it does not infer upcoming turns or control the truck.",
        "",
        "The samples CSV pivots the event stream into one row per UTC second for easier inspection.",
        "",
    ])
    summary_path.write_text("\n".join(lines), encoding="utf-8")
    return summary_path, samples_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize a telemetry probe JSONL capture.")
    parser.add_argument("capture", type=Path, help="Path to a .jsonl capture written by the telemetry probe")
    args = parser.parse_args()
    if not args.capture.is_file():
        parser.error(f"capture file not found: {args.capture}")
    metadata, events, malformed = parse_capture(args.capture)
    summary, samples = make_outputs(args.capture, metadata, events, malformed)
    print(f"Read {len(events):,} telemetry events.")
    print(f"Wrote {summary}")
    print(f"Wrote {samples}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
