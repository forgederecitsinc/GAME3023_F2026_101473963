# GAME3023_F2026_101473963

Project(s) for GAME3023 for Fall 2023 by Roshan Christy Romio Christy (101473963)

## Live Wire

**Project Overview & Core Theme:** Live Wire is a top-down 2.5D action-puzzle game set inside a decommissioned, multi-tier power substation that has been brought back online. The arena has an upper control booth, catwalks, moving walkways and a ground-level junction grid. Power is both the main objective and the main hazard: live wires and charged grids help charge the generator, but they can also kill the player and enemies.

**Core Gameplay Mechanics:** The player moves around under a fixed isometric camera (`USpringArmComponent` / `UCameraComponent`) with camera lag. The main loop is to collect colour/ID-coded **Battery Capsules**, plug them into the matching **Laser Guns (Power Cylinders)** and aim the beams at **Laser Targets** so that **Transformer Bushings** light up across the substation. Laser beams and patrolling **Enemy Drones** damage the player, so restoring power always has to be weighed against staying alive.

**Technical Setup & Architecture:** The game runs on a C++ core (`AActor`, `ACharacter`, `AGameModeBase`, `UUserWidget` + Slate) with Blueprint subclasses for visual tweaks. The project uses Git/GitHub for version control, with Git LFS for binary assets (`.uasset`, `.umap`). The engine is Unreal Engine 5.8 on macOS (Apple Silicon).

> **Design vision (planned, not yet implemented):** Arc Pulse weapon with knockback, a 10-minute match timer, a Generator Charge Meter, a graph-based `ACircuitManager`, cyclic electrified grates, sentry turrets, elevators, and Behavior Tree / AI Perception "Sentient" opponents restricted from player-only walkways.

---

## Controls

| Action | Keyboard | DualSense (PS5) |
|---|---|---|
| Move | WASD | Left Stick |
| Aim / Face direction | Mouse | Right Stick (Y inverted by default) |
| Collect battery | Tap **E** | Tap **Square** |
| Attach battery to Laser Gun | Hold **E** for 5 s | Hold **Square** for 5 s |
| Remove battery from Laser Gun | **E** | **Square** |
| Drop battery | **R** / **Q** | **Circle** |
| *Debug:* take 15 damage / heal 15 HP | **K** / **H** | — |

Jumping is turned off on purpose (puzzles stay on the ground plane).

---

## Devlogs

Each entry covers one feature or mechanic, in the order it was built. Commit hashes are listed so every entry can be traced in the Git history.

---

### Devlog #01: Project Setup & Version Control
**Date:** 2026-10-05 · **Commits:** `b789b26`, `29c0fbd`, `867e398`, `024e186`, `20bb294`

- Created the UE 5.8 C++ Third Person project `GAME3023_F26_Roshan`.
- Set up **Git LFS** for `.uasset` / `.umap` and other binary formats.
- Wrote a `.gitignore` that excludes `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`, `Build/`, IDE folders and `.DS_Store`.
- Removed the default template assets to get a clean base for the level.

---

### Devlog #02: Power Cylinder C++ Base Classes
**Date:** 2026-10-13 · **Commit:** `3f0aad4`

- Wrote the first C++ actor classes for the power cylinders, with Blueprint subclasses inheriting from them.
- Set up the pattern used for the rest of the project: **logic and procedural component setup in C++, visual tweaks in Blueprints.**

---

### Devlog #03: Battery Capsule Pickup
**Date:** 2026-11-01 · **Commits:** `3cf6893`, `5d43c10` · **Files:** `BatteryCapsule.h/.cpp`

- `ABatteryCapsule` is a carryable power cell built procedurally from engine primitives: base plinth, pedestal, battery core cylinder and top cap.
- Has a sphere **interaction zone**. Walking into it shows a prompt, and the player can pick the battery up.
- The battery attaches to the player while carried. **R** drops it back onto the floor.
- **Idle animation:** spins at 90°/s and bobs (2 Hz frequency, 10 cm amplitude) so it reads as a collectable.
- `bHideBaseWhenCarried` hides the plinth/pedestal while the battery is in the player's hands.

---

### Devlog #04: Power Cylinder System & Battery Matching
**Date:** 2026-11-01 → 2026-11-02 · **Commits:** `d5cfc55`, `ec95ec6`, `e1df926`, `c0d64fb` · **Files:** `RedPowerCylinder.h/.cpp`

- `ARedPowerCylinder` has a **battery socket**. Plugging a battery in powers the cylinder.
- **ID matching:** each battery has a `BatteryID` and each cylinder has a `CellID` (4-digit IDs, default `1001`). The cylinder only powers on if the IDs match. A mismatched battery still socket in but leaves it **OFFLINE** with an on-screen warning.
- The old hex-colour matching was replaced with **ID-driven colours**: `GetColorForID()` turns a 4-digit ID into a palette colour, so a battery and its matching cylinder always look the same.
- All cylinder settings are **instance-editable** in the Details panel.

---

### Devlog #05: Isometric "Hades-style" Camera
**Date:** 2026-11-02 → 2026-11-05 · **Commits:** `e1df926`, `0ce2b6c` · **Files:** `GAME3023_F26_RoshanCharacter.h/.cpp`

- Fixed top-down camera: −60° pitch, 1600 cm arm length, 85° FOV, 100 cm socket height offset.
- Camera lag (speed 4.0) for smooth tracking.
- Camera collision is **off** so the camera doesn't snap in when the player walks under catwalks.
- Every camera value is exposed in the inspector for tuning.

---

### Devlog #06: DualSense (PS5) Controller Support
**Date:** 2026-11-05 · **Commit:** `aad12ab`

- Out-of-the-box gamepad bindings using hardcoded key fallbacks, so no Input Mapping Context asset is needed.
- Twin-stick-style **right-stick aiming** with a configurable dead zone (0.2) to stop stick drift.
- **Controller vibration** (intensity 0.75) when picking up, plugging in and taking damage.
- `bInvertRightStickAimY` (on by default): pushing the stick up turns the character toward the top of the screen.
- Later cleanup: removed the R1/L1 duplicate bindings, so **Square = interact** and **Circle = drop**.

---

### Devlog #07: Level Design: Substation Arena Blockout
**Date:** 2026-11-01 → 2026-11-07 · **Commits:** `0aa0d7a`, `72d0c1e`, `7cfb3e0`, `66ba604`, `78e947d`, `97c23f5`, `549d4c8`, `d7d2190`, `fe87ad1`, `eaad9fc`, `11023f4`

- Redesigned and built out `Lvl_ThirdPerson` as a multi-tier substation using **World Partition** (one file per actor, which keeps merges small).
- Built a **level-design-asset (LDA) material palette** (`Blockouts/Materials/LDA` + `Colors`): walkable, wall, roof, catwalk, hazard, stairs, cover, column, fence, industrial, and container colours, so each surface type is readable during blockout.
- Placed conveyor belts and turner props (`M_ConveyerBelt`, `M_blueTurner`, `M_yellowTurner`).
- Placed lasers, targets and transformer bushings step by step to form the puzzle routes.
- Added a `BP_Transformer` level asset.

---

### Devlog #08: Roof / Catwalk See-Through Occlusion Cutout
**Date:** 2026-11-05 · **Commits:** `c5d1405`, `0ce2b6c` · **Assets:** `MPC_PlayerOcclusion`, `MF_SimsStyleCutout`, `M_OcclusionSurface`

- When a roof or catwalk blocks the line between the camera and the player, a **soft circular cutout** opens above the player (Sims / Hades / Tunic style).
- Every frame the character traces from the camera to a smoothed target point, then puts dynamic material instances on the blocking meshes and drives their parameters: `OcclusionCameraPos`, `OcclusionPlayerPos`, `OcclusionRadius` (260), `OcclusionSoftness` (140), `OcclusionStrength`.
- Fades in and out smoothly (speed 6.0) and keeps 5% opacity in the centre for a slight glass effect.
- **Electric cyan rim glow** on the cutout edge to fit the theme.
- Original materials are restored when the player leaves or when play ends.

---

### Devlog #09: Laser Gun (Power Cylinder Redesign)
**Date:** 2026-11-05 → 2026-11-06 · **Commits:** `40b4045`, `97c23f5`

- The cylinder was redesigned as a **turret-style laser gun**: plinth, pedestal, pillar post, neck collar, receiver body, stepped barrel, nozzle tip and a battery dock cradle on top.
- When powered, it fires a visible **laser beam mesh** from the nozzle (length 2500 cm, thickness 14 cm, adjustable `GunYaw`).
- `bLaserStopsAtObstacles`: the beam is line-traced and cut short at the first thing it hits.
- `bPreviewLaserInEditor` shows the beam in the viewport without pressing Play, which makes aiming easier during level design.
- **Jumping disabled** (`32bd571`) to keep gameplay on the puzzle plane.

---

### Devlog #10: Laser Targets & ID-Matched Hit Detection
**Date:** 2026-11-06 · **Commits:** `8a9a743`, `dbd2973`, `b6ea25e`, `785e9de` · **Files:** `LaserTarget.h/.cpp`

- `ALaserTarget` is the receiver post for beams: base pedestal, pillar, neck collar, coloured target body, top cap and a side **sensor box**.
- **ID matching:** the gun's `CheckActorTargetIDMatch()` only counts a hit if the target's `TargetID` equals the gun's `CellID`. Wrong-ID targets block the beam but don't activate.
- **Hit glow:** a matched target lights up with emissive glow and turns off when the beam leaves (tracked through `LastHitLaserTarget`).
- Broadcasts an `OnLaserHitStateChanged` delegate so other systems can react.
- Sensor auto-centres on the target body face (`bAutoCenterSensor`). Every mesh slot can be swapped from Blueprint.

---

### Devlog #11: Blueprint Exposure & Editor Live-Preview
**Date:** 2026-11-06 · **Commits:** `a09a942`, `d923e36`, `b6ea25e`

- All procedural components (`BasePlinthMesh`, `BatteryMesh`, `TopCapMesh`, etc.) changed from `VisibleAnywhere` to **`EditAnywhere` / `BlueprintReadWrite`**, so designers can edit them in `BP_BatteryCapsule`, `BP_LaserTarget` and `BP_RedPowerCylinder`.
- Added `PostEditChangeProperty` and `OnConstruction` rebuilds, so changing an ID, colour or scale **updates live in the viewport**.
- Added a dedicated `_Materials/LaserTarget` material set for the battery and target meshes.

---

### Devlog #12: Interaction Prompt UI
**Date:** 2026-11-06 → 2026-11-07 · **Commits:** `db21318`, `79c6682` · **Files:** `InteractPromptWidget.h/.cpp`

- `UInteractPromptWidget` is a minimal **Slate-built** prompt with a PlayStation Square button badge, an action line ("Press Square to Collect Battery") and a target subtitle ("Laser Gun (ID: 1001) – Online").
- The character scans a **260 cm radius** every tick for the nearest valid interactable (loose batteries and laser guns) and updates the prompt to match the situation: *Collect*, *Hands Full*, *Attach*, *Remove*, *Requires Battery*.
- Docks at the bottom centre of the screen (`bAnchorPromptToScreenBottom`) with alpha fades in and out.

---

### Devlog #13: Background Music
**Date:** 2026-11-06 → 2026-11-07 · **Commits:** `657a5c9`, `2aaba8d`

- Added the `LiveWire_Substation_Theme` track, generated with a Python synth script and imported to `/Game/Sound`.
- The character spawns a **2D non-spatialised audio component** on `BeginPlay`. It loops seamlessly through `OnAudioFinished`, with editable volume (0.65) and pitch, auto-play toggle, and `Play` / `Stop` / `SetBGMVolume` Blueprint functions.
- The generator script was later removed from the repo (`2aaba8d`) because a replacement track is planned. The imported theme asset is still in `Content/Sound`.

---

### Devlog #14: Player Health, Damage, Death & Respawn
**Date:** 2026-11-06 → 2026-11-07 · **Commits:** `9891e9a`, `c066e3c`

- Player starts with **100 HP** (`MaxHealth` / `CurrentHealth`).
- Damage comes in through both Unreal's `TakeDamage` override and a custom `ApplyDamage(Damage, Causer, Location, Impulse)`, which can also apply knockback impulse.
- Added `HealPlayer` and `SetHealth`, plus `OnHealthChanged` and `OnPlayerDied` multicast delegates for UI and Blueprint hooks.
- **Death:** a carried battery is dropped automatically, and the player respawns at `RespawnLocation` after `RespawnDelay` (1 s). The respawn timer is cleared safely in `EndPlay`.
- Taking damage triggers controller rumble.

---

### Devlog #15: Transformer Bushings (Neon Power Indicators)
**Date:** 2026-11-06 · **Commits:** `5aa3fb1`, `ccbc8fd`, `30f1681` · **Files:** `TransformerBushing.h/.cpp`

- `ATransformerBushing` is a procedural **ceramic insulator stack**: multiple ring discs on a post, styled after real substation bushings.
- When **lit**, it swaps to the glowing `M_laserbeam` neon material, raises emissive to 25×, and turns on a point light (8000 lumens, 800 cm radius).
- **Neon ignition flicker:** a short flicker plays on power-up, like a real neon tube igniting.
- Lights up when its `BushingID` matches either a laser target that has just been hit or a laser beam hitting the bushing directly. Supports an optional `LinkedLaserTarget` override and `bStayLitOnceActivated` for permanent progress.
- `OnTransformerStateChanged` delegate plus a `OnLitStateChanged` BlueprintNativeEvent. Optional light-up and power-down sounds.
- `bPreviewLitInEditor` shows the lit look in the editor viewport.

---

### Devlog #16: Player Health HUD
**Date:** 2026-11-07 · **Commit:** `d14b9ae` · **Files:** `PlayerHealthHUDWidget.h/.cpp`

- Health bar (340 × 24) in the **top-right corner**, built in Slate.
- **Colour gradient** follows health: cyan/green → yellow → crimson.
- A **lagging damage trail** shows how much health was just lost, and the bar **pulses** at critical health.
- Updated on every damage and heal call. Debug keys **K** (−15) and **H** (+15) for testing.

---

### Devlog #17: Laser Beam Burn Damage
**Date:** 2026-11-07 · **Commit:** `0d59448`

- Active laser beams are now a hazard: a player standing in a beam takes **10 damage every 0.35 s**.
- The gun's per-mesh colour properties were simplified so all colours come from `CellID`, which removes a whole group of colours that could fall out of sync.

---

### Devlog #18: Material Overrides & Blueprint Compile Safety
**Date:** 2026-11-07 · **Commit:** `216816f`

- Added **per-part material override slots** to the laser gun (plinth, pedestal, pillar, collar, body, barrel, nozzle, dock, beam) and the laser target, editable in Blueprint and per instance.
- **Bug fix:** creating transient dynamic material instances on Class Default Objects or archetypes during Blueprint compilation was corrupting component template serialization, which reverted materials. The colour code now returns early for `RF_ClassDefaultObject | RF_ArchetypeObject`.

---

### Devlog #19: Content Browser Cleanup
**Date:** 2026-11-07 · **Commit:** `122fff2`

- Scanned every `.uasset` and the C++ source for package references and removed orphaned or duplicate materials: `_Materials/M_Battery`, `M_RedPowerCyclinder` (duplicates of the `_Materials/gameplay/` versions), `M_gray` and `M_greenTurner`.
- The active gameplay, laser-target and arena prop materials were kept.

---

### Devlog #20: Enemy Drone AI & Laser Bomb Particles
**Date:** 2026-11-07 · **Commit:** `fc60d96` · **Files:** `EnemyDrone.h/.cpp`, `DroneLaserParticle.h/.cpp`, `BP_EnemyDrone`

- `AEnemyDrone` hovers **350 cm** above the floor and moves at 450 cm/s, turning smoothly to face where it's going.
- **Full NavMesh coverage patrol:** generates waypoints on a 600 cm grid within a 5000 cm radius, then visits each reachable point using NavMesh path corridors. Manual waypoints can override this. Uses the `NavigationSystem` module.
- **Downward laser spotlight** (tight 12°/18° cone, 1500 cm range) that also works as its **detection cone**: when the player is inside the lit circle on the floor (plus 55 cm capsule padding), the drone **pursues** them.
- While the player is under the light, the drone drops **laser bomb particles** every 0.45 s. `ADroneLaserParticle` falls at 1400 cm/s with a glowing red mesh and point light, and deals **10 damage** once on contact.
- All patrol, light and attack values are instance-editable. The mesh slot is left empty so a custom drone model can be assigned in Blueprint.

---

### Devlog #21: Hold-to-Attach Battery with Circular Progress Ring
**Date:** 2026-11-07 · **Commit:** `c066e3c`

- Plugging a battery into a laser gun now needs a **5-second hold** of E / Square. Letting go cancels it. **Picking up** a loose battery is still instant.
- New Slate widget `SCircularHoldBadge`: a **radial progress ring** (0 → 100%) drawn around the Square button logo in the interaction prompt.
- Interact input listens for `Started`, `Completed` and `Canceled` on Enhanced Input, plus raw key press/release fallbacks. A fresh press is required after each completed hold, so holding the button down doesn't trigger again by accident.
- **Pulsed rumble** during the hold for feedback through the controller.
- Interaction is blocked while the player is dead.

---

### Devlog #22: Build Hygiene
**Ongoing**

- Regular clean rebuilds with UnrealBuildTool (`Build.sh … -clean`, plus manual purges of `Binaries/`, `Intermediate/` and `DerivedDataCache/`) to catch stale-artifact problems early.
- Work is committed one feature or mechanic at a time, so each change can be reviewed and reverted on its own.

---

## Source Map

| System | Files |
|---|---|
| Player character, camera, input, occlusion, health, BGM, interaction | `GAME3023_F26_RoshanCharacter.h/.cpp` |
| Battery capsule | `BatteryCapsule.h/.cpp` |
| Laser gun / power cylinder | `RedPowerCylinder.h/.cpp` |
| Laser target | `LaserTarget.h/.cpp` |
| Transformer bushing | `TransformerBushing.h/.cpp` |
| Enemy drone + projectile | `EnemyDrone.h/.cpp`, `DroneLaserParticle.h/.cpp` |
| UI | `InteractPromptWidget.h/.cpp`, `PlayerHealthHUDWidget.h/.cpp` |
