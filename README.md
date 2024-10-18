# PeriDEM - Peridynamics-based discrete element model of granular systems

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/eba90e085ba048cb8f895010b8f13b03)](https://app.codacy.com/gh/prashjha/PeriDEM?utm_source=github.com&utm_medium=referral&utm_content=prashjha/PeriDEM&utm_campaign=Badge_Grade_Settings) [![CircleCI](https://circleci.com/gh/prashjha/PeriDEM.svg?style=shield)](https://circleci.com/gh/prashjha/PeriDEM) [![codecov](https://codecov.io/gh/prashjha/PeriDEM/branch/main/graph/badge.svg?token=JyVHXtXJWS)](https://codecov.io/gh/prashjha/PeriDEM) [![GitHub release](https://img.shields.io/github/release/prashjha/PeriDEM.svg)](https://GitHub.com/prashjha/PeriDEM/releases/) [![GitHub license](https://img.shields.io/github/license/prashjha/PeriDEM.svg)](https://github.com/prashjha/PeriDEM/blob/main/LICENSE) [![GitHub issues](https://img.shields.io/github/issues/prashjha/PeriDEM.svg)](https://github.com/prashjha/PeriDEM/issues) [![Join the chat at https://gitter.im/PeriDEM/community](https://badges.gitter.im/PeriDEM/community.svg)](https://gitter.im/PeriDEM/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![GitHub repo size](https://img.shields.io/github/repo-size/prashjha/PeriDEM.svg)](https://GitHub.com/prashjha/PeriDEM/) [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.13888588.svg)](https://zenodo.org/records/13888588)


<p align="center"> <img src="./assets/logo/logo.png" width="400"> </p>

## Table of contents

  - [Introduction](#Introduction)
  - [Documentation](#Documentation)
  - [Tutorial](#Tutorial)
  - [Examples](#Examples)
  - [Brief implementation details](#Brief-implementation-details)
  - [Installation](#Installation)
    * [Dependencies](#Dependencies)
    * [Building the code](#Building-the-code)
    * [Recommendations for quick build](#Recommendations-for-quick-build)
  - [Running simulations](#Running-simulations)
    * [Two-particle with wall](#Two-particle-with-wall)
    * [Compressive test](#Compressive-test)
    * [Attrition tests](#Attrition-tests)
    * [Single particle deformation](#Single-particle-deformation)
  - [Visualizing results](#Visualizing-results)
  - [Developers](#Developers)

## Introduction
Implementation of the high-fidelity model of granular media that combines the advantages 
of peridynamics and discrete element method (DEM). 
The model has the following advantages over existing mechanical models for granular media:
  - handle intra-particle deformation and breakage/damage
  - handle arbitrary shape of the particle. Inter-particle contact is not specific to any shape of the particle
  - tunable inter-particle contact parameters
  - easy to add different mechanical constitutive laws within peridynamics for individual particle deformation

For more details about the model and results, we refer to the paper:

> Prashant K. Jha, Prathamesh S. Desai, Debdeep Bhattacharya, Robert P Lipton (2020). 
> **Peridynamics-based discrete element method (PeriDEM) model of granular systems involving breakage of arbitrarily shaped particles**. 
> *Journal of the Mechanics and Physics of Solids, 151*, p.104376. Doi https://doi.org/10.1016/j.jmps.2021.104376.

Download pdf [here](https://prashjha.github.io/publication/jha-2020-peridem/jha-2020-peridem.pdf).

We have created channels on various platforms: 
- [PeriDEM on Gitter](https://gitter.im/PeriDEM/community?utm_source=share-link&utm_medium=link&utm_campaign=share-link)
  * Gitter is absolutely open and easy to join.
- [PeriDEM on slack](peridem.slack.com)
  * Send us an email if interested in joining the workspace.

## [Documentation](https://prashjha.github.io/PeriDEM/)
[Doxygen generated documentation](https://prashjha.github.io/PeriDEM/) details functions and objects in the library. 

## Tutorial
We explain the setting-up of simulations in further details in [tutorial](./tutorial/README.md). 
We consider `two-particle` test setup with non-circular particles and `compressive-test` to 
discuss the various aspects of simulations.

## [Examples](./examples/README.md)
We next highlight some key examples. Further details are available in [examples/README.md](./examples/README.md). 

### Two-particle tests

|       <img src="./assets/two_particle_circ_no_damp.gif" width="200">       |     <img src="./assets/two_particle_circ_damp.gif" width="200">      |
|:--------------------------------------------------------------------------:|:--------------------------------------------------------------------:|
| [Circular without damping](./examples/PeriDEM/two_particles/circ_no_damp/) | [Circular with damping](./examples/PeriDEM/two_particles/circ_damp/) |

|    <img src="./assets/two_particle_circ_diff_material.gif" width="200">     |   <img src="./assets/two_particle_circ_damp_diff_radius.gif" width="200">   |            <img src="./assets/two_particle_circ_diff_radius_diff_material.gif" width="200">             |
|:---------------------------------------------------------------------------:|:---------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------:|
| [Different materials](./examples/PeriDEM/two_particles/circ_diff_material/) | [Different radius](./examples/PeriDEM/two_particles/circ_damp_diff_radius/) | [Different radius different material](./examples/PeriDEM/two_particles/circ_diff_radius_diff_material/) |

### Two-particle with wall test

|   <img src="./assets/two_particle_wall_concave_diff_material_diff_size.gif" width="400">    | 
|:-------------------------------------------------------------------------------------------:| 
| [Concave particles](./examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/) |

### Compressive tests
Setup for this test consists of 502 circular and hexagonal-shaped particles of varying 
radius and orientation inside a rectangle container. The container's top wall is moving 
downward at a prescribed speed, resulting in the compression of the particle system. 
The quantity of interest is the compressive strength of the media. The reaction force 
(downward) on the moving wall should increase with the increasing penetration of this wall; 
however, after a certain amount of compression of the media, the damage will initiate 
in individual particles, especially those connected by force chains, resulting in the 
yielding of the system. For more details, we refer to 
[Jha et al. 2021](https://prashjha.github.io/publication/jha-2020-peridem/)

| <img src="./assets/compressive_test.gif" width="600"> | 
|:-----------------------------------------------------:| 
|              Compressive test simulation              | 


### Attrition tests
We consider mix of different particles in a rotating container. Particles considered include circular, triangular, hexagonal, and drum shaped. Particles come in large and small shapes (their sizes are purturbed randomly). In order to to introduce diversity of material properties, we considered large particles to be tougher compared to the smaller ones. Setup files are in [examples/PeriDEM/attrition_tests](./examples/PeriDEM/attrition_tests)

|                                         <img src="./assets/attrition_test_sim1.gif" width="250">                                         |                                                                              <img src="./assets/attrition_test_sim2.gif" width="250">                                                                               | 
|:----------------------------------------------------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:| 
| Rotating cylinder ([setup](./examples/PeriDEM/attrition_tests/sim1_multi_particle_circ_tri_drum_with_rotating_cylinder_with_protrusion)) | Rotating cylinder with center of rotation offset ([setup](./examples/PeriDEM/attrition_tests/sim2_multi_particle_circ_tri_drum_hex_with_rotating_cylinder_with_protrusion_thin_container_and_change_rotation_rate)) | 

Complex container geometries can be considered as well. For example, the image below is from [attrition_tests](./examples/PeriDEM/attrition_tests/sim4_multi_particle_circ_tri_drum_hex_with_rotating_rectangle_container_with_protrusion_and_opening_within_bigger_rectangle_container) and includes rotating rectangle with opening and internal groves of different shapes. The rotating container with particles inside is contained within another rectangle which is fixed in its place. 

<img src="./examples/PeriDEM/attrition_tests/sim4_multi_particle_circ_tri_drum_hex_with_rotating_rectangle_container_with_protrusion_and_opening_within_bigger_rectangle_container/init_view.png" width="600">

### Single particle deformation
We can use `PeriDEM` executable or `Peridynamics` executable in `apps` directory to simulate the deformation of single particle/structure using peridynamics. See [examples/README.md](./examples/README.md) and [examples/Peridynamics](./examples/Peridynamics) folder. 

## Brief implementation details
The main implementation of the model is carried out in the model directory [dem](./src/model/dem). 
The model is implemented in class [DEMModel](./src/model/dem/demModel.cpp). 
Function `DEMModel::run()` performs the simulation. We next look at some key methods in `DEMModel` in more details:

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
files (such as `.yaml`, `.msh`, `particle_locations.csv` files). 

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
    // update the point cloud (make sure that d_x is updated along with displacment)
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
Above gives the basic idea of simulation steps. For more thorough understanding of 
the implementation, interested readers can look at 
[demModel.cpp](.n/src/model/dem/demModel.cpp).

## [Installation](./tools/README.md)

### Dependencies
Core dependencies are:
  - [cmake](https://cmake.org/) (>= 3.10.2) 
  - [vtk](https://vtk.org/) (>= 7.1.1)
  - [yaml-cpp](https://github.com/jbeder/yaml-cpp) (>= 0.5.2)
  - [metis](https://github.com/KarypisLab/METIS) (>= 5.1.0)
  - MPI

Following dependencies are included in the `PeriDEM` library in `external` folder (see [external/README.md](./external/README.md) for more details):
  - [fast-cpp-csv-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/tree/master) (version included - master)
  - [fmt](https://github.com/fmtlib/fmt) (>= 7.1.3, version included - 10.2.1)
  - [nanoflann](https://github.com/jlblancoc/nanoflann) (>= 1.3.2, version included - v1.5.5)
  - [taskflow](https://github.com/taskflow/taskflow) (>= 3.7.0)
  - [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css) (>= v2.3.3) 

### Building the code
If all the dependencies are installed on the global path (e.g., `/usr/local/`), 
commands for building the PeriDEM code is as simple as
```sh
cmake   -DEnable_Documentation=OFF # or ON \
        -DEnable_Tests=ON \
        -DEnable_High_Load_Tests=OFF # ON if you want ctest to include high-load tests \
        -DDisable_Docker_MPI_Tests=ON # only for docker; OFF if you can run MPI in docker\
        -DVTK_DIR="${VTK_DIR}" # e.g., /usr/local/lib/cmake/vtk-9.3 \
        -DMETIS_DIR="${METIS_DIR}" # e.g., /usr/lib \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>
        
make -j 4
```

We refer to [tools/README.md](./tools/README.md) for further details about installing dependencies and building the library in different ubuntu releases.

### Future plans
We are trying to make PeriDEM MPI-friendly so that we can target large problems. 
We are moving in following key directions:
- MPI parallelism for Peridynamics simulations (deformation of single body subjected to external loading)
- MPI parallelism for PeriDEM simulations. Issue is distributing particles to different 
  processors and performing communication efficiently
- Asynchronous parallelism within MPI? Currently, we use `Taskflow` to perform 
  parallel for loops in a non-mpi simulation. In future, we will be interested in using 
  multithreading combined with MPI to further speed-up the simulations
- GPU parallelism?

We are looking for collaborators and HPC experts in making the most of available 
compute resource and performing truly large-scale high-fidelity granular media simulations. 
If any of the above future directions interest you or if you have new directions 
in mind, please do reach out to us.    

### Ask for help
In the past, `PeriDEM` library depended on large libraries such as `HPX`, `PCL`, `Boost` (explicitly dependence). 
We have put a lot of efforts into reducing the dependencies to absolutely minimum 
so that it is easier to build and run `PeriDEM` in different operating systems and clusters. 
At this point, only major library it depends on is `VTK` which can be compiled to 
different machines quite successfully (patience is needed in compiling `VTK` though). 
If you carefully read the information and use the scripts provided, you should be 
able to compile PeriDEM in ubuntu (>= 18.04) and mac.  

Feel free to reach out or open an issue. For more open 
discussion of issues and ideas, contact via 
[PeriDEM on Gitter](https://gitter.im/PeriDEM/community?utm_source=share-link&utm_medium=link&utm_campaign=share-link) 
or [PeriDEM on slack](peridem.slack.com) (for slack, send us an email to join). 
If you like some help, want to contribute, extend the code, or discuss new ideas, 
please do reach out to us.

## Running simulations
Assuming that the input file is `input.yaml` and all other files such as `.msh` 
file for particle/wall and particle locations file are created and their filenames 
with paths are correctly provided in `input.yaml`, we will run the problem (using 4 threads) 
```sh
<path of PeriDEM>/bin/PeriDEM -i input.yaml -nThreads 4
```

Some examples are listed below.

### Two-particle with wall
Navigate to the example directory [examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp](./examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp) 
and run the example as follows
```sh
mkdir ../out # <-- make directory for simulation output. In .yaml, we specify output path as './out'
<peridem build path>bin/PeriDEM -i input_0.yaml -nThreads 2
```

You may also use the included [problem_setup.py](./examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp/problem_setup.py) 
to modify simulation parameters and run the simulation using 
[run.sh](./examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/run.sh) 
(in directory [examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size](./examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size)). 
`run.sh` shows how different input files are created for the simulation.

> :exclamation: You may need to modify the path of `PeriDEM` executable in `run.sh` file. 

> In all `problem_setup.py` files in the example and test directory, the main function is `create_input_file()`. 
> Here we set all model parameters, create `.yaml` input file, and `.geo` files for meshing.

### Compressive test
Navigate to the example directory [examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp](./examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp) 
and run the example as follows (note that this is a computationally expensive example)
```sh
mkdir ../out 
<peridem build path>bin/PeriDEM -i input_0.yaml -nThreads 12
```

As before:
  - you can modify [problem_setup.py](./examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp/problem_setup.py), see `create_input_file()` method, to change the simulation settings 
  - run the simulation using [run.sh](./examples/PeriDEM/compressive_test/n500_circ_hex/run1/run.sh).

## Visualizing results
Simulation files `output_*.vtu` can be loaded in either [ParaView](https://www.paraview.org/) 
 or [VisIt](https://wci.llnl.gov/simulation/computer-codes/visit). 

By default, in all tests and examples, we only output the particle mesh, i.e., 
pair of nodal coordinate and nodal volume, and not the finite element mesh 
(it can be enabled by setting `Perform_FE_Out: true` within `Output` block in the input `yaml` file). 
After loading the file in ParaView, the first thing to do is to change the plot 
type from **`Surface`** to **`Point Gaussian`**. Next, a couple of things to do are:
  - Adjust the radius of circle/sphere at the nodes by going to the `Properties` 
    tab on the left side and change the value of **`Gaussian Radius`**
  - You may also want to choose the field to display. For starter, you could 
    select the `Damage_Z` variable, a ratio of **maximum bond strain in the neighborhood of a node and critical bond strain**. 
    When the `Damage_Z` value is below one at a given node, the deformation in 
    the vicinity of that node is elastic, whereas when the value is above 1, 
    it indicates there is at least one node in the neighborhood which has bond 
    strain above critical strain (meaning the bond between these two nodes is broken)
  - You may also need to rescale the plot by clicking on the **`Zoom to Data`** button in ParaView
  - Lastly, when the `Damage_Z` is very high at few nodes, you may want to rescale 
    the data to the range, say `[0,2]` or `[0,10]`, so that it is easier to identify 
    regions with elastic deformation and region with fracture.
 
## Citations
If this library was useful in your work, we recommend citing the following article:

> Jha, P.K., Desai, P.S., Bhattacharya, D. and Lipton, R., 2021. 
> **Peridynamics-based discrete element method (PeriDEM) model of granular systems involving breakage of arbitrarily shaped particles**. 
> *Journal of the Mechanics and Physics of Solids, 151*, p.104376.

You can also cite the PeriDEM using zenodo doi:

> Prashant K., J. (2024). Peridynamics-based discrete element method (PeriDEM) model of granular systems. Zenodo. https://doi.org/10.5281/zenodo.13888588

## Developers
  - [Prashant K. Jha](https://prashjha.github.io/) 
    (pjha.sci@gmail.com, prashant.jha@sdsmt.edu)
