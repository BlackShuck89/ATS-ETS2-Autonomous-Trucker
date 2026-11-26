# Initial architecture and feasibility plan

## Intended operating mode

The target is short-break autonomy: while the player steps away, the companion should continue the current delivery along the game's active route for ordinary driving. It must not be limited to operating ACC or Lane Assistant. Route following, road positioning, lane changes, exits, signals, weather response, and control recovery are part of the eventual system.

The player must be able to take over immediately. If the companion loses route confidence or encounters a situation it cannot handle, it should reduce risk and stop or request the player's attention rather than make an uncertain maneuver.

## Proposed components

### 1. Game telemetry adapter

Use the official SCS Telemetry SDK as the initial source for truck and game state. First inspect and record the fields available for the supported ATS and ETS2 versions, including:

- World position and orientation
- Speed, steering, throttle, and brake state
- Navigation distance, ETA, and route speed limit
- In-game time
- Current lights, turn indicators, and wiper state
- Rain/weather state, if available through the supported interface

The adapter should normalize units and version differences into a stable internal snapshot. Missing fields must be represented as unavailable rather than guessed.

### 2. Route and road understanding

Telemetry navigation values are useful but may not include the complete route geometry or upcoming turn sequence. The feasibility prototype must determine whether route following can be built from:

1. Direct SDK data, if it exposes enough route structure;
2. Game map data plus the truck's position and route information; or
3. Reading the Route Advisor and other on-screen cues.

The system should keep route confidence and expose why confidence is low. Map mods and game updates can change the road graph and must be treated as compatibility concerns.

### 3. Driving policy

A route-following controller should decide desired lane, speed, signals, lights, and wipers. It can use the game's ACC, Emergency Braking, and Lane Assistant as control aids, but must not assume those features perform route planning.

The policy should distinguish at least:

- Cruise on a known road segment
- Prepare for a route maneuver
- Change lanes when safe
- Execute a ramp or junction
- Recover from a missed turn or unexpected stop
- Yield control or enter a conservative fallback state

### 4. Input adapter

Send controls through a supported input device or virtual controller, using the game's ordinary bindings where possible. Read telemetry back to confirm that stateful controls such as lights, indicators, and wipers actually changed. Avoid blind repeated toggles.

Player input should override automation immediately. The interface should include a clear enable/disable state and a visible indication of what the controller is doing.

## First milestone: read-only telemetry probe

The first implementation should not control the truck. It should connect to the game, show live values, and save a timestamped log. Drive a short route that includes a straight highway, a lane change, an exit, a junction, nighttime or dusk, and rain if practical.

Record:

- Which values are directly available and how often they update
- Whether navigation values identify upcoming maneuvers or only distance/ETA
- Whether road position can be matched to an in-game map
- Whether weather and vehicle-control states are exposed
- What breaks when the game or map version changes

The deliverable for this milestone is a verified telemetry inventory and a decision on the route-data source. Only then should the project send any driving controls.

## Initial references

- [SCS Telemetry SDK](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry)
- [SCS advanced input configuration](https://modding.scssoft.com/wiki/Documentation/Engine/Advanced_input_configuration)
- [ATS 1.47: Adaptive Cruise Control and Emergency Braking](https://blog.scssoft.com/2023/04/american-truck-simulator-147-update.html)
- [ATS 1.50: Lane Assistant](https://blog.scssoft.com/2024/05/american-truck-simulator-150-update.html)
