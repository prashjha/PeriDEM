## Peridynamics-based discrete element method (PeriDEM) model of granular systems

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


## Installation

### Dependencies
Core dependencies for building the executible:

- cmake 
- boost
- hwloc
- jemalloc
- hpx
- vtk
- flann
- pcl
- yaml
- fmt (included as external library in the code)

Dependencies for running the examples:

- gmsh
- python3 
- numpy

### Building the code
If all the libraries are installed in global space, commands for building the code is as simple as
```sh
cmake   -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>

make -j 4
```

If libraries such as hpx, pcl are installed in custom paths, we will write
```sh
cmake   -DHPX_DIR="<hpx directory>/lib/cmake/HPX" \
        -DPCL_DIR="<pcl directory>" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>

make -j 4
```

### Recommendations for quick build of the code
1. Install following dependencies
```sh
sudo apt-get install libboost-dev libvtk7-dev \
		 libyaml-cpp-dev libhwloc-dev libjemalloc-dev libflann-dev
```

2. Build hpx and pcl. See shell scripts [ubuntu 18.04](https://github.com/prashjha/PeriDEM/blob/main/tools/script/docker/u1804-pd/install.sh) and [ubuntu 20.04](https://github.com/prashjha/PeriDEM/blob/main/tools/script/docker/u2004-pd/install.sh) For mac too the steps will be same assuming all other dependencies are installed using brew.

3. Build peridem [see script](https://github.com/prashjha/PeriDEM/blob/main/tools/script/build_scripts/ubuntu-18.04/install_peridem.sh)
```sh
cmake   -DHPX_DIR="<hpx directory>/lib/cmake/HPX" \
        -DPCL_DIR="<pcl directory>" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        -DCMAKE_BUILD_TYPE=Release \
        <PeriDEM source directory>

make -j 4
```

4. Running the simulation. Assuming that the input file is `input.yaml` and all other files such as `.msh` file for particle and wall and particle locations file are created and their filenames with paths are provided in the `input.yaml` file, we will run the problem (using 4 threads) 

```sh
<path of PeriDEM>/bin/PeriDEM -i input.yaml --hpx:threads=4
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


## Developers

- [Prashant K. Jha](https://prashjha.github.io/) (pjha.sci@gmail.com, pjha@utexas.edu)
