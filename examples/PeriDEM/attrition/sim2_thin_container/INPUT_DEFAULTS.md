# Attrition input defaults audit (modular vs main sim2)

**Rule:** do not rely on omitted keys. Modular defaults differ from main in
several places; if a key is missing you silently get different physics.

## Trap defaults (will bite if omitted)

| Key | Modular default if omitted | Main sim2 / needed | Status in deck |
|-----|---------------------------|--------------------|----------------|
| `Contact.Damping_On` | **true** | false | set false |
| `Contact.Friction_On` | **true** | false | set false |
| `Contact.Damping_Law` | **com_and_node** | (no law; damping off) | set **off** |
| `Contact.Friction_Law` | coulomb_simple | coulomb (mu unused if off) | set coulomb_simple |
| `Contact.Kn_Factor` | 1 | 1 | set 1 |
| `Contact.Beta_n_Factor` | **1** | 100 | set **100** |
| `Contact.Epsilon` | **1** | 0.95 | set **0.95** |
| `Neighbor.Search_Interval` | **1** | 40 | set **40** |
| `Neighbor.Search_Factor` | **1** | 10 | set **10** |
| `Particle_Generation.Random_Rotation` | **true** | CSV supplies theta | set **false** |
| `Model.Self_Contact` | broken_bond_kn | main applies broken-bond self-contact in PD | set **broken_bond_kn** |
| `Model.Bond_Break` | tension | tension (unset) | set tension |
| `Model.Wall_Contact` | meshed | meshed | set meshed |
| `Output.Perform_FE_Out` | **true** | false | set false |
| `Material.Density` | **1** | 1200 | set 1200 |

## Explicit physics (must match main sim2 numbers)

| Item | Value |
|------|-------|
| Horizon | 2·h = 4e-4 (h = R_small/5) |
| K / G / Gc small | 1e4 / 6e3 / 50 |
| K / G / Gc large+wall | 1e5 / 6e4 / 100 |
| Kn (Silling) | 18 K_eff / (π δ⁵) |
| Geometry | R_in=0.02, R_out=0.0203, L_bar=0.005, W_bar=3e-4 |
| ω / center | −40π about (−0.004, 0.004, 0) |

## Wall DOFs — already controlled by input (do not confuse with main’s flag)

Previously, `All_Dofs_Constrained: true` set two things: (a) fix all wall dofs,
(b) `d_computeForce=false` (wall out of PD + out of contact *search*).

Modular already covers (a) via **`Displacement_BC`** on the wall particle
(`Direction: [1,2]` + rotation time/spatial functions). Evidence from short
probe: `dofs=46101`, `free=42141` → **3960 fixed** = 1980 wall nodes × 2 dofs.
Same pattern as compressive decks (`Zero_Displacement` or prescribed motion).

Also: `is_wall: true` already **excludes the wall from PD force**
(`periDEMModel.cpp`: walls stay on contact list, not on `d_fPdCompNodes`).

So wall kinematics are input-controlled. Grain tunneling through a thin fixed
wall is a **contact** issue, not missing all-dofs in the deck.

## Still not exposable / optional plumbing

1. **`Contact.Correct_Volume`** (added): default `true` (current modular).
   Set `false` for full `Vj` like main DEM contact. Attrition deck sets false.

2. **Main-only: wall `d_computeForce=false`** (skips wall neighbor search)  
   Modular keeps walls searching/reacting by design. Kinematics already fixed
   by BC; only wire if you want exact main search behavior.
