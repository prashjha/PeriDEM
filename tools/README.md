# PeriDEM: Installation

## Dependencies
Core dependencies are:
  - [cmake](https://cmake.org/) (>= 3.10.2) 
    * recommend to install using `apt-get` or `brew`
  - [vtk](https://vtk.org/) (>= 7.1.1)
    * recommend to build using script [install_vtk_alone.sh](./compile_scripts/ubuntu/install_vtk_alone.sh) or [install_cmake_and_vtk.sh](./compile_scripts/ubuntu/install_cmake_and_vtk.sh)
    * required to output simulation results in `.vtu` format
  - [yaml-cpp](https://github.com/jbeder/yaml-cpp) (>= 0.5.2)
    * recommend to install using `apt-get` or `brew`
    * required to parse input file
  - [metis](https://github.com/KarypisLab/METIS) (>= 5.1.0)
    * recommend to install using `apt-get` or `brew`. If using `apt-get`, recommend 
      to create symlink to `libmetis.so` file in `/usr/lib/` directory; 
      see towards end in script [install_apt-get_libs.sh](./compile_scripts/ubuntu/install_apt-get_libs.sh). 
      This helps `cmake` locate metis library. 
    * required to partition the mesh
  - MPI
    * for parallel simulations
  - [libflann-dev](https://www.cs.ubc.ca/research/flann/) (1.9)
    * recommend to install using `apt-get` or `brew`
    * needed by `nanoflann` library 

Following dependencies are included in the `PeriDEM` library in `external` folder (see [PeriDEM/external/README.md](../external/README.md) for more details):
  - [fast-cpp-csv-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/tree/master) (version included - master)
    * required to read `.csv` files
  - [fmt](https://github.com/fmtlib/fmt) (>= 7.1.3, version included - 10.2.1)
    * included as external library in the code
    * required to output formatted strings
  - [nanoflann](https://github.com/jlblancoc/nanoflann) (>= 1.3.2, version included - v1.5.5)
    * included as external library in the code
    * required for neighbor search
  - [taskflow](https://github.com/taskflow/taskflow) (>= 3.7.0)
    * included as external library in the code
    * required for asynchronous/parallel for loop
  - [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css) (>= v2.3.3)
    * included as external library in the code
    * useful in building better doxygen documentation (see [docs/input-conf.doxy.in](./docs/input-conf.doxy.in) file) 

Additional dependencies for running the examples:
  - [gmsh](https://gmsh.info/) (>= 3.0.6)
    * recommend to install using `apt-get` or `brew`
    * required to build the mesh of various objects in the test
  - [python3](https://www.python.org/)
    * required to run the test python scripts
  - [numpy](https://numpy.org/)
    * required to run the test python scripts

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

ctest --verbose
```

## Installing dependencies
### Mac
Brew can be used to install all dependencies in mac as follows
```shell
brew install cmake libomp open-mpi tbb \
  yaml-cpp flann gmsh metis vtk 
```
### Ubuntu
1. Essential libraries can be installed using `apt-get` as follows
    ```shell
    sudo apt-get update 
    sudo apt-get install -y \
      libopenmpi-dev openmpi-bin \
      libblas-dev liblapack-dev libmpfr-dev libgmp-dev \
      libtbb-dev libasio-dev libglvnd-dev \
      libgmsh-dev gmsh \
      libflann-dev \
      libmetis-dev \
      libyaml-cpp-dev \
      python3-pip
      
    # for metis, create symlink
    sudo ln -sf /usr/lib/x86_64-linux-gnu/libmetis.so /usr/lib/libmetis.so 
    
    # python libs
    # NOTE: add '--break-system-packages' at the end if installing in Ubuntu 24.04 (noble)
    pip3 install numpy pyvista pandas
    ```
2. For `cmake`, if you are in ubuntu >= 20.04, install using `apt-get`
    ```shell
    sudo apt-get update \
    sudo apt-get install -y cmake 
    ```
   If you are in ubuntu < 20.04 (e.g., 18.04 bionic), install by adding new ppa to apt-get as follows
    ```shell
    # get codename of ubuntu
    UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"
    # add repo
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${UBUNTU_CODENAME} main" 
    echo "fixing public key issue with kitware repo"
    if [ "${UBUNTU_CODENAME}" = "bionic" ]; then
      gpg --keyserver keyserver.ubuntu.com --recv-keys 6AF7F09730B3F0A4 
      gpg --export --armor 6AF7F09730B3F0A4 | sudo apt-key add - 
    elif [ "${UBUNTU_CODENAME}" = "focal" ]; then
      gpg --keyserver keyserver.ubuntu.com --recv-keys 1A127079A92F09ED 
      gpg --export --armor 1A127079A92F09ED | sudo apt-key add - 
    else 
      echo "no fix for UBUNTU_CODENAME = ${UBUNTU_CODENAME} if there is a key error. try building cmake from repository"
    fi
    
    # now you can install recent cmake from apt-get
    sudo apt-get update 
    sudo apt-get install -y cmake
    ```
3. For vtk, if you are in ubuntu >= 22.04, install using `apt-get` as follows:
    ```shell
    sudo apt-get install -y libvtk9-dev 
    ```
   If you are in ubuntu < 22.04 (e.g., 18.04 bionic, 20.04 focal), build vtk and install as follows
```shell
# get codename of ubuntu
UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"

# instal dependency first
if [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "bionic" ]; then
  sudo apt-get install -y libgl1-mesa-dev
fi

# set some paths where we will download vtk and build
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
BUILDTHREADS="$(cat /proc/cpuinfo | grep processor | wc -l)" # or 12

install_at_global="0" # 0 - install in script path, 1 in /usr/local
VTK_INSTALL_PATH="/usr/local"
if [ $install_at_global = "0" ]; then
    VTK_INSTALL_PATH=$SCRIPTPATH/install/vtk/$VTK_VERSION/Release
fi
echo "VTK_INSTALL_PATH = ${VTK_INSTALL_PATH}"

# clone vtk
VTK_MAJOR_VERSION=9
VTK_MINOR_VERSION=3
VTK_PATCH_VERSION=0
VTK_VERSION=$VTK_MAJOR_VERSION.$VTK_MINOR_VERSION.$VTK_PATCH_VERSION
echo "VTK_VERSION = ${VTK_VERSION}"

git clone --recursive https://gitlab.kitware.com/vtk/vtk.git vtk-source-${VTK_VERSION}
cd vtk-${VTK_VERSION}
git checkout v${VTK_VERSION}

cd .. && mkdir -p vtk-build-${VTK_VERSION} && cd vtk-build-${VTK_VERSION}
cmake -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:STRING=${VTK_INSTALL_PATH} \
  -D BUILD_SHARED_LIBS=ON \
  -D BUILD_TESTING=OFF \
  -D VTK_REQUIRED_OBJCXX_FLAGS='' \
  -D HDF5_BUILD_FRAMEWORKS=OFF \
  -D VTK_BUILD_DOCUMENTATION=OFF \
  -D VTK_BUILD_EXAMPLES=OFF \
  -D VTK_BUILD_SCALED_SOA_ARRAYS=OFF \
  -D VTK_BUILD_SPHINX_DOCUMENTATION=OFF \
  -D VTK_GROUP_ENABLE_MPI=NO \
  -D VTK_GROUP_ENABLE_Qt=DONT_WANT \
  -D VTK_GROUP_ENABLE_Rendering=NO \
  -D VTK_GROUP_ENABLE_Views=NO \
  -D VTK_GROUP_ENABLE_Web=NO \
  -D VTK_Group_MPI=ON \
  -D VTK_USE_MPI=OFF \
  -D VTK_WRAP_PYTHON=OFF \
  ../vtk-source-${VTK_VERSION}
make -j -l$BUILDTHREADS
# installation
if [[ "$install_at_global" = "1" ]]; then
  sudo make install
else 
  make install
fi
echo VTK_LIB_CMAKE_DIR="${VTK_INSTALL_PATH}/lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
echo VTK_LIB_DIR="${VTK_INSTALL_PATH}/lib"
echo VTK_INCLUDE_DIR="${VTK_INSTALL_PATH}/include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
```

### Compile scripts
In directory [PeriDEM/tools/compile_scripts](./compile_scripts) various scripts are 
included to help install dependencies and compile PeriDEM code. 
If you follow this documentation, you probably would not need to look at 
those scripts as the dependencies in this library is kept quite small and can 
be easily installed using either `apt-get` or `brew`.

### Docker
For `circle-ci` testing, we use docker images `prashjha/peridem-base-jammy:latest` 
(`ubuntu 22.04`) and `prashjha/peridem-base-noble:latest` (`ubuntu 24.04`). 
The associated dockerfiles and scripts to use pre-built docker images in compiling code and using for clion remote development can be found in [PeriDEM/tools/docker/README.md](./docker/README.md).
