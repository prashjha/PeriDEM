---
title: 'PeriDEM -- High-fidelity modeling of granular media consisting of deformable complex-shaped particles'
tags:
  - Granular media
  - Peridynamics
  - Discrete element method
  - Fracture
  - Particle breakage
  - Particle interlocking
authors:
  - name: Prashant K. Jha
    orcid: 0000-0003-2158-364X
    affiliation: 1
affiliations:
  - name: Department of Mechanical Engineering, South Dakota School of Mines and Technology, Rapid City, SD 57701, USA
    index: 1
date: 25 November 2025
header-includes:
  - \usepackage{amsfonts,amssymb,amsmath}
bibliography: paper.bib

---

# Summary

Accurate simulation of granular materials under extreme mechanical conditions, such as crushing, fracture, and large deformation, remains a significant challenge in geotechnical, manufacturing, and mining applications. Classical discrete element method (DEM) models typically treat particles as rigid or nearly rigid bodies, limiting their ability to capture internal deformation and fracture. The PeriDEM library, first introduced in [@Jha2021JMPS], addresses this limitation by modeling particles as deformable solids using peridynamics, a nonlocal continuum theory that naturally accommodates fracture and significant deformation. Inter-particle contact is handled using DEM-inspired local laws, enabling realistic interaction between complex-shaped particles.

Implemented in \texttt{C++}, PeriDEM is designed for extensibility and ease of deployment. It relies on a minimal set of external libraries, supports multithreaded execution, and includes demonstration examples involving compaction, fracture, and rotational dynamics. The framework facilitates granular-scale simulations, supports the development of constitutive models, and serves as a foundation for multi-fidelity coupling in real-world applications.

# Statement of Need

Granular materials play a central role in many engineered systems, but modeling their behavior under high loading, deformation, and fragmentation remains an open problem. Popular open-source DEM codes such as YADE [@yade2021], BlazeDEM [@govender2016blaze], Chrono DEM-Engine [@zhang_2024_deme], and LAMMPS [@THOMPSON2022108171] are widely used but typically treat particles as rigid, limiting their accuracy in scenarios involving internal deformation and breakage. A recent review by Dosta et al. [@dosta2024comparing] compares several DEM libraries. Meanwhile, peridynamics-based codes such as Peridigm [@littlewood2024peridigm] and NLMech [@Jha2021] are designed to simulate deformation and fracture within a single structure, with limited support for multi-structure simulations. 

PeriDEM fills this gap by integrating state-based peridynamics for intra-particle deformation with DEM-style contact laws for particle interactions. This hybrid approach enables direct simulation of particle fragmentation, stress redistribution, and dynamic failure propagation—capabilities essential for modeling granular compaction, attrition, and crushing.

Recent multiscale approaches, including DEM-continuum and DEM-level-set coupling methods [@harmon2021modeling], aim to bridge scales but often rely on homogenization assumptions. Sand crushing in geotechnical systems, for example, has been modeled using micro-CT-informed FEM or phenomenological laws [@chen2023mechanical]. PeriDEM offers a particle-resolved alternative that allows bottom-up investigation of granular failure and shape evolution, especially in systems where fragment dynamics are critical.

## Brief Introduction to PeriDEM Model

![Motion of particle system.\label{fig:schemMultiParticles}](./files/multi-particle.png){width=40%}

Consider a fixed frame of reference and $\{\boldsymbol{e}_i\}_{i=1}^d$ are orthonormal bases. Consider a collection of $N_P$ particles ${\Omega}^{(p)}_0$, $1\leq p \leq N_P$, where ${\Omega}^{(p)}_0 \subset \mathbb{R}^d$ with $d=2,3$ represents the initial configuration of particle $p$. Suppose $\Omega_0 \supset \cup_{p=1}^{N_P} {\Omega}^{(p)}_0$ is the domain containing all particles; see \autoref{fig:schemMultiParticles}. The particles in $\Omega_0$ are dynamically evolving due to external boundary conditions and internal interactions; let ${\Omega}^{(p)}_t$ denote the configuration of particle $p$ at time $t\in (0, t_F]$, and $\Omega_t \supset \cup_{p=1}^{N_P} {\Omega}^{(p)}_t$ domain containing all particles at that time. The motion ${\boldsymbol{x}}^{(p)} = {\boldsymbol{x}}^{(p)}({\boldsymbol{X}}^{(p)}, t)$ takes point ${\boldsymbol{X}}^{(p)}\in {\Omega}^{(p)}_0$ to ${\boldsymbol{x}}^{(p)}\in {\Omega}^{(p)}_t$, and collectively, the motion is given by $\boldsymbol{x} = \boldsymbol{x}(\boldsymbol{X}, t) \in \Omega_t$ for $\boldsymbol{X} \in \Omega_0$. We assume the media is dry and not influenced by factors other than mechanical loading (e.g., moisture and temperature are not considered). The configuration of particles in $\Omega_t$ at time $t$ depends on various factors, such as material and geometrical properties, contact mechanism, and external loading. 
Essentially, there are two types of interactions present in the media:

- *Intra-particle interaction* that models the deformation and internal forces in the particle and
- *Inter-particle interaction* that accounts for the contact between particles and the boundary of the domain in which the particles are contained.

In DEM, the first interaction is ignored, assuming that particle deformation is insignificant compared to inter-particle interactions. On the other hand, PeriDEM accounts for both interactions.

The balance of linear momentum for particle $p$, $1\leq p\leq N_P$, takes the form:
\begin{equation}
 {\rho}^{(p)} {\ddot{\boldsymbol{u}}}^{(p)}(\boldsymbol{X}, t) = {\boldsymbol{f}}^{(p)}_{int}(\boldsymbol{X}, t) + {\boldsymbol{f}}^{(p)}_{ext}(\boldsymbol{X}, t), \qquad \forall (\boldsymbol{X}, t) \in {\Omega}^{(p)}_0 \times (0, t_F)\,,
\end{equation}
where ${\rho}^{(p)}$, ${\boldsymbol{f}}^{(p)}_{int}$, and ${\boldsymbol{f}}^{(p)}_{ext}$ are density, and internal and external force densities. The above equation is complemented with initial conditions, ${\boldsymbol{u}}^{(p)}(\boldsymbol{X}, 0) = {\boldsymbol{u}}^{(p)}_0(\boldsymbol{X}), {\dot{\boldsymbol{u}}}^{(p)}(\boldsymbol{X}, 0) = {\dot{\boldsymbol{u}}}^{(p)}_0(\boldsymbol{X}), \boldsymbol{X} \in {\Omega}^{(p)}_0$. 

### Internal force – State-based peridynamics

The internal force term ${\boldsymbol{f}}^{(p)}_{int}(\boldsymbol{X}, t)$ in the momentum balance governs intra-particle deformation and fracture. In PeriDEM, this term is modeled using a simplified state-based peridynamics formulation that accounts for nonlocal interactions over a finite horizon. The underlying model and its numerical implementation are discussed in detail in [[@Jha2021JMPS], Sections 2.1 and 2.3].

### DEM-inspired contact forces

The external force term ${\boldsymbol{f}}^{(p)}_{ext}(\boldsymbol{X}, t)$ includes body forces, wall-particle interactions, and contact forces from other particles. Contact is modeled using a spring-dashpot-slider formulation applied locally when particles come within a critical distance; see \autoref{fig:peridemContact}. This approach introduces nonlinear normal forces, damping, and friction without relying on particle convexity or geometric simplifications. The full formulation of contact detection, force assembly, and implementation is detailed in [[@Jha2021JMPS], Section 2.2].

![High-resolution contact approach in PeriDEM model for granular materials between arbitrarily-shaped particles. The spring-dashpot-slider system shows the normal contact (spring), normal damping (dashpot), and tangential friction (slider) forces between points $\boldsymbol{x}$ and $\boldsymbol{y}$.\label{fig:peridemContact}](./files/peridem-contact.png){width=40%}

# Implementation

[PeriDEM](https://github.com/prashjha/PeriDEM) is implemented in \texttt{C++} and hosted on GitHub. It depends on a minimal set of external libraries, most of which are bundled in the `external` directory. Key dependencies include Taskflow [@huang2021taskflow] for multithreaded parallelism, nanoflann [@blanco2014nanoflann] for efficient neighborhood search, and VTK for output. The numerical strategies for neighbor search, peridynamic integration, damage evaluation, and time stepping follow those introduced in [[@Jha2021JMPS], Section 3]. The core simulation model is implemented in [`src/model/dem`](https://github.com/prashjha/PeriDEM/blob/v0.2.1/src/model/dem), with the class [`DEMModel`](https://github.com/prashjha/PeriDEM/blob/v0.2.1/src/model/dem/demModel.cpp) managing particle states, force calculations, and time integration. This work builds on earlier research in the analysis and numerical methods for peridynamics; see [@jha2018numerical; @jha2019numerical; @jha2018numerical2; @Jha2020peri; @jha2025nodal].

## Features

- Combines peridynamics and DEM to model intra-particle deformation and inter-particle contact  
- Simulates deformation and breakage of single particles under complex loading conditions  
- Supports arbitrarily shaped particles for realistic granular systems  
- Ongoing development of MPI-based parallelism and adaptive modeling strategies to improve efficiency without sacrificing accuracy

## Examples

Example cases are described in [examples/README.md](https://github.com/prashjha/PeriDEM/blob/v0.2.1/examples/README.md). One key simulation demonstrates the compression of over 500 circular and hexagonal particles in a rectangular container by displacing the top wall. The stress on the wall becomes increasingly nonlinear with penetration depth as damage accumulates and the medium yields (see \autoref{fig:peridemSummary}a).

Preliminary performance tests show that compute time increases exponentially with particle count due to the nonlocal nature of both peridynamic and contact interactions—highlighting a computational bottleneck. This motivates future integration of MPI-based parallelism and a multi-fidelity modeling framework. Additional examples include attrition of non-circular particles in a rotating cylinder (\autoref{fig:peridemSummary}c).

![(a) Nonlinear response under compression, (b) exponential growth of compute time due to nonlocality of internal and contact forces, and (c) rotating cylinder with nonspherical particles.\label{fig:peridemSummary}](./files/peridem-summary.png){width=60%}

# Acknowledgements

This work was supported by the U.S. National Science Foundation through the Engineering Research Initiation (ERI) program under Grant No. 2502279. The support has contributed to the continued development and enhancement of the PeriDEM library.



# References
