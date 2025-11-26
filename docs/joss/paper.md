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
  - name: Prashant Kumar Jha
    orcid: 0000-0003-2158-364X
    affiliation: 1
affiliations:
 - name: Department of Mechanical Engineering, South Dakota School of Mines and Technology, Rapid City, SD 57701, USA
   index: 1
header-includes:
   - \usepackage{amsfonts,amssymb,amsmath}
date: 25 November 2025
bibliography: paper.bib

---

# Summary

Accurate simulation of granular materials under extreme mechanical conditions-such as crushing, fracture, and large deformation—remains a significant challenge in geotechnical, manufacturing, and mining applications. Classical discrete element method (DEM) models typically treat particles as rigid or nearly rigid bodies, limiting their ability to capture internal deformation and fracture. The PeriDEM library, first introduced in [@jha2021peridynamics], addresses this limitation by modeling particles as deformable solids using peridynamics, a nonlocal continuum theory that naturally accommodates fracture and large deformation. Inter-particle contact is handled using DEM-inspired local laws, enabling realistic interaction between complex-shaped particles.

Implemented in C++, PeriDEM is designed for extensibility and ease of deployment. It relies on a minimal set of external libraries, supports multithreaded execution, and includes demonstration examples involving compaction, fracture, and rotational dynamics. The framework facilitates granular-scale simulations, supports the development of constitutive models, and serves as a foundation for multi-fidelity coupling in real-world applications.

# Statement of Need

Granular materials play a central role in many engineered systems, but modeling their behavior under high loading, deformation, and fragmentation remains an open problem. Popular open-source DEM codes such as YADE [@yade2021], BlazeDEM [@govender2016blaze], Chrono DEM-Engine [@zhang_2024_deme], and LAMMPS [@THOMPSON2022108171] are widely used but typically treat particles as rigid, limiting their accuracy in scenarios involving internal deformation and breakage. A recent review by Dosta et al. [@dosta2024comparing] compares these libraries across a range of bulk processes. Meanwhile, peridynamics-based codes like Peridigm [@littlewood2024peridigm] and NLMech [@Jha2021] offer detailed fracture modeling but do not capture realistic particle contact mechanics or bulk granular dynamics.

PeriDEM fills this gap by integrating state-based peridynamics for intra-particle deformation with DEM-style contact laws for particle interactions. This hybrid approach enables direct simulation of particle fragmentation, stress redistribution, and dynamic failure propagation—capabilities essential for modeling granular compaction, attrition, and crushing.

Recent multiscale approaches, including DEM-continuum and DEM-level-set coupling methods [@harmon2021levelset], attempt to bridge scales but often require homogenized assumptions. Sand crushing in geotechnical systems, for example, has been modeled using micro-CT-informed FEM or phenomenological laws [@chen2023mechanical]. PeriDEM offers a particle-resolved alternative that allows bottom-up investigation of granular failure and shape evolution, especially in systems where fragment dynamics are critical.

# Background

The PeriDEM model was introduced in [@jha2021peridynamics], demonstrating its ability to model inter-particle contact and intra-particle fracture for complex-shaped particles. It is briefly described next.

## Brief Introduction to PeriDEM Model

![Motion of particle system.\label{fig:schemMultiParticles}](./files/multi-particle.png){width=60%}

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

### Internal force - State-based peridynamics

Since all expressions in this paragraph are for a fixed particle $p$, we drop the superscript $p$, noting that material properties and other quantities can depend on the particle $p$.
Following [@silling2007peridynamic] and simplified expression of state-based peridynamics force in [@jha2021peridynamics], the internal force takes the form, for $\boldsymbol{X} \in {\Omega}^{(p)}_0$,
\begin{equation}
 {\boldsymbol{f}}^{(p)}_{int}(\boldsymbol{X}, t) = \int_{B_{\epsilon}(\boldsymbol{X}) \cap {\Omega}^{(p)}_0} \left( \boldsymbol{T}_{\boldsymbol{X}}(\boldsymbol{Y}) - \boldsymbol{T}_{\boldsymbol{Y}}(\boldsymbol{X}) \right) \, \mathrm{d} \boldsymbol{Y}\,,
\end{equation}
where $\boldsymbol{T}_{\boldsymbol{X}}(\boldsymbol{Y}) - \boldsymbol{T}_{\boldsymbol{Y}}(\boldsymbol{X})$ is the force on $\boldsymbol{X}$ due to nonlocal interaction with $\boldsymbol{Y}$. Let $R = |\boldsymbol{Y} - \boldsymbol{X}|$ be the reference bond length, $r = |\boldsymbol{x}(\boldsymbol{Y}) - \boldsymbol{x}(\boldsymbol{X})|$ current bond length, $s(\boldsymbol{Y}, \boldsymbol{X}) = (r - R)/R$ bond strain, then $\boldsymbol{T}_{\boldsymbol{X}}(\boldsymbol{Y})$ is given by [@silling2007peridynamic; @jha2021peridynamics]
\begin{equation}
 \boldsymbol{T}_{\boldsymbol{X}}(\boldsymbol{Y}) = h(s) J(R/\epsilon)\, \left[R \theta_{\boldsymbol{X}} \left(\frac{3\kappa}{m_{\boldsymbol{X}}} - \frac{15 G}{3 m_{\boldsymbol{X}}}\right) + (r - R) \frac{15 G}{m_{\boldsymbol{X}}}\right] \frac{\boldsymbol{x}(\boldsymbol{Y}) - \boldsymbol{x}(\boldsymbol{X})}{|\boldsymbol{x}(\boldsymbol{Y}) - \boldsymbol{x}(\boldsymbol{X})|}\,,
\end{equation}
where
\begin{equation}
 \begin{split}
 m_{\boldsymbol{X}} &= \int_{B_\epsilon(\boldsymbol{X}) \cap {\Omega}^{(p)}_0} R^2 J(R/\epsilon) \, \mathrm{d} \boldsymbol{Y}\,,\\
 \theta_{\boldsymbol{X}} &= h(s) \frac{3}{m_{\boldsymbol{X}}} \int_{B_\epsilon(\boldsymbol{X}) \cap {\Omega}^{(p)}_0} (r - R) \, R \, J(R/\epsilon) \, \mathrm{d} \boldsymbol{Y}\,,\\
 h(s) &= \begin{cases}
 1\,, &\qquad \text{if } s < s_0 := \sqrt{\frac{\mathcal{G}_c}{\left(3 G + (3/4)^4 \left[\kappa - 5G/3\right]\right)\epsilon}}\,, \\
 0\,, & \qquad \text{otherwise}\,.
 \end{cases}
 \end{split}
\end{equation}
In the above, $J: [0, \infty) \to \mathbb{R}$ is the influence function, $\kappa, G, \mathcal{G}_c$ are bulk and shear moduli and critical energy release rate, respectively. These parameters, including the nonlocal length scale $\epsilon$, could depend on the particle $p$.

### DEM-inspired contact forces

![High-resolution contact approach in PeriDEM model for granular materials\cite{jha2021peridynamics} between arbitrarily-shaped particles. The spring-dashpot-slider system shows the normal contact (spring), normal damping (dashpot), and tangential friction (slider) forces between points $\boldsymbol{x}$ and $\boldsymbol{y}$.\label{fig:peridemContact}](./files/peridem-contact.png){width=60%}

The external force density ${\boldsymbol{f}}^{(p)}_{ext}$ is generally expressed as
\begin{equation}
 {\boldsymbol{f}}^{(p)}_{ext} = {\rho}^{(p)}\boldsymbol{b} + \boldsymbol{f}^{\Omega_0, (p)} + \sum_{q\neq p} {\boldsymbol{f}}^{(q),(p)}\,,
\end{equation}
where $\boldsymbol{b}$ is body force per unit mass, $\boldsymbol{f}^{\Omega_0, (p)}$ and ${\boldsymbol{f}}^{(q),(p)}$ are contact forces due to interaction between particle $p$ and container $\Omega_0$ and neighboring particles $q$, respectively. In [@jha2021peridynamics], the contact between two particles is applied locally where the contact takes place; this is exemplified in \autoref{fig:peridemContact} where contact between points $\boldsymbol{y}$ and $\boldsymbol{x}$ of two distinct particles $p$ and $q$ is activated when they get sufficiently close. The contact forces are shown using a spring-dashpot-slider system. To fix the contact forces, consider a point $\boldsymbol{X}\in {\Omega}^{(p)}_0$ and let ${R}^{(q),(p)}_c$ be the critical contact radius (points in particles $p$ and $q$ interact if the distance is below this critical distance). Further, define the relative distance between two points $\boldsymbol{Y} \in {\Omega}^{(q)}_0$ and $\boldsymbol{X} \in {\Omega}^{(p)}$ and normal and tangential directions as follows:
\begin{equation}
 \begin{split}
 {\Delta}^{(q),(p)}(\boldsymbol{Y}, \boldsymbol{X}) &= \vert {\boldsymbol{x}}^{(q)}(\boldsymbol{Y}) - {\boldsymbol{x}}^{(p)}(\boldsymbol{X}) \vert - {R}^{(q),(p)}_c\,, \\
 {\boldsymbol{e}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) &= \frac{{\boldsymbol{x}}^{(q)}(\boldsymbol{Y}) - {\boldsymbol{x}}^{(p)}(\boldsymbol{X})}{\vert {\boldsymbol{x}}^{(q)}(\boldsymbol{Y}) - {\boldsymbol{x}}^{(p)}(\boldsymbol{X}) \vert}\,, \\
 {\boldsymbol{e}}^{(q),(p)}_T(\boldsymbol{Y}, \boldsymbol{X}) &= \left[ \boldsymbol{I} - {\boldsymbol{e}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) \otimes {\boldsymbol{e}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) \right]\frac{{\dot{\boldsymbol{x}}}^{(q)}(\boldsymbol{Y}) - {\dot{\boldsymbol{x}}}^{(p)}(\boldsymbol{X})}{\vert {\dot{\boldsymbol{x}}}^{(q)}(\boldsymbol{Y}) - {\dot{\boldsymbol{x}}}^{(p)}(\boldsymbol{X}) \vert} \,.
 \end{split}
\end{equation}
Then the force on particle $p$ at $\boldsymbol{X}$ due to contact with particle $q$ can be written as [@jha2021peridynamics]:
\begin{equation}
 {\boldsymbol{f}}^{(q),(p)} (\boldsymbol{X}, t) = \int_{\boldsymbol{Y} \in {\Omega}^{(q)}_0 \cap B_{{R}^{(q),(p)}_c}(\boldsymbol{X})} \left( {\boldsymbol{f}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) + {\boldsymbol{f}}^{(q),(p)}_T(\boldsymbol{Y}, \boldsymbol{X}) \right)\, \mathrm{d} \boldsymbol{Y}\,,
\end{equation}
with normal and tangential forces following [@jha2021peridynamics; @desai2019rheometry] given by, if ${\Delta}^{(q),(p)}(\boldsymbol{Y}, \boldsymbol{X}) < 0$,
\begin{equation}
 {\boldsymbol{f}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) = 
 \left[ {\kappa}^{(q),(p)}_N {\Delta}^{(q),(p)}(\boldsymbol{Y}, \boldsymbol{X}) - {\beta}^{(q),(p)}_N {\dot{\Delta}}^{(q),(p)}(\boldsymbol{Y}, \boldsymbol{X})  \right]\,, 
\end{equation}
else ${\boldsymbol{f}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) = \boldsymbol{0}$, and
\begin{equation}
 {\boldsymbol{f}}^{(q),(p)}_T(\boldsymbol{Y}, \boldsymbol{X}) = -{\mu}^{(q),(p)}_T \, \vert {\boldsymbol{f}}^{(q),(p)}_N(\boldsymbol{Y}, \boldsymbol{X}) \vert\, {\boldsymbol{e}}^{(q),(p)}_T\,.
\end{equation}
Here, ${\kappa}^{(q),(p)}_N, {\beta}^{(q),(p)}_N, {\mu}^{(q),(p)}_T$ are coefficients for normal contact, normal damping, and tangential friction forces, and generally depend on the material properties of two particles $p$ and $q$.

# Implementation

[PeriDEM](https://github.com/prashjha/PeriDEM) is implemented in GitHub. It is written in C++ and uses only a handful of external libraries, all included in the `external` folder, allowing the code to be built and tested on Ubuntu and Mac systems relatively quickly. Specifically, we use taskflow [@huang2021taskflow] for asynchronous multithreaded computation, nanoflann [@blanco2014nanoflann] for tree-based neighbor search to calculate contact forces, and VTK for output. MPI and metis [@karypis1997metis] have recently been integrated to implement distributed parallelism in the near future. This work is based on the previous research on analysis and numerical methods for peridynamics; see [@jha2018numerical; @jha2019numerical; @jha2018numerical2; @Jha2020peri; @lipton2019complex].

## Features
- Hybrid modeling using peridynamics and DEM for intra-particle and inter-particle interactions.
- It can simulate the deformation and breakage of a single particle with complex boundary conditions using peridynamics.
- Support for arbitrarily shaped particles, allowing for realistic simulation scenarios. 
- MPI will be used for distributed computing in the near future.
- Future work includes developing an adaptive modeling approach to enhance efficiency without compromising accuracy.

## Brief implementation details
The primary implementation of the model is carried out in the model directory [dem](https://github.com/prashjha/PeriDEM/tree/main/src/model/dem), and the PeriDEM model is implemented in class [DEMModel](https://github.com/prashjha/PeriDEM/tree/main/src/model/dem/demModel.cpp). The [README file](https://github.com/prashjha/PeriDEM/blob/main/README.md) discusses installation, examples, and brief implementation details. 

## Examples

![(a) Nonlinear response under compression, (b) exponential growth of compute time due to nonlocality of internal and contact forces, and (c) rotating cylinder with nonspherical particles.\label{fig:peridemSummary}](./files/peridem-summary.png){width=80%}

Examples are described in [examples/README.md](https://github.com/prashjha/PeriDEM/tree/main/examples/README.md) of the library. One key result is the compression of 500+ circular and hexagonal particles in a rectangular container by moving the top wall. The stress on the moving wall as a function of wall penetration becomes increasingly nonlinear, and the media shows signs of yielding as the damage becomes extensive; see \autoref{fig:peridemSummary}a. Preliminary compute-time analysis with an increasing number of particles shows an exponential increase in compute time for contact and peridynamics forces, which is unsurprising given that both computations are nonlocal. This also indicates a bottleneck in the PeriDEM approach, motivating us to consider MPI parallelism and a multi-fidelity framework. Demonstration examples also include attrition of various non-circular particles in a rotating cylinder \autoref{fig:peridemSummary}c. 



# References
