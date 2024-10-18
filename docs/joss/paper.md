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
affiliations:
 - name: Department of Mechanical Engineering, South Dakota School of Mines and Technology, Rapid City, SD 57701, USA
   index: 1
header-includes:
   - \usepackage{amsfonts,amssymb,amsmath,bm}
date: 17 October 2024
bibliography: paper.bib

---

# Summary

Granular materials are crucial in various sectors, including geotechnical, manufacturing, and mining. Predictive modeling of these materials under extensive loading becomes challenging due to the deformation and breakage of particles and the complex contact mechanisms between complex-shaped particles undergoing considerable deformation. Focusing on the scenarios when particle deformation and breakage are crucial, the PeriDEM model introduced in [@jha2021peridynamics] is implemented in the PeriDEM library. The underlying idea is that individual particles are modeled as deformable solids using peridynamics theory, and the contact between two deforming particles is applied locally at the contact region, allowing the modeling of complex-shaped particles. Integrating peridynamics within the discrete element method (DEM) provides a flexible, hybrid framework that handles the contact mechanics at the particle boundary while accounting for the internal material response, including deformation and fracture. This opens up new avenues for exploring the interactions in granular systems, including developing constitutive laws for phenomenological continuum models, understanding effective behavior when subjected to extensive loading, and the impact of particle shape on particle dynamics. 

# Statement of Need

As stated earlier, granular materials are prevalent in numerous industrial sectors, and the predictive modeling and simulation of these materials will play a fundamental role in designing processes. Current modeling techniques, such as DEM, are used widely, and numerous open-source libraries exist. However, the models based on DEM struggle to model particle deformation and breakage and accurately capture the behavior of granular materials under extreme conditions, especially when dealing with complex geometries and deformable particles. PeriDEM overcomes the challenges and implements a high-fidelity framework combining DEM and peridynamics to allow for accurate simulations of granular systems under extreme loading conditions. PeriDEM library makes the implementation of the high-fidelity approach transparent. The library depends on limited external libraries and is easier to build on Ubuntu and Mac systems, allowing quick testing and extension to user-specific needs. 

# Background

The PeriDEM model was introduced in [@jha2021peridynamics], demonstrating its ability to model inter-particle contact and intra-particle fracture for complex-shaped particles. It is briefly described next.

## Brief Introduction to PeriDEM Model

![Motion of particle system.\label{fig:schemMultiParticles}](./files/multi-particle.png){width=60%}

Consider a fixed frame of reference and $\{\bm{e}_i\}_{i=1}^d$ are orthonormal bases. Consider a collection of $N_P$ particles ${\Omega}^{(p)}_0$, $1\leq p \leq N_P$, where ${\Omega}^{(p)}_0 \subset \mathbb{R}^d$ with $d=2,3$ represents the initial configuration of particle $p$. Suppose $\Omega_0 \supset \cup_{p=1}^{N_P} {\Omega}^{(p)}_0$ is the domain containing all particles; see \autoref{fig:schemMultiParticles}. The particles in $\Omega_0$ are dynamically evolving due to external boundary conditions and internal interactions; let ${\Omega}^{(p)}_t$ denote the configuration of particle $p$ at time $t\in (0, t_F]$, and $\Omega_t \supset \cup_{p=1}^{N_P} {\Omega}^{(p)}_t$ domain containing all particles at that time. The motion ${\bm{x}}^{(p)} = {\bm{x}}^{(p)}({\bm{X}}^{(p)}, t)$ takes point ${\bm{X}}^{(p)}\in {\Omega}^{(p)}_0$ to ${\bm{x}}^{(p)}\in {\Omega}^{(p)}_t$, and collectively, the motion is given by $\bm{x} = \bm{x}(\bm{X}, t) \in \Omega_t$ for $\bm{X} \in \Omega_0$. We assume the media is dry and not influenced by factors other than mechanical loading (e.g., moisture and temperature are not considered). The configuration of particles in $\Omega_t$ at time $t$ depends on various factors, such as material and geometrical properties, contact mechanism, and external loading. 
Essentially, there are two types of interactions present in the media:
- *Intra-particle interaction* that models the deformation and internal forces in the particle and
- *Inter-particle interaction* that accounts for the contact between particles and the boundary of the domain in which the particles are contained.
In DEM, the first interaction is ignored, assuming particle deformation is insignificant compared to the inter-particle interaction. On the other hand, PeriDEM accounts for both interactions.

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
In the above, $J: [0, \infty) \to \mathbb{R}$ is the influence function, $\kappa, G, \mathcal{G}_c$ are bulk and shear moduli and critical energy release rate, respectively. These parameters, including nonlocal length scale $\epsilon$, could depend on the particle $p$.

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

[PeriDEM](https://github.com/prashjha/PeriDEM) is implemented in GitHub. It is based on C++ and uses only a handful of external libraries, which are included in the library in the `external` folder, allowing the code to be built and tested in Ubuntu and Mac systems relatively quickly. Specifically, we use taskflow [@huang2021taskflow] for asynchronous multithreaded computation, nanoflann [@blanco2014nanoflann] for tree search to calculate neighbors for contact forces, and VTK for output. MPI and metis [@karypis1997metis] have recently been integrated to implement distributed parallelism in the near future. 

## Features
- Hybrid modeling using peridynamics and DEM for intra-particle and inter-particle interactions.
- It can simulate the deformation and breakage of a single particle with complex boundary conditions using peridynamics.
- Support for arbitrary shaped particles, allowing for realistic simulation scenarios. 
- MPI will be used for distributed computing in the near future.
- Future work includes developing an adaptive modeling approach to enhance efficiency without compromising accuracy.

## Brief implementation details
The primary implementation of the model is carried out in the model directory [dem](https://github.com/prashjha/PeriDEM/tree/main/src/model/dem) and the PeriDEM model is implemented in class [DEMModel](https://github.com/prashjha/PeriDEM/tree/main/src/model/dem/demModel.cpp). 
The function `DEMModel::run()` performs the simulation. We next look at some key methods in `DEMModel` in more detail:

### DEMModel::run()
This function does three tasks:
```cpp
void model::DEMModel::run(inp::Input *deck) {
    // initialize data
    init();
    
    // check for restart
    if (d_modelDeck_p->d_isRestartActive)
      restart(deck);
    
    // integrate in time
    integrate();
}
```

In `DEMModel::init()`, the simulation is prepared by reading the input 
files (such as `.yaml`, `.msh`, and `particle_locations.csv`). 

### DEMModel::integrate()
Key steps in  `DEMModel::integrate()` are 
```cpp
void model::DEMModel::run(inp::Input *deck) {
    // apply initial condition
    if (d_n == 0)
      applyInitialCondition();
    
    // apply loading
    computeExternalDisplacementBC();
    computeForces();
    
    // time step
    for (size_t i = d_n; i < d_modelDeck_p->d_Nt; i++) {
      // advance simulation to next step
      integrateStep();
      
      // perform output if needed
      output();
    }
}
```

In `DEMModel::integrateStep()`, we either utilize the central-difference scheme, 
implemented in `DEMModel::integrateCD()`, or the velocity-verlet scheme, 
implemented in `DEMModel::integrateVerlet()`. As an example, we look at `DEMModel::integrateCD()` method below:
```cpp
void model::DEMModel::integrateVerlet() {
    // update current position, displacement, and velocity of nodes
    {
      tf::Executor executor(util::parallel::getNThreads());
      tf::Taskflow taskflow;
    
      taskflow.for_each_index(
        (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1,
          [this, dt, dim](std::size_t II) {
            auto i = this->d_fPdCompNodes[II];
    
            const auto rho = this->getDensity(i);
            const auto &fix = this->d_fix[i];
    
            for (int dof = 0; dof < dim; dof++) {
              if (util::methods::isFree(fix, dof)) {
                this->d_v[i][dof] += 0.5 * (dt / rho) * this->d_f[i][dof];
                this->d_u[i][dof] += dt * this->d_v[i][dof];
                this->d_x[i][dof] += dt * this->d_v[i][dof];
              }
            }
          } // loop over nodes
      ); // for_each
    
      executor.run(taskflow).get();
    }
    
    // advance time
    d_n++;
    d_time += dt;
    
    // update displacement bc
    computeExternalDisplacementBC();
    
    // compute force
    computeForces();
    
    // update velocity of nodes (similar to the above) 
}
```

### DEMModel::computeForces()
The key method in time integration is `DEMModel::computeForces()`
In this function, we compute internal and external forces at each node of a particle 
and also account for the external boundary conditions. This function looks like
```cpp
void model::DEMModel::computeForces() {
    // update the point cloud (make sure that d_x is updated along with displacement)
    auto pt_cloud_update_time = d_nsearch_p->updatePointCloud(d_x, true);
    pt_cloud_update_time += d_nsearch_p->setInputCloud();
    
    // reset forces to zero ...
    
    // compute peridynamic forces
    computePeridynamicForces();
    
    // compute contact forces between particles
    computeContactForces();
        
    // Compute external forces
    computeExternalForces();
}
```

### Further reading
The above gives the basic idea of implementation. For a closer look, interested readers can look at 
[demModel.cpp](https://github.com/prashjha/PeriDEM/tree/main/src/model/dem/demModel.cpp).

## Examples

![(a) Nonlinear response under compression, (b) exponential growth of compute time due to nonlocality of internal and contact forces, and (c) rotating cylinder with nonspherical particles.\label{fig:peridemSummary}](./files/peridem-summary.png){width=80%}

Examples are described in [examples/README.md](https://github.com/prashjha/PeriDEM/tree/main/examples/README.md) of the library. One key result is the compression of 500+ circular and hexagon particles in a rectangular container by moving the top wall. The stress on the moving wall as a function of wall penetration becomes increasingly nonlinear, and media shows signs of yielding as the damage becomes extensive; see \autoref{fig:peridemSummary}a. Preliminary compute time analysis with an increasing number of particles shows an exponential increase in compute time of contact and peridynamics forces, which is unsurprising as both computations are nonlocal. This also shows the bottleneck with the PeriDEM approach, motivating us to consider MPI-parallelism and multi-fidelity framework. Demonstration examples also include attrition of various non-circular particles in a rotating cylinder \autoref{fig:peridemSummary}c. 



# References
