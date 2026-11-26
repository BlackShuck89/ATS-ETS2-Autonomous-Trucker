# Telemetry probe

This Windows-only prototype is read-only. It loads as an SCS Telemetry SDK plugin and writes a JSON Lines capture while ATS or ETS2 is running. It does not send input or control the truck.

## Build

1. Download the SCS Telemetry SDK from the [official modding page](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry) and extract it.
2. Install Visual Studio 2022 with the C++ desktop workload and CMake.
3. From the repository root, configure and build:

   ```powershell
   cmake -S telemetry-probe -B build/telemetry-probe -A x64 -DSCS_SDK_INCLUDE_DIR="C:/path/to/telemetry-sdk/include"
   cmake --build build/telemetry-probe --config Release
   ```

   Set SCS_SDK_INCLUDE_DIR to the directory containing scssdk.h and the common folder.

## Install and capture

Copy build/telemetry-probe/Release/autonomous_trucker_probe.dll into the game's plugin folder:

- ATS: American Truck Simulator/bin/win_x64/plugins/
- ETS2: Euro Truck Simulator 2/bin/win_x64/plugins/

Create the plugins folder if needed, then start the game. SCS may ask you to approve loading a telemetry plugin.

Captures are written to:

```text
%USERPROFILE%/Documents/ATS-ETS2-Autonomous-Trucker/captures/
```

Set ATS_ETS2_PROBE_DIR before starting the game to choose another output folder.

Drive a short route that includes a highway stretch, an exit or junction, a lane change, and any lighting or weather conditions you want to examine. The plugin records one JSONL event per telemetry callback; it does not currently capture screenshots or raw weather values.

## Compile a capture

With Python 3 installed, run:

```powershell
py tools/compile_capture.py "C:/path/to/capture.jsonl"
```

This writes:

- capture-summary.md — session details, channel coverage, position ranges, and numeric ranges
- capture-samples.csv — one row per UTC second, with the latest observed value per channel

Share the summary and CSV for initial review. The JSONL capture contains the full event stream if detailed timing or channel behavior needs investigation.

## Captured channels

The plugin requests game time, truck world placement, speed, steering/throttle/brake input, navigation distance/time/speed limit, cruise-control set speed, wipers, indicators, hazard lights, and headlight states.

A channel that is unavailable for a game/API version is skipped. The summary distinguishes missing channels from channels that produced events. The initial probe intentionally does not infer route turns, weather, or lane geometry.
