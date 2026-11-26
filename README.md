# ATS / ETS2 Autonomous Trucker

A Windows-first companion project for American Truck Simulator and Euro Truck Simulator 2. The goal is to let the player step away briefly during long trips while the system continues following the active GPS route.

The eventual driver should handle ordinary route following, lane changes, exits, signals, headlights, and wipers. It may use the games' built-in Adaptive Cruise Control and Lane Assistant, but it must also understand the route and road context.

## Current milestone: read-only telemetry capture

The first deliverable is a plugin that records truck telemetry without sending controls, plus a small compiler that turns a capture into a summary and per-second CSV.

- [Telemetry probe build and capture instructions](telemetry-probe/README.md)
- [Capture compiler](tools/compile_capture.py)
- [Initial architecture and feasibility plan](docs/initial-architecture.md)

The probe requests position, heading, speed, steering/throttle/brake, navigation distance/time/speed limit, game time, cruise-control speed, indicators, lights, and wiper state. It does not yet capture weather, screenshots, route geometry, or turn instructions. The initial capture will show which additional sensing or map data is needed.

## Planned milestones

1. **Telemetry feasibility:** build and run the read-only probe; inspect route and vehicle-state coverage.
2. **Route understanding:** derive upcoming decisions from supported data, map data, or Route Advisor perception.
3. **Input bridge:** send ordinary game controls and verify the resulting state.
4. **Autonomous route following:** handle highway driving, lane changes, exits, and junctions.
5. **Conditions and short-break mode:** adapt lights/wipers, allow immediate takeover, and add conservative fallback behavior.

Early prototypes are read-only or supervised. This project operates only inside ATS/ETS2 and is not a real-world vehicle control system.
