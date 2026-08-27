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

See `src/stage1.c` for the implementation.

## Stage 2: Fuel Limiter (Min-Select Control Logic)

**Goal:** add a controller that prevents the ceiling violation Stage 1 demonstrated, by capping the mass flow that's actually used rather than letting driver demand pass through.

**Variable change:** Renamed mass_flow to driver_demand, so the signature itself makes clear it's the pre-limit requested value from the drive pressing the pedal.

**Architecture research:** real engine control uses a "min-select" (low-select) pattern: the ECU compares driver demand (driver_demand) against a regulatory limit (mass_flow_limit) and always uses whichever is lower: final_mass_flow = min(driver_demand, regulatory_limit).

**Implementation: apply_fuel_limiter(driver_demand, mass_flow_limit) function,  takes both values as parameters rather than reading a constant internally, so it doesn't depend on anything outside its own inputs. This makes it testable and reusable (e.g. with a different limit later, without changing the function itself).

**Simplification:** mass_flow_limit here is the flat 3,000 MJ/h ceiling converted to a mass flow value (CEILING / ENERGY_DENSITY), not the real RPM-dependent regulatory limit from the Overview section above (yet, see Stretch Goals below.)

**Result:** before the step, unchanged from Stage 1 (ceiling_comparison = −3000 MJ/h). After the step, driver demand is still 80 kg/h, but actual_mass_flow is capped at 71.428571 kg/h, giving energy_flow = 3,000 MJ/h exactly and ceiling_comparison = 0 MJ/h - zero violation.

**floating point precision:** mass_flow_limit is derived by division and then multiplied back, floating-point in C rounding could produce a tiny non-zero result (e.g. 1e-10) instead of exactly 0, even with no real violation. Worth using a small tolerance rather than a strict > 0  later.

**Stretch goals:**
- Dynamic driver demand — currently a hardcoded step (0 → 80 kg/h at 5ms). Making this a flexible test scenario (different demand shapes, ramps, multiple runs) is deferred to Stage 3
- User-input fuel compounds via scanf — would let different teams' energy density values be tested interactively. Deferred because it would reopen the "energy density is a fixed constant for the whole project" design decision, which should be a deliberate revision
- RPM-dependent regulatory limit and air-density- limited flow (from atmospheric intake pressure regulations, FIA Article 5.5.2) both deferred until the core controller architecture above is solid.

See `src/stage2.c` for the implementation.

## Stage 3: Structs — Test Scenarios and Team Fuel Profiles
 
**Goal:** replace the driver demand step and the fixed energy density  with structured, and in
the fuel case, user-supplied data, using structures for the first
time.
 
**Test scenario struct:** `struct test_scenario1` holds
`before_value`, `after_value`, `step_time`, and `name` (a driving
scenario label). One instance, `scenario`, replaces Stage 1/2's
hardcoded `0`/`80`/`5` step values directly, the loop's `if`/`else`
now reads `scenario.before_value`, `scenario.after_value`, and
`scenario.step_time` instead of bare numbers.
 
**Team fuel struct and dynamic input:** `struct team` holds
`name` and `fuel_energy_density`, filled interactively via `scanf` at
the start of `main`. This removed the `#define ENERGY_DENSITY 42.0`
constant. Every place that previously referenced `ENERGY_DENSITY` (`mass_flow_limit`
calculation, `energy_flow` calculation) now reads
`team.fuel_energy_density` instead.
 
**Result:** running with real input (e.g. team name "Ferrari",
fuel energy density 42.0) reproduces exactly the same output as
Stage 2's  version — `ceiling_comparison` still lands at
`0 MJ/h` after the step. 
 

**Stretch goal, newly added: PID with feedback.** A PID
controller could sit alongside (not replace) the current min-select
limiter the hard limiter retained underneath as a safety backstop. Deferred. this is planned *after* Stage 4 (CSV logging/plotting), since PID's actual benefits (smooth response vs. a hard
step) are best demonstrated visually once plotting exists.

The intended architecture is a cascade: PID computes a desired mass flow each iteration from feedback error, and that value is passed straight into the existing, unchanged apply_fuel_limiter from Stage 2. Under normal operation the limiter should rarely engage, since a well-tuned PID stays under the ceiling on its own. The  challenge is integral windup: if PID's output is clamped by the limiter, PID itself has no knowledge of that and keeps accumulating integral error against a target it's not actually reaching, causing overshoot or sluggish response once conditions change. Solving this requires anti-windup — feeding the limiter's clamping action back to the PID rather than letting the two controllers run blind to each other.</mark>
 
<mark>See `src/stage3.c` for the implementation.</mark>


