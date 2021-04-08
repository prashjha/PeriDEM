## Peridynamics-based discrete element method (PeriDEM) model of granular systems

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/eba90e085ba048cb8f895010b8f13b03)](https://app.codacy.com/gh/prashjha/PeriDEM?utm_source=github.com&utm_medium=referral&utm_content=prashjha/PeriDEM&utm_campaign=Badge_Grade_Settings)

<p align="center"> <img src="https://github.com/prashjha/PeriDEM/blob/main/assets/logo/logo.png"> </p>


Implementation of high-fidelity model of granular media that combines the advantages of peridynamics and discrete element method (DEM). The model has following advantages over existing mechanical models for granular media:

- handle intra-particle deformation and breakage/damage

- handle arbitrary shape of particle. Inter-particle contact is not specific to any shape of particle

- tunable inter-particle contact paramters

- easy to add different mechanical constitutive laws within peridynamics for individual particle deformation

For more details about the model and results, we refer to the paper:

> Prashant K. Jha, Prathamesh S. Desai, Debdeep Bhattacharya, Robert P Lipton (2020). Peridynamics-based discrete element method (PeriDEM) model of granular systems involving breakage of arbitrarily shaped particles. *Journal of the Mechanics and Physics of Solids*, Volume 151, 2021. Doi https://doi.org/10.1016/j.jmps.2021.104376.

Download pdf [here](https://prashjha.github.io/publication/jha-2020-peridem/jha-2020-peridem.pdf).

## Documentation
Doxygen generated documentation of the code can be found [here](https://prashjha.github.io/PeriDEM/). Documentation will be improved in due time.

## Examples
Some example cases. For more details, look at the `problem_setup.py` file or `input_0.yaml` file in [examples](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM).

To create example input files, the python script is provided. Python script allows parameterization of various modeling, geometrical parameters, and creating `.geo` files for `gmsh` and particle locations file. Typically, the input files consists of:

- `input.yaml` -- the main instruction file for `PeriDEM` with details about material models, particle geometries, time step, etc

- `particle_locations.csv` -- this file provides location and other details of the individual particles. Each row in the file consists of 

* `i` -- zone id that particle belongs to

* `x` -- x-coordinate of the center of the particle. Next two columns are similarly for `y` and `z` coordinates

* `r` -- radius of the particle 

* `o` -- orientation in radians. This is used to give particle (particle mesh) a rotation

- `mesh.msh` -- mesh file for the reference particle or wall. For example, in [compressive test](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/init_config/inp) example, there are total four mesh files: one each for circular and hexagon shaped particle and one each for fixed and mobile wall

### Two-particle tests

| <img src="assets/two_particle_circ_no_damp.gif" width="200"> | <img src="assets/two_particle_circ_damp.gif" width="200"> |
| :---: | :---: |
| [Circular without damping](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_no_damp/) | [Circular without damping](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_damp/) |

| <img src="assets/two_particle_circ_diff_material.gif" width="200"> | <img src="assets/two_particle_circ_damp_diff_radius.gif" width="200"> | <img src="assets/two_particle_circ_diff_radius_diff_material.gif" width="200"> |
| :---: | :---: | :---: |
| [Different materials](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_diff_material/) | [Different radius](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_damp_diff_radius/) | [Different radius different material](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_diff_radius_diff_material/) |

### Two-particle with wall test

| <img src="https://github.com/prashjha/PeriDEM/blob/main/assets/two_particle_wall_concave_diff_material_diff_size.gif" width="400"> | 
| :---: | 
| [Concave particles](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/) |

### Compressive test

Setup for this test consists of 502 circular and hexagonal shaped particles of varrying radius and orientation within a rectangle container. The top wall of the container is moving downward with prescribed speed resulting in compression of the particle system. The quantity of interest is the compressive strength of media; it is expected that the moving wall register reaction for with increasing penetration, however, after a certain amount of compression of media, the damage will initiate in individual particles especially those connected by force chain resulting in yielding of the media. For more details, we refer to [Jha et al 2021](https://prashjha.github.io/publication/jha-2020-peridem/)

| <img src="assets/compressive_test_cir_hex_n500.jpg" width="600"> | 
| :---: | 
| [Compressive test setup](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/) |


| <img src="assets/compressive_test_reaction_force_n500.jpg" width="600"> | 
| :---: | 
| **Top**: Plot of reaction force per unit area on top wall. **Bottom**: Particle state at four times. Color shows the damage at nodes. Damage 1 or above indicates the presence of broken bonds in the neighborhood of a node. |

| <img src="https://github.com/prashjha/PeriDEM/blob/main/assets/compressive_test.gif" width="600"> | 
| :---: | 
| Compressive test simulation |

## Brief implementation details
Main implementation of the model is carried out in the model directory [dem](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem). The model is implemented in class `DEMModel`, see [demModel.cpp](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem/demModel.cpp). Function `DEMModel::run()` performs the simulation. We next look at this and few more function in more details:

### DEMModel::run()
This function does three tasks:
```c++
// initialize data
init();

// check for restart
if (d_modelDeck_p->d_isRestartActive)
  restart(deck);

// integrate in time
integrate();
```

See `DEMModel::init()` where we setup the simulation by reading input files.

### DEMModel::integrate()
Basic steps in  `DEMModel::integrate()` are 
```c++
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
```

In `DEMModel::integrateStep()`, we either follow central-difference scheme, in `DEMModel::integrateCD()`, or velocity-verlet scheme, in `DEMModel::integrateVerlet()`. As an example, the steps in `DEMModel::integrateCD()` are

```c++
const auto dt = d_modelDeck_p->d_dt;
const auto dim = d_modelDeck_p->d_dim;

// update velocity and displacement
auto f = hpx::parallel::for_loop(
    hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
    d_fPdCompNodes.size(),
    [this, dt, dim](boost::uint64_t II) {
      auto i = this->d_fPdCompNodes[II];

      const auto rho = this->getDensity(i);
      const auto &fix = this->d_fix[i];

      for (int dof = 0; dof < dim; dof++) {
        if (util::methods::isFree(fix, dof)) {
          this->d_v[i][dof] += (dt / rho) * this->d_f[i][dof];
          this->d_u[i][dof] += dt * this->d_v[i][dof];
          this->d_x[i][dof] += dt * this->d_v[i][dof];
        }
      }
    } // loop over nodes
);    // end of parallel for loop
f.get();

d_n++;
d_time += dt;

// update displacement bc
computeExternalDisplacementBC();

// compute force
computeForces();
```

### DEMModel::computeForces()
In this function, we compute internal and external forces at each node of a particle. This function looks like
```c++
// update tree for search
auto pt_cloud_update_time = d_nsearch_p->updatePointCloud(d_x, true);
pt_cloud_update_time += d_nsearch_p->setInputCloud();

// reset all forces

// compute peridynamic force (internal force)
computePeridynamicForces();

// compute contact force
computeContactForces();

// compute external force (e.g. due to container motion, etc)
computeExternalForces();
```

### Further reading
Above gives the basic idea of simulation steps. For more thorough understanding of the implementation, interested readers can look at [demModel.cpp](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem/demModel.cpp). 

## Installation

### Dependencies
Core dependencies for building the executible:

- [cmake](https://cmake.org/) 
  - recommend to install using `apt-get`
- [boost](https://www.boost.org/) 
  - recommend to install using `apt-get`
  - required for building YAML, HPX, and possibly PCL libraries
- [hwloc](https://github.com/open-mpi/hwloc) 
  - recommend to install using `apt-get`
  - required to build HPX library
- [jemalloc](https://github.com/jemalloc/jemalloc)
  - recommend to install using `apt-get`
  - required to build HPX library
- [hpx](https://github.com/STEllAR-GROUP/hpx)
  - use build script to install
  - used for multi-threading calculations
- [vtk](https://vtk.org/)
  - recommend to install using `apt-get`
  - required to output simulation results in `.vtu` format
- [flann](https://github.com/mariusmuja/flann)
  - recommend to install using `apt-get`
  - required to build PCL library
- [pcl](https://pointclouds.org/)
  - use build script to install
  - required for tree search
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
  - recommend to install using `apt-get`
  - required to parse input file
- [fmt](https://github.com/fmtlib/fmt)
  - included as external library in the code
  - required to output formatted strings

Dependencies for running the examples:

- [gmsh](https://gmsh.info/)
  - recommend to install using `apt-get`
  - required to build the mesh of various objects in the test
- [python3](https://www.python.org/)
  - required to run the test python scripts
- [numpy](https://numpy.org/)
  - required to run the test python scripts

### Building the code
If all the libraries are installed in global space, commands for building the PeriDEM code is as simple as
```sh
cmake   -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>

make -j 4
```

If libraries such as hpx and pcl are installed in custom paths, we will write
```sh
cmake   -DHPX_DIR="<hpx directory>/lib/cmake/HPX" \
        -DPCL_DIR="<pcl directory>/share/pcl-1.11" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>

make -j 4
```

> :exclamation: Note that for HPX we provide `<hpx directory>/lib/cmake/HPX` and for PCL we provide `<pcl directory>/share/pcl-1.11`. Here, `<hpx directory>` and `<pcl directory>` are the root path of location where HPX and PCL are installed. 


### Recommendations for quickly building the PeriDEM library
1. Install following dependencies:

  - If you want to install minimal set of libraries

```sh
sudo apt-get install -y wget lzip \
  cmake liblapack-dev libblas-dev libopenmpi-dev \
  doxygen doxygen-latex graphviz ghostscript \
  gfortran libmpfr-dev libgmp-dev \
  libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
  libvtk7-dev gmsh libflann-dev python3-pip 

pip3 install numpy
```

  - If you want to install everything that is installed in docker image for testing the compilation

```sh
sudo apt-get update 
  
sudo apt-get install -y build-essential ubuntu-dev-tools rpm gcovr \
  git wget lzip \
  cmake autoconf libtool pkg-config \
  liblapack-dev libblas-dev libopenmpi-dev \
  doxygen doxygen-latex graphviz ghostscript \
  gfortran libmpfr-dev libgmp-dev \
  libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
  libvtk7-dev gmsh libflann-dev python3-pip 

pip3 install numpy pyvista pandas
```

> :zap: Above is also available in the bash script [install_base.sh](https://github.com/prashjha/PeriDEM/blob/main/tools/script/build_scripts/ubuntu-18.04/install_base.sh). Using this you can simply run:

```sh
./install_base.sh
```

2. Build hpx and pcl. For Ubuntu 18.04, you can use the [install_libs.sh](https://github.com/prashjha/PeriDEM/blob/main/tools/script/build_scripts/ubuntu-18.04/install_libs.sh) script and run
```sh
./install_libs.sh
```

For Ubuntu 20.04, above script should work. You may also get some help from the script we used to build the base docker images. We installed HPX and PCL in docker image using the scripts:

- [ubuntu 18.04 script](https://github.com/prashjha/PeriDEM/blob/main/tools/script/docker/u1804-pd/install.sh) 
- [ubuntu 20.04 script](https://github.com/prashjha/PeriDEM/blob/main/tools/script/docker/u2004-pd/install.sh)

For mac the steps will be same assuming 
  - key dependencies are installed using homebrew
```sh
brew install boost vtk yaml-cpp hwloc jemalloc flann cmake
```

  - HPX and PCL installed similar to above

> :warning: With recent update in homebrew where they changed the current version of boost, I am no longer able to build the HPX and PCL in mac Big Sur 11.2.1. 

3. Build peridem using [install_peridem.sh](https://github.com/prashjha/PeriDEM/blob/main/tools/script/build_scripts/ubuntu-18.04/install_peridem.sh)
```sh
./install_peridem.sh
```

> :warning: Be sure to modify the paths where HPX and PCL are installed in the install_peridem.sh script!

Or if you have already cloned the PeriDEM and are in the root directory of PeriDEM, simply run following in the terminal:
```sh
mkdir build && cd build 
cmake   -DHPX_DIR="<hpx directory>/lib/cmake/HPX" \
        -DPCL_DIR="<pcl directory>/share/pcl-1.11" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        ../.

make -j 4

ctest --verbose
```

### Build scripts
Some shell scripts are provided that may help in building the code on ubuntu 18.04, 20.04, and mac. 

### Docker
We also provide the docker image of the peridem code. The docker image is built using the three layers

- layer 1 (u1804-base or u2004-base) -- base ubuntu 18.04 or ubuntu 20.04 with some essential libraries installed

- layer 2 (u1804-pd or u2004-pd) -- built hpx and pcl on layer 1

- layer 3 (PeriDEM) -- built PeriDEM on layer 2


Dockerfile associated to all layers listed above can be found in [build scripts](https://github.com/prashjha/PeriDEM/blob/main/tools/script/docker). Image corresponding to layer 2 above can be downloaded from the dockerhub:

- for ubuntu 18.04 
        - link: https://hub.docker.com/r/prashjha/u1804-pd
        - `docker pull prashjha/u1804-pd`

- for ubuntu 20.04 
        - link: https://hub.docker.com/r/prashjha/u2004-pd
        - `docker pull prashjha/u2004-pd`

### Future plans to remove some dependencies
PeriDEM currently depends on 4 major libraries: Boost, VTK, HPX, PCL

- boost is not used directly but is required in building HPX, PCL, YAML-CPP 
- if there are lightweight vtu writer and reader, dependency on VTK can be avoided
- HPX is used for the multi-threading operation. Currently, HPX is little troublesome to build, especially in clusters
- Only one feature of PCL is utilized currently. We do try to avoid building many libraries within PCL, but it will be nice to have more lightweight library that performs tree search and that can be included as external library in the code

### Seek help
Building PeriDEM is not a piece of cake! This I know from my experience trying to build the dependencies in different operating systems and with different configurations. If you are stuck, feel free to reach out, or open an issue. 

## Running the simulation 
Assuming that the input file is `input.yaml` and all other files such as `.msh` file for particle and wall and particle locations file are created and their filenames with paths are provided in the `input.yaml` file, we will run the problem (using 4 threads) 

```sh
<path of PeriDEM>/bin/PeriDEM -i input.yaml --hpx:threads=4
```

Some examples:

### Two particle with wall

Navigate to the example directory `examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp` and run the example as follows
```sh
mkdir ../out # <-- make directory for simulation output. In .yaml, we specify output path as './out'
<peridem build path>bin/PeriDEM -i input_0.yaml --hpx:threads=2
```

You may also use the included [problem_setup.py](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp/problem_setup.py) to modify some parameters and [run.sh](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/run.sh) (in directoy `examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size`) to run the simulation. `run.sh` shows how different input files are created for simulation.

> :exclamation: You will need to modify the path of `PeriDEM` executible in `run.sh` file, see variable `execsrc`. 

> In all `problem_setup.py` files in the example and test directory, the main function is `create_input_file()`. Here we set all model parameters, create `.yaml` input file, and `.geo` files for meshing.

### Compressive test

Navigate to the example directory `examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp` and run the example as follows (note that this is expensive example)
```sh
mkdir ../out 
<peridem build path>bin/PeriDEM -i input_0.yaml --hpx:threads=12
```

As in the case of previous example:

 - you can modify [problem_setup.py](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp/problem_setup.py) to change the simulation settings 
 - and then use the [run.sh](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/run1/run.sh) (in directory `examples/PeriDEM/compressive_test/n500_circ_hex/run1`) to run the new simulation

> In all `problem_setup.py` files in the example and test directory, the main function is `create_input_file()`. Here we set all model parameters, create `.yaml` input file, and `.geo` files for meshing.

## Developers

- [Prashant K. Jha](https://prashjha.github.io/) (pjha.sci@gmail.com, pjha@utexas.edu)
