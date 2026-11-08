# GAME3023_F2026_101473963

Project(s) for GAME3023 for Fall 2023 by Roshan Christy Romio Christy (101473963)

Project Overview & Core Theme: Live Wire is a top-down 2.5D action-puzzle game set within a decommissioned multi-tier power substation brought back online. The arena combines an upper control booth, catwalks, moving walkways, and a ground-level junction grid. Power serves as both the central objective and a primary hazard—live wires and charged grids aid in charging the main generator while posing a lethal threat to both the player and opponents.

Core Gameplay Mechanics: Players navigate using a fixed isometric perspective (USpringArmComponent/UCameraComponent) with dynamic zoom and camera lag. Combat relies on the Arc Pulse weapon to deal damage and knock back enemies into hazards. Players must balance network repairs with survival under a 10-minute match timer, managing dynamic health, 5-second post-respawn invulnerability, and objective progress via the Generator Charge Meter.

Puzzle & Environmental Systems: Puzzle mechanics center around carrying and socketing Battery Capsules into Transformer Bushings to establish continuous electrical graph links managed by ACircuitManager. Navigating the arena requires managing cyclic electrified grates (Grates A/B), dodging Sentry Turrets with visual warning beams, and utilizing player-only moving walkways and elevators for vertical traversal.

AI Opponent Framework: Sentient AI enemies patrol the levels and attempt to sabotage circuit progress. Built using Unreal Engine Behavior Trees, Blackboards, and AI Perception (Sight), Sentients navigate via dynamic NavMesh routing and elevators while being restricted from using player-only moving walkway bridges via custom NavLink/NavModifier settings.
Technical Setup & Architecture: Built on a C++ core framework (AActor, APawn, ACharacter, AGameModeBase) paired with Blueprint subclasses for UI and visual customization. The project is managed under Git/GitHub version control with Git LFS for binary assets (.uasset, .umap), utilizing modular C++ components for health, inventory, circuit graph validation, and custom UMG HUD integration.