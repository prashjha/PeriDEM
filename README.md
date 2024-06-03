# PeriDEM - Peridynamics-based discrete element model of granular systems

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/eba90e085ba048cb8f895010b8f13b03)](https://app.codacy.com/gh/prashjha/PeriDEM?utm_source=github.com&utm_medium=referral&utm_content=prashjha/PeriDEM&utm_campaign=Badge_Grade_Settings) [![CircleCI](https://circleci.com/gh/prashjha/PeriDEM.svg?style=shield)](https://circleci.com/gh/prashjha/PeriDEM) [![codecov](https://codecov.io/gh/prashjha/PeriDEM/branch/main/graph/badge.svg?token=JyVHXtXJWS)](https://codecov.io/gh/prashjha/PeriDEM) [![GitHub release](https://img.shields.io/github/release/prashjha/PeriDEM.svg)](https://GitHub.com/prashjha/PeriDEM/releases/) [![GitHub license](https://img.shields.io/github/license/prashjha/PeriDEM.svg)](https://github.com/prashjha/PeriDEM/blob/main/LICENSE) [![GitHub issues](https://img.shields.io/github/issues/prashjha/PeriDEM.svg)](https://github.com/prashjha/PeriDEM/issues) [![Join the chat at https://gitter.im/PeriDEM/community](https://badges.gitter.im/PeriDEM/community.svg)](https://gitter.im/PeriDEM/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![GitHub repo size](https://img.shields.io/github/repo-size/prashjha/PeriDEM.svg)](https://GitHub.com/prashjha/PeriDEM/) [![DOI](https://zenodo.org/badge/334516168.svg)](https://zenodo.org/badge/latestdoi/334516168)


<p align="center"> <img src="assets/logo/logo.png"> </p>

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
  - [Visualizing results](#Visualizing-results)
  - [Developers](#Developers)

## Introduction
Implementation of the high-fidelity model of granular media that combines the advantages of peridynamics and discrete element method (DEM). The model has the following advantages over existing mechanical models for granular media:
  - handle intra-particle deformation and breakage/damage
  - handle arbitrary shape of the particle. Inter-particle contact is not specific to any shape of the particle
  - tunable inter-particle contact parameters
  - easy to add different mechanical constitutive laws within peridynamics for individual particle deformation

For more details about the model and results, we refer to the paper:

> Prashant K. Jha, Prathamesh S. Desai, Debdeep Bhattacharya, Robert P Lipton (2020). **Peridynamics-based discrete element method (PeriDEM) model of granular systems involving breakage of arbitrarily shaped particles**. *Journal of the Mechanics and Physics of Solids, 151*, p.104376. Doi https://doi.org/10.1016/j.jmps.2021.104376.

Download pdf [here](https://prashjha.github.io/publication/jha-2020-peridem/jha-2020-peridem.pdf).

We have created channels on various platforms: 
- [PeriDEM on Gitter](https://gitter.im/PeriDEM/community?utm_source=share-link&utm_medium=link&utm_campaign=share-link)
  * Gitter is absolutely open and easy to join.
- [PeriDEM on slack](peridem.slack.com)
  * Send us an email if interested in joining the workspace.

## Documentation
Doxygen generated documentation of the code can be found [here](https://prashjha.github.io/PeriDEM/). Documentation will be improved in due time.

## Tutorial
We explain the setting-up of simulations in further details in [tutorial](tutorial/README.md). We consider `two-particle` test setup with non-circular particles and `compressive-test` to discuss the various aspects of simulations.

## Examples
We next highlight some key examples. For more details, look at the `create_input_file()` within `problem_setup.py` or `input_0.yaml` in [examples](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM).

To create input files, the python script is provided. Python script allows easy parameterization of various modeling and geometrical parameters and creating `.geo` files for `gmsh` and particle locations file. Typically, the input files consists of:

  - `input.yaml` - the main instruction file for `PeriDEM` with details about material models, particle geometries, time step, etc
  - `particle_locations.csv` - this file provides location and other details of the individual particles. Each row in the file consists of 
    * `i` - zone id that particle belongs to
    * `x` - x-coordinate of the center of the particle. Next two columns are similarly for `y` and `z` coordinates
    * `r` - radius of the particle 
    * `o` - orientation in radians. This is used to give particle (particle mesh) a rotation
  - `mesh.msh` - mesh file for the reference particle or wall. For example, in [compressive test](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/init_config/inp) example, there are four mesh files: one each for the circular and hexagon-shaped particle and one each for the fixed and mobile wall.

### Two-particle tests

| <img src="assets/two_particle_circ_no_damp.gif" width="200"> | <img src="assets/two_particle_circ_damp.gif" width="200"> |
| :---: | :---: |
| [Circular without damping](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_no_damp/) | [Circular without damping](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_damp/) |

| <img src="assets/two_particle_circ_diff_material.gif" width="200"> | <img src="assets/two_particle_circ_damp_diff_radius.gif" width="200"> | <img src="assets/two_particle_circ_diff_radius_diff_material.gif" width="200"> |
| :---: | :---: | :---: |
| [Different materials](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_diff_material/) | [Different radius](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_damp_diff_radius/) | [Different radius different material](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles/circ_diff_radius_diff_material/) |

### Two-particle with wall test

| <img src="assets/two_particle_wall_concave_diff_material_diff_size.gif" width="400"> | 
| :---: | 
| [Concave particles](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/) |

### Compressive tests
Setup for this test consists of 502 circular and hexagonal-shaped particles of varying radius and orientation inside a rectangle container. The container's top wall is moving downward at a prescribed speed, resulting in the compression of the particle system. The quantity of interest is the compressive strength of the media. The reaction force (downward) on the moving wall should increase with the increasing penetration of this wall; however, after a certain amount of compression of the media, the damage will initiate in individual particles, especially those connected by force chains, resulting in the yielding of the system. For more details, we refer to [Jha et al. 2021](https://prashjha.github.io/publication/jha-2020-peridem/)

| <img src="assets/compressive_test_cir_hex_n500.jpg" width="600"> | 
| :---: | 
| [Compressive test setup](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/) |

| <img src="assets/compressive_test_reaction_force_n500.jpg" width="600"> | 
| :---: | 
| **Top**: Plot of reaction force per unit area on the top wall. **Bottom**: Particle state at four times. Color shows the damage at nodes. Damage 1 or above indicates the presence of broken bonds in the neighborhood of a node. |

| <img src="assets/compressive_test.gif" width="600"> | 
| :---: | 
| Compressive test simulation | 

## Brief implementation details
The main implementation of the model is carried out in the model directory [dem](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem). The model is implemented in class `DEMModel`, see [demModel.cpp](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem/demModel.cpp). Function `DEMModel::run()` performs the simulation. We next look at some key methods in `DEMModel` in more details:

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

In `DEMModel::init()`, the simulation is prepared by reading the input files (such as `.yaml`, `.msh`, `particle_locations.csv` files). 

### DEMModel::integrate()
Key steps in  `DEMModel::integrate()` are 
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

In `DEMModel::integrateStep()`, we either utilize the central-difference scheme, implemented in `DEMModel::integrateCD()`, or the velocity-verlet scheme, implemented in `DEMModel::integrateVerlet()`. As an example, we look at `DEMModel::integrateCD()` method below:
```c++
// update velocity and displacement
const auto dt = d_modelDeck_p->d_dt;
const auto dim = d_modelDeck_p->d_dim;

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

d_n++;
d_time += dt;

// update displacement bc
computeExternalDisplacementBC();

// compute force
computeForces();

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
        }
      }
    } // loop over nodes
  ); // for_each

  executor.run(taskflow).get();
}
```

### DEMModel::computeForces()
The key method in time integration is `DEMModel::computeForces()`. In this function, we compute internal and external forces at each node of a particle and also account for the external boundary conditions. This function looks like
```c++
bool dbg_condition = d_n % d_infoN == 0;

log("  Compute forces \n", 2, dbg_condition, 3);

// update the point cloud (make sure that d_x is updated along with displacment)
auto pt_cloud_update_time = d_nsearch_p->updatePointCloud(d_x, true);
pt_cloud_update_time += d_nsearch_p->setInputCloud();
tree_compute_time += pt_cloud_update_time;
log(fmt::format("    Point cloud update time (ms) = {} \n",
                  pt_cloud_update_time), 2, dbg_condition, 3);

// reset force
auto t1 = steady_clock::now();
tf::Executor executor(util::parallel::getNThreads());
tf::Taskflow taskflow;

taskflow.for_each_index(
  (std::size_t) 0, d_x.size(), (std::size_t) 1, 
    [this](std::size_t i) { this->d_f[i] = util::Point(); }
); // for_each

executor.run(taskflow).get();

log(fmt::format("    Force reset time (ms) = {} \n",
                  util::methods::timeDiff(t1, steady_clock::now())), 2, dbg_condition, 3);

// compute peridynamic forces
t1 = steady_clock::now();
computePeridynamicForces();
auto pd_time = util::methods::timeDiff(t1, steady_clock::now());
pd_compute_time += pd_time;
log(fmt::format("    Peridynamics force time (ms) = {} \n", pd_time), 2, dbg_condition, 3);

// compute contact forces between particles
t1 = steady_clock::now();
computeContactForces();
auto contact_time = util::methods::timeDiff(t1, steady_clock::now());
contact_compute_time += contact_time;
log(fmt::format("    Contact force time (ms) = {} \n", contact_time), 2, dbg_condition, 3);

// Compute external forces
t1 = steady_clock::now();
computeExternalForces();
auto extf_time = util::methods::timeDiff(t1, steady_clock::now());
extf_compute_time += extf_time;
log(fmt::format("    External force time (ms) = {} \n", extf_time), 2, dbg_condition, 3);
```

### Further reading
Above gives the basic idea of simulation steps. For more thorough understanding of the implementation, interested readers can look at [demModel.cpp](https://github.com/prashjha/PeriDEM/blob/main/src/model/dem/demModel.cpp).

## Installation

### Dependencies
Core dependencies are:
  - [cmake](https://cmake.org/) (>= 3.10.2) 
    * recommend to install using `apt-get` or `brew`
  - [boost](https://www.boost.org/) (>= 1.65.1)
    * recommend to install using `apt-get` or `brew`
    * required for building YAML
  - [vtk](https://vtk.org/) (>= 7.1.1)
    * recommend to build using script [install_vtk_alone.sh](tools/build_scripts/ubuntu/install_vtk_alone.sh) or [install_libs.sh](tools/build_scripts/ubuntu/install_libs.sh)
    * required to output simulation results in `.vtu` format
  - [nanoflann](https://github.com/jlblancoc/nanoflann) (>= 1.3.2)
    * included as external library in the code
    * required for neighbor search
  - [yaml-cpp](https://github.com/jbeder/yaml-cpp) (>= 0.5.2)
    * recommend to install using `apt-get` or `brew`
    * required to parse input file
  - [fmt](https://github.com/fmtlib/fmt) (>= 7.1.3)
    * included as external library in the code
    * required to output formatted strings
  - [metis](https://github.com/KarypisLab/METIS) (>= 5.1.0)
    * recommend to install using `apt-get` or `brew`. If using `apt-get`, recommend to create symlink to `libmetis.so` file in `/usr/lib/` directory. See towards end in [install_base.sh](tools/build_scripts/ubuntu/install_base.sh). This helps `cmake` locate metis library. 
    * required to partition the mesh
  - MPI
    * for parallel simulations

Dependencies for running the examples:
  - [gmsh](https://gmsh.info/) (>= 3.0.6)
    * recommend to install using `apt-get` or `brew`
    * required to build the mesh of various objects in the test
  - [python3](https://www.python.org/)
    * required to run the test python scripts
  - [numpy](https://numpy.org/)
    * required to run the test python scripts

### Building the code
If all the dependencies are installed on the global path (e.g., `/usr/local/`), commands for building the PeriDEM code is as simple as
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

### Recommendations for quick build
1. Install most of the dependencies using `apt-get`:
```sh
# get ubuntu codename
UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"

# Install essentials
sudo apt-get update 

sudo apt-get install -y \
    git less wget curl lzip bzip2 unzip autoconf libtool pkg-config \
    rpm gcovr ruby-coveralls libproj-dev m4 \
    software-properties-common ubuntu-dev-tools \
    gfortran make build-essential libssl-dev clang-format-10 clang-tidy \
    openssh-server rsync \
    doxygen doxygen-latex graphviz ghostscript \
    lldb valgrind \
    python3-pip \
    ibopenmpi-dev openmpi-bin \
    libblas-dev liblapack-dev libmpfr-dev libgmp-dev \
    libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
    libgmsh-dev gmsh libflann-dev \
    libmetis-dev \
    libtbb-dev libasio-dev libglvnd-dev \
    libjpeg-dev zlib1g-dev # for pillow python library \
    libgl1-mesa-dev # for VTK

if [ "${UBUNTU_CODENAME}" = "focal" ]; then
  sudo apt-get install -y gcc-10 g++-10 
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100 
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100
  echo "Display gcc version"  && gcc -v
fi

# clean
sudo apt-get autoremove -y && sudo apt-get autoclean -y

# python packages
pip3 install numpy pyvista pandas

# create symlink for metis
sudo ln -sf /usr/lib/x86_64-linux-gnu/libmetis.so /usr/lib/libmetis.so 
echo "Display metis header and lib files" 
ls /usr/include/metis* 
ls /usr/lib/libmetis*
```

> :zap: Above is also available in the bash script [install_base.sh](tools/build_scripts/ubuntu/install_base.sh).

If you are in mac, the essential libraries can be installed via `brew`
```sh
brew install cmake boost vtk yaml-cpp metis gmsh openmpi
# if building documentation, we also need doxygen
brew install doxygen
```

2. Build peridem using [compile_peridem.sh](tools/build_scripts/ubuntu/compile_peridem.sh) or in mac use script [compile_peridem.sh](tools/build_scripts/mac/compile_peridem.sh):
```sh
# may have to do `chmod +x compile_peridem.sh` first
./compile_peridem.sh
```

> :warning: Be sure to modify `compile_peridem.sh` file to specify the correct paths where VTK and Metis are installed!

Alternatively, if you have already cloned the PeriDEM library and are in the root directory of PeriDEM, run following in the terminal:
```sh
mkdir -p build && cd build 
cmake   -DEnable_Documentation=OFF # or ON \
        -DEnable_Tests=ON \
        -DEnable_High_Load_Tests=OFF # ON if you want ctest to include high-load tests \
        -DDisable_Docker_MPI_Tests=ON # only for docker; OFF if you can run MPI in docker\
        -DVTK_DIR="${VTK_DIR}" # e.g., /usr/local/lib/cmake/vtk-9.3 \
        -DMETIS_DIR="${METIS_DIR}" # e.g., /usr/lib \
        -DCMAKE_BUILD_TYPE=Release \
        ../.

make -j 4

# ctest is recommended post building
ctest --verbose
```

### Docker
For `circle-ci` testing, we use docker images `prashjha/peridem-base-focal:latest` (`ubuntu 20.04`) and `prashjha/peridem-base-bionic:latest` (`ubuntu 18.04`). The associated dockerfiles and links to pull from docker hub are:
  - for `focal` (`ubuntu 20.04`)
    * link: https://hub.docker.com/r/prashjha/peridem-base-focal
    * `docker pull prashjha/peridem-base-focal:latest`
    * [dockerfile](https://github.com/prashjha/dockerimages/blob/main/peridem-base-focal/Dockerfile)
  - for `bionic` (`ubuntu 18.04`)
      * link: https://hub.docker.com/r/prashjha/peridem-base-bionic
      * `docker pull prashjha/peridem-base-bionic:latest`
      * [dockerfile](https://github.com/prashjha/dockerimages/blob/main/peridem-base-bionic/Dockerfile)
  - clion setup for remote development
      * [Dockerfile-peridem-clion-focal](tools/docker/docker-clion-remote/Dockerfile-peridem-clion-focal)
      * [Dockerfile-peridem-clion-bionic](tools/docker/docker-clion-remote/Dockerfile-peridem-clion-bionic)
  - for example of building `PeriDEM` using docker
    * [Dockerfile-test-peridem-focal](tools/docker/compile-peridem-using-docker/Dockerfile-test-peridem-focal)
    * [Dockerfile-test-peridem-bionic](tools/docker/compile-peridem-using-docker/Dockerfile-test-peridem-bionic)
    * These dockerfiles call [compile_peridem.sh](tools/docker/compile-peridem-using-docker/compile_peridem.sh) script

In [Packages](https://github.com/prashjha?tab=packages&repo_name=PeriDEM), docker images of PeriDEM built on ubuntu-20.04 (`prashjha/peridem-base-focal:latest`) is provided.

### Future plans
We are trying to make PeriDEM MPI-friendly so that we can target large problems. We are moving in following key directions:
- MPI parallelism for Peridynamics simulations (deformation of single body subjected to external loading)
- MPI parallelism for PeriDEM simulations. Issue is distributing particles to different processors and performing communication efficiently
- Asynchronous parallelism within MPI? Currently, we use `Taskflow` to perform parallel for loops in a non-mpi simulation. In future, we will be interested in using multithreading combined with MPI to further speed-up the simulations
- GPU parallelism?

We are looking for collaborators and HPC experts in making the most of available compute resource and performing truly large-scale high-fidelity granular media simulations. If any of the above future directions interest you or if you have new directions in mind, please do reach out to us.    

### Ask for help
In the past, `PeriDEM` library depended on large libraries such as `HPX`, `PCL`, `Boost` (explicitly dependents). 
We have put a lot of efforts into reducing the dependencies to absolutely minimum so that it is easier to build and run `PeriDEM` in different operating systems and clusters. 
At this point, only major library it depends on is `VTK` which can be compiled to different machines quite successfully (patience is needed in compiling `VTK` though).  
If you carefully read the information and use the scripts provided, you should be able to compile PeriDEM in ubuntu (>= 18.04) and mac.  

Still, if you are stuck, feel free to reach out or open an issue. For more open discussion of issues and ideas, contact via [PeriDEM on Gitter](https://gitter.im/PeriDEM/community?utm_source=share-link&utm_medium=link&utm_campaign=share-link) or [PeriDEM on slack](peridem.slack.com) (for slack, send me an email to join). If you like some help, want to contribute, extend the code, or discuss new ideas, please do reach out to us.

## Running simulations
Assuming that the input file is `input.yaml` and all other files such as `.msh` file for particle/wall and particle locations file are created and their filenames with paths are correctly provided in `input.yaml`, we will run the problem (using 4 threads) 
```sh
<path of PeriDEM>/bin/PeriDEM -i input.yaml -nThreads 4
```

Some examples are listed below.

### Two-particle with wall
Navigate to the example directory `examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp` and run the example as follows
```sh
mkdir ../out # <-- make directory for simulation output. In .yaml, we specify output path as './out'
<peridem build path>bin/PeriDEM -i input_0.yaml -nThreads 2
```

You may also use the included [problem_setup.py](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/inp/problem_setup.py) to modify simulation parameters and run the simulation using [run.sh](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size/run.sh) (in directoy `examples/PeriDEM/two_particles_wall/concave_diff_material_diff_size`). `run.sh` shows how different input files are created for the simulation.

> :exclamation: You will need to modify the path of `PeriDEM` executible in `run.sh` file, see variable `execsrc`. 


> In all `problem_setup.py` files in the example and test directory, the main function is `create_input_file()`. Here we set all model parameters, create `.yaml` input file, and `.geo` files for meshing.

#### Important remark on modifying input.yaml file
To test the examples quickly, you can directly modify the `input.yaml` and re-run the simulation as shown above. For example, you can alter `Final_Time`, `Time_Steps`, `Contact_Radius_Factor`, `Kn`, and other fields in the yaml file. 

However, some care is required when changing the geometrical details of particles and walls in the `input.yaml` file. If you change these details in the `.yaml` file, you will have to ensure that the `.msh` file correspond to the new geometry. 

Except geometrical parameters of walls and particles, rest of the parameters in `input.yaml` can be modified.

> In due time, we will provide more information on setting up input files and covering all aspects of the simulation.

### Compressive test
Navigate to the example directory `examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp` and run the example as follows (note that this is an expensive example)
```sh
mkdir ../out 
<peridem build path>bin/PeriDEM -i input_0.yaml -nThreads 12
```

As before:
  - you can modify [problem_setup.py](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/run1/inp/problem_setup.py), see `create_input_file()` method, to change the simulation settings 
  - run the simulation using [run.sh](https://github.com/prashjha/PeriDEM/blob/main/examples/PeriDEM/compressive_test/n500_circ_hex/run1/run.sh) (in directory `examples/PeriDEM/compressive_test/n500_circ_hex/run1`).

### Compute times for various examples (From old version of the code!)
For reference, we list the compute times for various examples.
  - `T` is the total compute time in units of `second`
  - `T(n)` means compute time when running the example with `n` threads.

| Test | T(1) | T(2) | T(4) | T(8) |
| :--- | :---: |  :---: |  :---: |  :---: |
| two_particles/circ_damp | 143.7 | 95.1 | 76.4 | 78.6 |
| two_particles/circ_damp_diff_radius | 164 | 114.6 | 96.7 | 99.4 |
| two_particles/circ_diff_material | 287.7 | 190.1 | 152.7 | 160 |
| two_particles/circ_diff_radius_diff_material | 329.1 | 229.4 | 195.3 | 200 |
| two_particles/circ_no_damp | 143.8 | 94.5 | 76.7 | 78.5 |
| two_particles_wall/concave_diff_material_diff_size | 2749.9 | 1534.6 | 980.8 | 691.1 |

## Visualizing results
Simulation files `output_*.vtu` can be loaded in either [ParaView](https://www.paraview.org/) (tested on 5.4.1 and later versions) or [VisIt](https://wci.llnl.gov/simulation/computer-codes/visit) (tested on 2.13.3). 

By default, in all tests and examples, we only output the particle mesh, i.e., pair of nodal coordinate and nodal volume, and not the finite element mesh (it can be enabled by setting `Perform_FE_Out: true` within `Output` block in the input `yaml` file). After loading the file in ParaView, the first thing to do is to change the plot type from **`Surface`** to **`Point Gaussian`**. Next, a couple of things to do are:
  - Adjust the radius of circle/sphere at the nodes by going to the `Properties` tab on the left side and change the value of **`Gaussian Radius`**
  - You may also want to choose the field to display. For starter, you could select the `Damage_Z` variable, a ratio of **maximum bond strain in the neighborhood of a node and critical bond strain**. When the `Damage_Z` value is below one at a given node, the deformation in the vicinity of that node is elastic, whereas when the value is above 1, it indicates there is at least one node in the neighborhood which has bond strain above critical strain (meaning the bond between these two nodes is broken)
  - You may also need to rescale the plot by clicking on the **`Zoom to Data`** button in ParaView
  - Lastly, when the `Damage_Z` is very high at few nodes, you may want to rescale the data to the range, say `[0,2]` or `[0,10]`, so that it is easier to identify regions with elastic deformation and region with fracture.
 
## Citations
If this library was useful in your work, we recommend citing the following article:

> Jha, P.K., Desai, P.S., Bhattacharya, D. and Lipton, R., 2021. **Peridynamics-based discrete element method (PeriDEM) model of granular systems involving breakage of arbitrarily shaped particles**. *Journal of the Mechanics and Physics of Solids, 151*, p.104376.

You can also cite the PeriDEM using zenodo doi:

> http://doi.org/10.5281/zenodo.4733259

## Developers
  - [Prashant K. Jha](https://prashjha.github.io/) (pjha.sci@gmail.com, prashant.jha@austin.utexas.edu, prashant.jha@port.ac.uk)
