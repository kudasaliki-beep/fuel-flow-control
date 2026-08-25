# Fuel Flow Limiter Control System (C)

Simulated real-time fuel flow limiting control system, modelling a
constraint used in Formula 1 power units — including limiter control
logic, data logging, and controller tuning analysis.

## Overview

From 2026, F1 replaced the traditional mass-based fuel flow limit
(100 kg/h) with a maximum **energy** flow rate limit of 3,000 MJ/h
(roughly equivalent to 70 kg/h of fuel, depending on energy density).
This shift aligns with the introduction of 100% advanced sustainable
fuels and a roughly 50-50 power split between internal combustion and
electrical energy — it stops fuel suppliers being penalised for using
sustainable blends with different energy densities, while ensuring
every team gets equivalent performance regardless of fuel choice.

The 3,000 MJ/h cap applies at high engine speeds. At lower RPM, a
separate formula limits energy flow to control low-speed acceleration
and corner-exit traction:

```
Maximum Allowed Energy Flow (MJ/h) = 0.27 × Engine RPM + 165   
```

- At 10,500 RPM, this formula would allow ~4,000 MJ/h, but the hard
  3,000 MJ/h cap overrides it.
- Below 6,790 RPM, allowed energy flow scales linearly with engine
  speed.

Teams are also limited to 2,700 MJ total energy consumption per race.
There's no longer a strict fuel volume/weight limit — only the energy
cap — though fuel must maintain a minimum density of 720 kg/m³ to
prevent ultra-light, hyper-volatile chemical mixtures.


## Problem Statement

Modern F1 power units are restricted to a maximum fuel energy flow
rate (3,000 MJ/h). Exceeding this limit is illegal and gives a direct
power advantage; staying meaningfully under it wastes available
performance. The real system is a hard ceiling, not a fixed target —
the control problem is to track as close to the limit as possible
without crossing it, using continuously measured actual flow.

Real enforcement measures mass flow directly via a physical sensor and
derives energy flow from it in real time: the FIA uses contactless
ultrasonic Allengra transducers (two separate meters — one for the
team, one encrypted and FIA-only), working on a differential
transit-time principle — timing ultrasonic pulses between transducers
to calculate fluid velocity, with no moving parts to interfere with
flow. The two pipes have deliberately different geometry, making it
mechanically difficult to synchronise the two meters' readings. Even
if the sensor fails, flow can still be estimated indirectly from fuel
pressure and injector timing, though less accurately.


The controller must keep energy flow rate (derived from mass flow ×
fuel energy density) below the ceiling, while getting as close to it
as possible. Since energy density varies slightly batch-to-batch, the
actual mass flow rate corresponding to the limit isn't fixed — it
depends on which fuel is loaded.

**Sources:**
- https://www.auto123.com/en/news/f1-technique-formula-1-fuel-flow-sensor-explained/36019/
- https://scuderiafans.com/f1-how-the-new-2026-fuel-flow-meter-works-and-why-it-matters/

## Design Decisions

- **Limiter, not setpoint** — the controller tracks a ceiling, not a
  fixed target, matching how the real system works.
- **Energy flow, not mass flow** — `energy_flow = mass_flow ×
  energy_density`, with the ceiling check applied to the derived
  energy flow.
- **Energy density** — fixed constant for now; modelling batch-to-batch
  variation is a stretch goal, not core scope.
- **Data types** — `double` throughout, for precision over many future
  loop iterations.
- **Comparison convention** — `energy_flow − ceiling`; a negative
  result means safely under the limit, positive means a violation.

## Stage 0: Single Time-Step Calculation

**Inputs:** mass flow (kg/h), energy density (MJ/kg), energy flow
ceiling (3,000 MJ/h).

**Calculation:** derive energy flow from mass flow × energy density,
then compare against the ceiling.

**Assumptions at this stage** 
- No time dimension — single snapshot, no loop yet.
- Fixed, hardcoded inputs.
- Instantaneous, lag-free conversion — no sensor delay, noise, or
  measurement error modelled.
- No actuator or control input yet.
- Energy density treated as a single fixed constant.
- No unit conversion complexity (all values already in per-hour terms).

**Sanity check:** 70 kg/h × 42 MJ/kg = 2,940 MJ/h → −60 MJ/h vs.
ceiling (safely under). Matches `src/stage0.c` output.

See `src/stage0.c` for the implementation.

## Stage 1: Plant Simulation Loop

**Goal:** run the Stage 0 calculation repeatedly over simulated
time, with mass flow changing, and no control logic
yet to prove that an uncontrolled system can violate the ceiling.

**Driving scenario research:** real mass flow drops close to 0 kg/h
under braking or coasting and rises sharply under full throttle. All
teams run the same FIA-mandated standard ECU, giving a uniform,
near-instant throttle response (~10-15 ms). A physically detailed
model of that response (driver input delay, first-order actuator lag,
torque dead time) was researched and is documented for a future
refinement, but is **not yet implemented at stage 1** 

**Time step and duration:** `dt = 0.1 ms`, run for 200 iterations
(20 ms total simulated time). Mass flow is 0 kg/h for the first 5 ms
(simulating coasting e.g releasing pedal before a break zone to reserve fuel), then steps to a fixed high value at the 5 ms mark for the remaining 15 ms.

**Step-change value — deliberately unrealistic, and labelled as
such:** the real 2026 target mass flow (~70 kg/h) does *not* violate
the ceiling on its own (70 × 42 = 2,940 MJ/h) so it can't demonstrate a failure by
itself. Stage 1 instead uses **80 kg/h**, a value chosen specifically
showing that a mass flow rate  too high leads to a ceiling violation, which will be addressed in stage 2.

**Assumptions at this stage:**
- Mass flow changes as an instant step, not a physically modelled
  transition (see driving scenario research above).
- Still no actuator or controller — mass flow is set directly by
  the scenario, not by any control logic.
- Energy density remains a fixed constant for the rest of the project, since energy density is roughly constant within a single race/fuel batch
- 80 kg/h is a deliberate stress-test value

**Result:** confirms the uncontrolled failure mode. Before the
step (mass flow = 0), `ceiling_comparison = −3000 MJ/h` (safely under).
After the step (mass flow = 80 kg/h), `energy_flow = 3,360 MJ/h` and
`ceiling_comparison = +360 MJ/h`  sustained ceiling
violation with nothing to correct it, which is what Stage 2's
controller needs to prevent.



