# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Build a PeriDEM input deck in Python.

Each block is produced by the ``inp::*Deck::getExampleJson`` that the C++
drivers call, reached through ``python/bindings.cpp``. The default values are
declared in those C++ factories and not in this module, so a deck written in
Python and the same deck written in C++ have the same defaults.

    import peridem
    from peridem import deck

    d = deck.Deck(dim=2, t_final=1.0e-4, n_steps=2000)
    d.set_output("runs/out", tags=["Displacement", "Velocity", "Damage_Z"])
    d.set_gravity(0.0, -10.0)

    circ = d.add_particle_type(deck.Geometry("circle", [1e-3, 0, 0, 0]),
                               mesh_size=1e-4)
    steel = d.add_material(horizon=3e-4, density=1200.0, K=2.16e7, nu=0.25,
                           Gc=100.0)
    d.add_contact_pair(0, 0, K=2.16e7, horizon=3e-4)
    d.place(0.0, 0.0, geometry=circ, material=steel)

    sim = d.simulation()
    sim.run()

``d.model``, ``d.output`` and ``d.neighbor`` are plain dicts and can be edited
before ``to_dict()`` for keys the methods below do not cover.
"""

from __future__ import annotations

import copy
import json
import os
from pathlib import Path
from typing import Any, Iterable, Sequence

from . import _core
from ._core import Geometry  # re-exported: geometry is a C++ object

__all__ = [
    "Deck",
    "Geometry",
    "MeshSpec",
    "contact_stiffness",
    "harmonic_mean",
    "to_E",
    "to_G",
    "to_K",
    "to_Gc",
    "to_KIc",
    "example_geometry",
    "acceptable_geometries",
]

_decks = _core.decks


def _load(text: str) -> dict[str, Any]:
    return json.loads(text)


# ---------------------------------------------------------------------------
# Elastic / contact helpers (same C++ functions the drivers use)
# ---------------------------------------------------------------------------

to_E = _core.to_E
to_K = _core.to_K
to_Gc = _core.to_Gc
to_KIc = _core.to_KIc
harmonic_mean = _core.harmonic_mean
acceptable_geometries = _core.acceptable_geometries


def to_G(K: float | None = None, nu: float = 0.25, E: float | None = None) -> float:
    """Shear modulus from either (K, nu) or (E, nu)."""
    if (K is None) == (E is None):
        raise ValueError("give exactly one of K or E")
    if E is not None:
        return _core.to_G_from_E(E, nu)
    return _core.to_G_from_K(K, nu)


def contact_stiffness(K1: float, K2: float, horizon: float,
                     horizon_power: int = 5) -> float:
    """Normal contact stiffness, from ``util::normalContactStiffness``.

    ``Kn = 18 * harmonic_mean(K1, K2) / (pi * horizon**horizon_power)``.

    ``horizon_power`` is 5 for three-dimensional nodal weights and 4 for
    two-dimensional ones. The 2D examples and the library's own
    internal-contact stiffness use 5, so it stays an explicit argument rather
    than being inferred from the deck's dimension.
    """
    return _core.normal_contact_stiffness(K1, K2, horizon, horizon_power)


def example_geometry(name: str, center: Sequence[float] = (0.0, 0.0, 0.0),
                     s: float = 0.001) -> Geometry:
    """Canonical parameters for ``name`` at ``center`` with length scale ``s``."""
    return Geometry.example(name, list(center), s)


# ---------------------------------------------------------------------------
# Mesh
# ---------------------------------------------------------------------------


class MeshSpec:
    """How one particle type gets its reference mesh.

    Either read a ``.msh`` from disk::

        MeshSpec(file="meshes/mesh_cir.msh")

    or let Gmsh build it in-process from the geometry::

        MeshSpec(size=1.0e-4)                       # never touches disk
        MeshSpec(size=1.0e-4, file="mesh_cir.msh")  # also writes the .msh
    """

    def __init__(self, file: str | os.PathLike[str] | None = None,
                 size: float | None = None, *, write: bool | None = None,
                 info: str = "gmsh_builtin_mesh",
                 voids: Sequence[Sequence[float]] | None = None):
        if file is None and size is None:
            raise ValueError("MeshSpec needs a file, a size, or both")
        self.file = None if file is None else str(file)
        self.size = size
        self.info = info
        # Boxes [xlo, ylo, zlo, xhi, yhi, zhi] carved out of a generated mesh,
        # so a notch is a real gap rather than material with its bonds cut.
        self.voids = [list(map(float, v)) for v in (voids or [])]
        for v in self.voids:
            if len(v) != 6:
                raise ValueError("each void region needs 6 values "
                                 "[xlo, ylo, zlo, xhi, yhi, zhi]")
        # Writing the .msh is only meaningful when we are creating one.
        self.write = (self.file is not None) if write is None else bool(write)

    @property
    def creates_mesh(self) -> bool:
        return self.size is not None

    def to_dict(self) -> dict[str, Any]:
        """The mesh set block, built by ``inp::MeshDeck::getExampleJson``."""
        return _load(_decks.mesh(
            filename=self.file or "",
            h=-1.0 if self.size is None else self.size,
            create_mesh=self.creates_mesh,
            create_mesh_info=self.info,
            write_mesh_file=bool(self.write and self.file is not None),
            void_regions=self.voids))


def _as_mesh(spec: MeshSpec | str | os.PathLike[str] | float | None,
             mesh_size: float | None) -> MeshSpec:
    if isinstance(spec, MeshSpec):
        return spec
    if spec is None:
        if mesh_size is None:
            raise ValueError("particle type needs a mesh file or a mesh_size")
        return MeshSpec(size=mesh_size)
    if isinstance(spec, (int, float)):
        return MeshSpec(size=float(spec))
    return MeshSpec(file=spec, size=mesh_size)


# ---------------------------------------------------------------------------
# Deck
# ---------------------------------------------------------------------------


class Deck:
    """A PeriDEM input deck under construction.

    The indexed families of the deck are kept as Python lists and only turned
    into ``Set_1``, ``Set_2``, ... keys in :meth:`to_dict`, so the counts can
    never disagree with the contents.
    """

    def __init__(self, dim: int = 2, t_final: float = 1.0, n_steps: int = 10,
                 *, spatial: str = "finite_difference",
                 time: str = "central_difference",
                 particle_sim_type: str = "Multi_Particle",
                 populate_element_node_connectivity: bool = True,
                 quad_order: int = 2, seed: int = 0,
                 bond_break: str | None = None,
                 self_contact: str | None = None,
                 wall_contact: str | None = None,
                 mpi_strategy: str | None = None):
        self.model: dict[str, Any] = _load(_decks.model(
            dim=dim, t_final=t_final, n_steps=n_steps, spatial=spatial,
            time=time,
            populate_element_node_connectivity=populate_element_node_connectivity,
            quad_order=quad_order, particle_sim_type=particle_sim_type,
            seed=seed))
        for key, value in (("Bond_Break", bond_break),
                           ("Self_Contact", self_contact),
                           ("Wall_Contact", wall_contact),
                           ("MPI_Strategy", mpi_strategy)):
            if value is not None:
                self.model[key] = value

        self.output: dict[str, Any] = _load(_decks.output(path="./"))
        self.neighbor: dict[str, Any] = _load(_decks.neighbor())

        self.geometries: list[Geometry] = []
        self.meshes: list[MeshSpec] = []
        self.materials: list[dict[str, Any]] = []
        self.contact_pairs: dict[tuple[int, int], dict[str, Any]] = {}
        self.contact_laws: dict[str, Any] = {}
        self.n_contact_groups = 0

        self.force_bc: list[dict[str, Any]] = []
        self.displacement_bc: list[dict[str, Any]] = []
        self.ic: list[dict[str, Any]] = []
        self.gravity: list[float] | None = None

        self.particles: list[dict[str, Any]] = []
        self.generation_method = "From_File"
        self.random_rotation = False
        self.comment: str | None = None
        self.test: dict[str, Any] | None = None
        #: extra top-level blocks merged into to_dict() verbatim
        self.extra: dict[str, Any] = {}
        #: Values derived while building the deck, for the caller to reuse.
        #: The examples put the notch position and the horizon here so that a
        #: later step does not recompute them. Not written to the deck.
        self.extra_info: dict[str, Any] = {}

    @property
    def particle_sim_type(self) -> str:
        return str(self.model.get("Particle_Sim_Type", "Multi_Particle"))

    @property
    def is_single_particle(self) -> bool:
        return self.particle_sim_type == "Single_Particle"

    def set_comment(self, text: str) -> None:
        self.comment = text

    def set_test(self, name: str, *, wall_id: int = 0,
                 wall_force_direction: int = 0) -> None:
        """Post-processing test block (``two_particle``, ``Compressive_Test``, ...)."""
        self.test = _load(_decks.test(
            test_name=name, particle_id_compressive_test=wall_id,
            particle_force_direction_compressive_test=wall_force_direction))

    # -- model / output -----------------------------------------------------

    @property
    def dim(self) -> int:
        return int(self.model["Dimension"])

    def set_output(self, path: str | os.PathLike[str] = "./",
                   tags: Sequence[str] = ("Displacement",),
                   interval: int = 1, *, out_format: str = "vtu",
                   debug: int = 2, perform_fe_out: bool = True,
                   compress_type: str = "zlib", perform_out: bool = True,
                   dt_test_out: int = 1, tag_pp: str = "",
                   pvd_collection: bool = False) -> None:
        """Output block. ``path`` is normalised to a trailing separator."""
        p = str(path)
        if not p.endswith(("/", os.sep)):
            p += "/"
        self.output = _load(_decks.output(
            out_format=out_format, path=p, tags=list(tags),
            output_interval=interval, debug=debug,
            perform_fe_out=perform_fe_out, compress_type=compress_type,
            perform_out=perform_out, dt_test_out=dt_test_out, tag_pp=tag_pp,
            pvd_collection=pvd_collection))

    def set_neighbor(self, update_criteria: str = "simple_all",
                     s_factor: float = 1.0, update_interval: int = 1,
                     near_bd_nodes_tol: float = 0.5) -> None:
        self.neighbor = _load(_decks.neighbor(
            update_criteria=update_criteria, s_factor=s_factor,
            update_interval=update_interval,
            near_bd_nodes_tol=near_bd_nodes_tol))

    def set_gravity(self, gx: float = 0.0, gy: float = 0.0,
                    gz: float = 0.0) -> None:
        self.gravity = [float(gx), float(gy), float(gz)]

    def add_rigid_particle(self, particle_id: int, mass: float) -> None:
        """Treat a placed particle as a rigid body of the given mass.

        Its nodal forces are replaced by the rigid-body acceleration each step,
        so it decelerates on contact instead of being driven through.
        """
        if mass <= 0.0:
            raise ValueError("rigid particle mass must be positive")
        self.model.setdefault("Rigid_Particles", []).append(
            {"Id": int(particle_id), "Mass": float(mass)})

    # -- particle types (geometry + mesh, one family) -----------------------

    def add_particle_type(self, geometry: Geometry | str,
                          mesh: MeshSpec | str | os.PathLike[str] | float | None = None,
                          *, params: Sequence[float] | None = None,
                          mesh_size: float | None = None,
                          vec_type: Sequence[str] = (),
                          vec_flag: Sequence[str] = ()) -> int:
        """Register a reference shape and its mesh. Returns its ``geom_id``.

        ``geometry`` may be a :class:`Geometry` or a geometry name plus
        ``params``. The mesh is either a ``.msh`` path or a Gmsh mesh size.
        """
        if isinstance(geometry, str):
            if params is None:
                raise ValueError(f"geometry '{geometry}' needs params")
            geometry = Geometry(geometry, list(params), list(vec_type),
                                list(vec_flag))
        self.geometries.append(geometry)
        self.meshes.append(_as_mesh(mesh, mesh_size))
        return len(self.geometries) - 1

    # -- materials ----------------------------------------------------------

    def add_material(self, *, horizon: float | None = None,
                     horizon_mesh_ratio: float | None = None,
                     density: float = 1.0, K: float | None = None,
                     G: float | None = None, nu: float | None = None,
                     E: float | None = None, Gc: float = 0.0,
                     material_type: str = "PDState",
                     is_plane_strain: bool = False,
                     compute_from_classical: bool = True,
                     influence_fn_type: int = 0,
                     influence_fn_params: Sequence[float] | None = None,
                     raw: dict[str, Any] | None = None) -> int:
        """Register a material. Returns its ``mat_id``.

        Give the elastic constants as (K, nu), (E, nu) or (K, G). Missing ones
        are filled in with the C++ ``material::to*`` conversions.
        """
        if K is None and E is not None and nu is not None:
            K = to_K(E, nu)
        if K is None:
            raise ValueError("material needs K, or E and nu")
        if G is None:
            if nu is None:
                raise ValueError("material needs G, or nu to derive it")
            # Same route the C++ drivers take: K -> E -> G.
            G = to_G(E=to_E(K, nu), nu=nu)
        if horizon is None and horizon_mesh_ratio is None:
            raise ValueError("material needs horizon or horizon_mesh_ratio")

        block = _load(_decks.material(
            material_type=material_type, is_plane_strain=is_plane_strain,
            horizon=-1.0 if horizon is None else horizon,
            horizon_mesh_ratio=-1.0 if horizon_mesh_ratio is None
            else horizon_mesh_ratio,
            density=density, K=K, G=G, Gc=Gc,
            compute_from_classical=compute_from_classical,
            influence_fn_type=influence_fn_type,
            E=-1.0 if E is None else E))
        if influence_fn_params:
            block["Influence_Function"]["Parameters"] = \
                [float(v) for v in influence_fn_params]
        if raw:
            block.update(copy.deepcopy(raw))
        self.materials.append(block)
        return len(self.materials) - 1

    # -- contact ------------------------------------------------------------

    def set_contact_laws(self, damping_law: str | None = None,
                         friction_law: str | None = None,
                         correct_volume: bool | None = None) -> None:
        for key, value in (("Damping_Law", damping_law),
                           ("Friction_Law", friction_law),
                           ("Correct_Volume", correct_volume)):
            if value is not None:
                self.contact_laws[key] = value

    def add_contact_pair(self, i: int, j: int, *,
                         contact_radius_factor: float = 0.95,
                         contact_radius: float | None = None,
                         Kn: float | None = None,
                         K: float | None = None,
                         K_pair: tuple[float, float] | None = None,
                         horizon: float | None = None,
                         damping_on: bool = True, friction_on: bool = False,
                         eps: float = 0.95, mu: float = 0.0,
                         Kn_factor: float = 1.0, beta_n_factor: float = 1.0,
                         delta_max: float = 1.0,
                         v_max: float = 0.0,
                         raw: dict[str, Any] | None = None) -> None:
        """Contact between contact group ``i`` and ``j`` (0-based, order free).

        ``Kn`` may be given directly, or derived from ``K_pair`` (the two bulk
        moduli) or a single ``K`` together with ``horizon``. ``raw`` merges
        extra keys into the block after the factory has built it.
        """
        if Kn is None and horizon is not None:
            if K_pair is not None:
                Kn = contact_stiffness(K_pair[0], K_pair[1], horizon)
            elif K is not None:
                Kn = contact_stiffness(K, K, horizon)
        if K is None and K_pair is not None:
            K = harmonic_mean(K_pair[0], K_pair[1])

        block = _load(_decks.contact_pair(
            contact_r=contact_radius if contact_radius is not None
            else contact_radius_factor,
            compute_contact_r=contact_radius is None,
            damping_on=damping_on, friction_on=friction_on,
            Kn=0.0 if Kn is None else Kn, eps=eps, mu=mu,
            Kn_factor=Kn_factor, beta_n_factor=beta_n_factor,
            delta_max=delta_max, v_max=v_max, K=0.0 if K is None else K))
        if raw:
            # For keys the factory normalises, such as Beta_n_Factor, which
            # it sets to zero when Damping_On is false.
            block.update(copy.deepcopy(raw))
        lo, hi = (i, j) if i <= j else (j, i)
        self.contact_pairs[(lo, hi)] = block
        self.n_contact_groups = max(self.n_contact_groups, hi + 1)

    # -- boundary and initial conditions ------------------------------------

    def add_displacement_bc(self, **kwargs: Any) -> int:
        self.displacement_bc.append(self._bc_set("Displacement_BC", kwargs))
        return len(self.displacement_bc) - 1

    def add_force_bc(self, **kwargs: Any) -> int:
        self.force_bc.append(self._bc_set("Force_BC", kwargs))
        return len(self.force_bc) - 1

    def add_initial_velocity(self, velocity: Sequence[float], *,
                             particles: Iterable[int] | None = None,
                             exclude: Iterable[int] | None = None,
                             region: Geometry | None = None) -> int:
        """Constant-velocity initial condition on the listed particles."""
        self.ic.append(self._bc_set("IC", {
            "particles": particles, "exclude": exclude, "region": region,
            "ic_type": "Constant_Velocity", "ic_vec": list(velocity)}))
        return len(self.ic) - 1

    def _bc_set(self, kind: str, kw: dict[str, Any]) -> dict[str, Any]:
        raw = kw.pop("raw", None)
        block = _load(_decks.bc_set(
            type=kind,
            region=kw.pop("region", None),
            particle_list=[int(p) for p in (kw.pop("particles", None) or [])],
            particle_exclude_list=[int(p) for p in (kw.pop("exclude", None) or [])],
            time_fn_type=kw.pop("time_fn_type", "") or "",
            time_fn_params=list(kw.pop("time_fn_params", None) or []),
            spatial_fn_type=kw.pop("spatial_fn_type", "") or "",
            spatial_fn_params=list(kw.pop("spatial_fn_params", None) or []),
            direction=[int(d) for d in (kw.pop("direction", None) or [])],
            zero_displacement=bool(kw.pop("zero_displacement", False)),
            ic_type=kw.pop("ic_type", "") or "",
            ic_vec=list(kw.pop("ic_vec", None) or [])))
        if kw:
            raise TypeError(f"unexpected BC argument(s): {sorted(kw)}")
        if raw:
            block.update(copy.deepcopy(raw))
        return block

    # -- particle placement -------------------------------------------------

    def place(self, x: float, y: float = 0.0, z: float = 0.0, *,
              geometry: int = 0, material: int = 0, contact: int = 0,
              theta: float = 0.0, scale: float = 1.0,
              wall: bool = False) -> int:
        """Place one instance of particle type ``geometry`` at (x, y, z)."""
        entry: dict[str, Any] = {
            "x": float(x), "y": float(y), "z": float(z),
            "theta": float(theta), "s": float(scale),
            "geom_id": int(geometry), "mat_id": int(material),
            "contact_id": int(contact),
        }
        if wall:
            entry["is_wall"] = True
        self.particles.append(entry)
        self.n_contact_groups = max(self.n_contact_groups, int(contact) + 1)
        return len(self.particles) - 1

    def place_many(self, sites: Iterable[Sequence[float]], **kwargs: Any) -> None:
        """``place`` for a sequence of (x, y[, z]) sites with shared options."""
        for site in sites:
            s = list(site)
            self.place(s[0], s[1] if len(s) > 1 else 0.0,
                       s[2] if len(s) > 2 else 0.0, **kwargs)

    # -- assembly -----------------------------------------------------------

    def _sets(self, items: Sequence[Any]) -> dict[str, Any]:
        out: dict[str, Any] = {"Sets": len(items)}
        for i, item in enumerate(items):
            out[f"Set_{i + 1}"] = item
        return out

    def to_dict(self) -> dict[str, Any]:
        """Assemble the full deck. Safe to call repeatedly."""
        if not self.geometries:
            raise ValueError("deck has no particle types; call add_particle_type")
        if not self.materials:
            raise ValueError("deck has no materials; call add_material")
        single = self.is_single_particle
        if single and len(self.geometries) != 1:
            raise ValueError("Single_Particle decks take exactly one particle type")
        if (not single and not self.particles
                and self.generation_method == "From_File"):
            raise ValueError("deck has no particles; call place()")

        d: dict[str, Any] = {}
        if self.comment is not None:
            d["Comment"] = self.comment
        d["Model"] = copy.deepcopy(self.model)
        d["Output"] = copy.deepcopy(self.output)

        n_bc = (len(self.force_bc), len(self.displacement_bc), len(self.ic))
        bc = _load(_decks.bc(n_force_sets=n_bc[0], n_disp_sets=n_bc[1],
                             n_ic_sets=n_bc[2],
                             gravity_active=self.gravity is not None,
                             gravity=self.gravity or [0.0, 0.0, 0.0]))
        # getExampleJson replaces Force_BC wholesale when there are force sets,
        # so re-attach gravity after the fact.
        if self.force_bc:
            bc["Force_BC"] = {"Sets": len(self.force_bc)}
            for i, s in enumerate(self.force_bc):
                bc["Force_BC"][f"Set_{i + 1}"] = s
        if self.gravity is not None:
            bc.setdefault("Force_BC", {})["Gravity"] = self.gravity
        for key, sets in (("Displacement_BC", self.displacement_bc),
                          ("IC", self.ic)):
            if not sets:
                continue
            bc[key] = {"Sets": len(sets)}
            for i, s in enumerate(sets):
                bc[key][f"Set_{i + 1}"] = s
        for key in ("Force_BC", "Displacement_BC", "IC"):
            if key in bc:
                d[key] = bc[key]

        geom_sets = [_load(g.to_json()) for g in self.geometries]
        d["Particle"] = self._sets(geom_sets)
        d["Mesh"] = self._sets([m.to_dict() for m in self.meshes])
        d["Material"] = self._sets([copy.deepcopy(m) for m in self.materials])

        if single:
            if self.test is not None:
                d["Test"] = copy.deepcopy(self.test)
            d.update(copy.deepcopy(self.extra))
            return d

        n_groups = max(self.n_contact_groups, 1)
        contact = _load(_decks.contact(n_sets=n_groups))
        for (i, j), block in self.contact_pairs.items():
            contact[f"Set_{i + 1}_{j + 1}"] = copy.deepcopy(block)
        missing = [f"Set_{i + 1}_{j + 1}"
                   for i in range(n_groups) for j in range(i, n_groups)
                   if not contact.get(f"Set_{i + 1}_{j + 1}")]
        if missing:
            raise ValueError(
                "contact pairs not set: " + ", ".join(missing) +
                ". Call add_contact_pair for every group combination.")
        contact.update(copy.deepcopy(self.contact_laws))
        d["Contact"] = contact

        d["Neighbor"] = copy.deepcopy(self.neighbor)

        gen = _load(_decks.particle_gen(method=self.generation_method))
        gen["Method"] = self.generation_method
        gen["Random_Rotation"] = bool(self.random_rotation)
        if self.generation_method == "From_File":
            data: dict[str, Any] = {"N": len(self.particles)}
            for i, p in enumerate(self.particles):
                data[str(i)] = copy.deepcopy(p)
            gen["Data"] = data
        d["Particle_Generation"] = gen

        if self.test is not None:
            d["Test"] = copy.deepcopy(self.test)
        d.update(copy.deepcopy(self.extra))
        return d

    def to_json(self, indent: int | None = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)

    def write(self, path: str | os.PathLike[str]) -> Path:
        """Write the deck as JSON. Useful to hand the same deck to bin/PeriDEM."""
        p = Path(path)
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(self.to_json() + "\n")
        return p

    def simulation(self, workdir: str | os.PathLike[str] | None = None):
        """Build an in-process :class:`peridem.Simulation` from this deck.

        ``workdir`` is the directory relative paths in the deck resolve against;
        it defaults to the current working directory.
        """
        from ._core import Simulation

        return Simulation.from_json(self.to_json(indent=None),
                                    "" if workdir is None else str(workdir))

    def run(self, workdir: str | os.PathLike[str] | None = None):
        """Build the simulation and run it to the final time."""
        sim = self.simulation(workdir)
        sim.run()
        return sim
