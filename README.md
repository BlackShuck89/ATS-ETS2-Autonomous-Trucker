# ATS / ETS2 Autonomous Trucker

A Windows-first companion project that can take over routine driving in American Truck Simulator and Euro Truck Simulator 2, so the player can step away briefly during long trips.

This is intended to drive the active truck along the active GPS route—not only toggle cruise control. It should use the game's built-in driver assistance where useful, and manage steering, route decisions, signals, lights, and wipers through normal game inputs where possible.

## Project goals

- Follow the active route through ordinary highway driving, exits, and junctions.
- Use Adaptive Cruise Control and Lane Assistant when available.
- Set turn signals for route maneuvers and lane changes.
- Adapt headlights and wipers to in-game time and weather.
- Allow immediate player takeover and provide a conservative response when route or road understanding is uncertain.
- Support ATS and ETS2, while accounting for game-version and map-mod differences.

## Initial engineering direction

Start with a read-only telemetry feasibility prototype. Log the truck's position, heading, speed, navigation values, speed limit, game time, and available vehicle state. Compare these readings with the in-game Route Advisor during representative drives.

The official SCS Telemetry SDK is the initial data source. A companion process can later send controls through a virtual input device or supported input path. The first milestone is to establish what route and road information can be read reliably before enabling autonomous controls.

## Planned milestones

1. **Telemetry probe:** connect to a running game and display/log available telemetry.
2. **Route understanding:** determine how to identify the road and upcoming route decisions from supported data, map data, or Route Advisor perception.
3. **Input bridge:** send normal game controls and confirm state changes.
4. **Highway autonomy:** follow a simple route segment using the game's driving aids.
5. **Route maneuvers:** handle signals, lane selection, exits, and junctions.
6. **Weather and lighting:** set wipers and lights based on observed conditions.
7. **Short-break mode:** combine route following with takeover controls and conservative fallback behavior.

## Scope

This project controls a vehicle inside ATS/ETS2. It is not a real-world vehicle control system. Early prototypes should remain read-only or require the player to supervise them.

See [docs/initial-architecture.md](docs/initial-architecture.md) for the first feasibility questions and system boundaries.
