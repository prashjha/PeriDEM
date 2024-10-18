---
title: 'PeriDEM -- High-fidelity modeling of granular media consisting of deformable complex-shaped particles'
tags:
  - Granular media
  - Peridynamics
  - Discrete element method
  - Fracture
  - Computational mechanics
  - Particle breakage
  - Particle interlocking
authors:
  - name: Prashant Kumar Jha
    orcid: 0000-0003-2158-364X
    affiliation: 1
output:
  bookdown::pdf_document:
    includes:
      in_header: preamble.tex
affiliations:
 - name: Department of Mechanical Engineering, South Dakota School of Mines and Technology, Rapid City, SD 57701, USA
   index: 1
date: 17 October 2024
bibliography: paper.bib

---

![PeriDEM logo.\label{fig:logo}](../assets/logo/joss_logo.png){ width=80% }

# Summary

Modeling dynamics of deformning and breaking particles interacting with each other is a challenging task from both mechanistic modeling and simulation point of views. The PeriDEM library implements a hybrid approach combining peridynamics and the discrete element method (DEM), providing a means to simulate the intricate behaviors of granular systems involving arbitrarily shaped particles and particle breakage. The PeriDEM framework integrates the strengths of DEM, which effectively captures inter-particle contact forces, with peridynamics, which can model intra-particle deformation and fracture. This hybrid approach is particularly suited for scenarios involving extreme conditions such as high-stress environments where particle deformation and breakage play significant roles.

# Statement of Need

Granular materials are prevalent in numerous industrial sectors, including geotechnical, manufacturing, mining engineering, and pharmaceutics. Current modeling techniques such as DEM struggle with accurately capturing the behavior of granular materials under extreme conditions, especially when dealing with complex geometries and deformable particles.  PeriDEM implements a high-fidelity framework combining DEM and peridynamics to allow for accurate simulations of granular systems under extreme loading conditions. 

# Background

PeriDEM model was introduced in [@jha2021peridynamics], where it demonstrated the ability to model both inter-particle contact and intra-particle fracture for arbitrarily shaped particles. The underlying basic idea is that individual particles are modeled as deformable solid using peridynamics theory, and the contact between two deforming particles are applied at locally at the contact region allowing modeling of complex-shaped particles. The integration of peridynamics within DEM provides a flexible, hybrid framework that handles the contact mechanics at the particle boundary while accounting for the internal material response, including deformation and fracture. This opens up new avenues for exploring the interactions in granular systems, including developing constitutive laws for phenomenological continuum models, understanding effective behavior when subjected to large loading, and impact of particle shape on particle dynamics. 

## Brief Introduction to PeriDEM Model

![Motion of particle system.\label{fig:schemMultiParticles}](./files/multi-particle.png)
Suppose a fixed frame of reference and $\{\be_i\}_{i=1}^d$ are orthonormal bases. Consider a collection of $N_P$ particles $\Pscript{\Omega}{p}_0$, $1\leq p \leq N_P$, where $\Pscript{\Omega}{p}_0 \subset \bbR^d$ with $d=2,3$ represents the initial configuration of particle $p$. Suppose $\Omega_0 \supset \cup_{p=1}^{N_P} \Pscript{\Omega}{p}_0$ is the domain containing all particles; see \autoref{fig:schemMultiParticles}. The particles in $\Omega_0$ are dynamically evolving due to external boundary conditions and internal interactions; let $\Pscript{\Omega}{p}_t$ denote the configuration of particle $p$ at time $t\in (0, t_F]$, and $\Omega_t \supset \cup_{p=1}^{N_P} \Pscript{\Omega}{p}_t$ domain containing all particles at that time. The motion $\Pscript{\bx}{p} = \Pscript{\bx}{p}(\Pscript{\bX}{p}, t)$ takes point $\Pscript{\bX}{p}\in \Pscript{\Omega}{p}_0$ to $\Pscript{\bx}{p}\in \Pscript{\Omega}{p}_t$, and collectively, the motion is given by $\bx = \bx(\bX, t) \in \Omega_t$ for $\bX \in \Omega_0$. We assume the media is dry and not influenced by factors other than mechanical loading (e.g., moisture and temperature are not considered). The configuration of particles in $\Omega_t$ at time $t$ depends on various factors, such as material and geometrical properties, contact mechanism, and external loading. 
Essentially, there are two types of interactions present in the media:
\begin{itemize}
    \item[(1.)] {\it Intra-particle interaction} that models the deformation and internal forces in the particle and
    \item[(2.)] {\it Inter-particle interaction} that accounts for the contact between particles and the boundary of the domain the particles are contained in.
\end{itemize}
In DEM, the first interaction is ignored, assuming particle deformation is insignificant compared to the inter-particle interaction. On the other hand, PeriDEM, accounts for both interactions and is summarized next.

The balance of linear momentum for particle $p$, $1\leq p\leq N_P$, takes the form:
\begin{equation}
    \Pscript{\rho}{p} \Pscript{\ddot{\bu}}{p}(\bX, t) = \Pscript{\bff}{p}_{int}(\bX, t) + \Pscript{\bff}{p}_{ext}(\bX, t), \qquad \forall (\bX, t) \in \Pscript{\Omega}{p}_0 \times (0, t_F)\,,
\end{equation}
where $\Pscript{\rho}{p}$, $\Pscript{\bff}{p}_{int}$, and $\Pscript{\bff}{p}_{ext}$ are density, and internal and external force densities. The above equation is complemented with initial conditions, $\Pscript{\bu}{p}(\bX, 0) = \Pscript{\bu}{p}_0(\bX), \Pscript{\dot{\bu}}{p}(\bX, 0) = \Pscript{\dot{\bu}}{p}_0(\bX), \bX \in \Pscript{\Omega}{p}_0$. 

### Internal force - State-based peridynamics

Since all expressions in this paragraph are for a fixed particle $p$, we drop the superscript $p$, noting that material properties and other quantities can depend on the particle $p$.
Following [@silling2007peridynamic] and simplified expression of state-based peridynamics force in [@jha2021peridynamics], the internal force takes the form, for $\bX \in \Pscript{\Omega}{p}_0$,
\begin{equation}
    \Pscript{\bff}{p}_{int}(\bX, t) = \int_{B_{\epsilon}(\bX) \cap \Pscript{\Omega}{p}_0} \left( \bT_{\bX}(\bY) - \bT_{\bY}(\bX) \right) \, \dd \bY\,,
\end{equation}
where $\bT_{\bX}(\bY) - \bT_{\bY}(\bX)$ is the force on $\bX$ due to nonlocal interaction with $\bY$. Let $R = |\bY - \bX|$ be the reference bond length, $r = |\bx(\bY) - \bx(\bX)|$ current bond length, $s(\bY, \bX) = (r - R)/R$ bond strain, then $\bT_{\bX}(\bY)$ is given by \cite{silling2007peridynamic, jha2021peridynamics}
\begin{equation}
    \bT_{\bX}(\bY) = h(s) J(R/\epsilon)\, \left[R \theta_{\bX} \left(\frac{3\kappa}{m_{\bX}} - \frac{15 G}{3 m_{\bx}}\right) + (r - R) \frac{15 G}{m_{\bX}}\right] \frac{\bx(\bY) - \bx(\bX)}{|\bx(\bY) - \bx(\bX)|}\,,
\end{equation}
where
\begin{equation}
    \begin{split}
        m_{\bX} &= \int_{B_\epsilon(\bX) \cap \Pscript{\Omega}{p}_0} R^2 J(R/\epsilon) \, \dd \bY\,,\\
        \theta_{\bX} &= h(s) \frac{3}{m_{\bX}} \int_{B_\epsilon(\bX) \cap \Pscript{\Omega}{p}_0} (r - R) \, R \, J(R/\epsilon) \, \dd \bY\,,\\
        h(s) &= \begin{cases}
            1\,, &\qquad \text{if } s < s_0 := \sqrt{\frac{\calG_c}{\left(3 G + (3/4)^4 \left[\kappa - 5G/3\right]\right)\epsilon}}\,, \\
            0\,, & \qquad \text{otherwise}\,.
        \end{cases}
    \end{split}
\end{equation}
In the above, $J: [0, \infty) \to \bbR$ is the influence function, $\kappa, G, \calG_c$ are bulk and shear moduli and critical energy release rate, respectively. These parameters, including nonlocal length scale $\epsilon$, could depend on the particle $p$.

### DEM-inspired contact forces
The external force density $\Pscript{\bff}{p}_{ext}$ is generally expressed as
\begin{equation}
    \Pscript{\bff}{p}_{ext} = \Pscript{\rho}{p}\bb + \bff^{\Omega_0, (p)} + \sum_{q\neq p} \PQscript{\bff}{q}{p}\,,
\end{equation}
where $\bb$ is body force per unit mass, $\bff^{\Omega_0, (p)}$ and $\PQscript{\bff}{q}{p}$ are contact forces due to interaction between particle $p$ and container $\Omega_0$ and neighboring particles $q$, respectively. In \citenb{jha2021peridynamics, jha2024peridynamics}, the contact between two particles is applied locally where the contact takes place; this is exemplified in \autoref{fig:peridemContact} where contact between points $\by$ and $\bx$ of two distinct particles $p$ and $q$ is activated when they get sufficiently close. The contact forces are shown using a spring-dashpot-slider system. To fix the contact forces, consider a point $\bX\in \Pscript{\Omega}{p}_0$ and let $\PQscript{R}{q}{p}_c$ be the critical contact radius (points in particles $p$ and $q$ interact if the distance is below this critical distance). Further, define the relative distance between two points $\bY \in \Pscript{\Omega}{q}_0$ and $\bX \in \Pscript{\Omega}{p}$ and normal and tangential directions as follows:
\begin{equation}
    \begin{split}
        \PQscript{\Delta}{q}{p}(\bY, \bX) &= \vert \Pscript{\bx}{q}(\bY) - \Pscript{\bx}{p}(\bX) \vert - \PQscript{R}{q}{p}_c\,, \\
        \PQscript{\be}{q}{p}_N(\bY, \bX) &= \frac{\Pscript{\bx}{q}(\bY) - \Pscript{\bx}{p}(\bX)}{\vert \Pscript{\bx}{q}(\bY) - \Pscript{\bx}{p}(\bX) \vert}\,, \\
        \PQscript{\be}{q}{p}_T(\bY, \bX) &= \left[ \bI - \PQscript{\be}{q}{p}_N(\bY, \bX) \otimes \PQscript{\be}{q}{p}_N(\bY, \bX) \right]\frac{\Pscript{\dot{\bx}}{q}(\bY) - \Pscript{\dot{\bx}}{p}(\bX)}{\vert \Pscript{\dot{\bx}}{q}(\bY) - \Pscript{\dot{\bx}}{p}(\bX) \vert} \,.
    \end{split}
\end{equation}
Then the force on particle $p$ due to contact with particle $q$ can be written as \cite{jha2021peridynamics}:
\begin{equation}
    \PQscript{\bff}{q}{p} (\bX, t) = \int_{\bY \in \Pscript{\Omega}{q}_0 \cap B_{\PQscript{R}{q}{p}}(\bX)} \left( \PQscript{\bff}{q}{p}_N(\bY, \bX) + \PQscript{\bff}{q}{p}_T(\bY, \bX) \right)\, \dd \bY\,,
\end{equation}
with normal and tangential forces following \citenb{jha2021peridynamics, desai2019rheometry} given by
\begin{equation}
    \PQscript{\bff}{q}{p}_N(\bY, \bX) = \begin{cases}
        0\,, & \quad \text{if } \PQscript{\Delta}{q}{p}(\bY, \bX) \geq 0\,, \\
        \left[ \PQscript{\kappa}{q}{p}_N \PQscript{\Delta}{q}{p}(\bY, \bX) - \PQscript{\beta}{q}{p}_N \PQscript{\dot{\Delta}}{q}{p}(\bY, \bX)  \right] \PQscript{\be}{q}{p}_N\,, & \quad \text{otherwise}\,,
    \end{cases}
\end{equation}
and
\begin{equation}
    \PQscript{\bff}{q}{p}_T(\bY, \bX) = -\PQscript{\mu}{q}{p}_T \, \vert \PQscript{\bff}{q}{p}_N(\bY, \bX) \vert\, \PQscript{\be}{q}{p}_T\,.
\end{equation}

# Implementation

PeriDEM is implemented as an open-source library available in GitHub; see [PeriDEM](https://github.com/prashjha/PeriDEM). The library is designed with an emphasis on scalability and extensibility, supporting high-performance computing (HPC) through multi-threaded implementations using Taskflow for asynchronous computation. For contact modeling, nanoflann library for tree search is used to find the neighboring discretized nodes.

## Features
- Hybrid modeling using peridynamics and DEM for intra-particle and inter-particle interactions.
- Support for arbitrarily shaped particles, allowing for realistic simulation scenarios.
- Future work includes developing an adaptive modeling approach to enhance efficiency without compromising accuracy.
- Open-source implementation with support for HPC environments, leveraging modern multi-threading techniques for scalability.

## Examples
PeriDEM is suitable for modeling the mechanical behavior of granular materials in applications such as construction (e.g., cement and ballast), mining (e.g., ore handling), and pharmaceutical (e.g., tablet formation). The model has already been employed to study the compressive strength of particle assemblies and to investigate particle breakage due to high-velocity impacts, demonstrating its capability in simulating realistic industrial scenarios.


### Key results

![Nonlinear response under compression, {\bf b} exponential growth of compute time due to nonlocality of internal and contact forces, and {\bf c} rotating cylinder with nonspherical particles.\label{fig:peridemSummary}](./files/peridem-summary.png)
The main result from PeriDEM is the compression of 502 circular and hexagon particles in a rectangular container by moving the top wall. The stress on the moving wall as a function of wall penetration becomes increasingly nonlinear, and media shows signs of yielding as the damage becomes extensive; see \autoref{fig:peridemSummary}a. Preliminary compute time analysis with an increasing number of particles shows an exponential increase in compute time of contact and peridynamics forces, which is unsurprising as both computations are nonlocal. Demonstration examples also include attrition of various non-circular particles in a rotating cylinder \autoref{fig:peridemSummary}c. 


# References
