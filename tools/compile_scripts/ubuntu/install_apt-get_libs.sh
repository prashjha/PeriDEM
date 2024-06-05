#!/bin/bash

# get ubuntu codename
UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"

# get name of this script
this_script=$(basename "$0")

(

# -----------------------
# basic variables
# -----------------------
cmake_build="0" # 0 - apt-get, 1 - add cmake repo and install, 2 - apt-get if possible else add cmake repo and install
cmake_add_repo_and_install="0" # 0 - yes, 1 - no; flag used only if cmake_build = 1 or 2
vtk_build="2" # 0 - apt-get, 1 - build and install vtk, 2 - apt-get if possible else build and install
doxygen_build="0" # 0 - yes, 1 - no

## open a file to write various key paths for subsequent use
path_file="$SCRIPTPATH/lib_vars_paths.txt"

# if path file exists, delete it so that it is created a fresh
if [[ -f  $path_file ]]; then
  echo "Path file exists, deleting it"
  rm $path_file
fi

# -----------------------
# init path file
# -----------------------
echo UBUNTU_CODENAME=${UBUNTU_CODENAME} >> ${path_file}

# -----------------------
# essentials
# -----------------------
# disable prompts during apt-get install
DEBIAN_FRONTEND=noninteractive

echo "installing essential libraries" && \
sudo apt-get update --fix-missing && \
sudo apt-get upgrade -y && \
sudo apt-get install -y \
  less ca-certificates gpg wget curl \
  lzip bzip2 unzip \
  software-properties-common ubuntu-dev-tools build-essential \
  openssh-server rsync 

# -----------------------
# make/configure related libraries
# -----------------------
echo "installing make/configure/cmake related libraries" 
sudo apt-get install -y \
  m4 autoconf libtool pkg-config make gfortran \
  lldb valgrind \
  clang-format clang-tidy 

# -----------------------
# cmake (manually install in older ubuntu, e.g., bionic)
# -----------------------
# decide if we use apt-get or build cmake
if [ ${cmake_build} == "2" ]; then
  if [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "jammy" ]  || [ "${UBUNTU_CODENAME}" = "noble" ]; then
    cmake_build="0"
  elif [ "${UBUNTU_CODENAME}" = "bionic" ]; then
    cmake_build="1"
  else 
    cmake_build="1"
  fi
fi

if [ ${cmake_build} = "0" ]; then 
    echo "installing cmake via apt-get"
    sudo apt-get install -y cmake
    echo "adding cmake executible paths to file" 
    CMAKE_EXE_DIR="/usr/bin"
    echo CMAKE_EXE_DIR="${CMAKE_EXE_DIR}" >> ${path_file}
else
    if [ $cmake_add_repo_and_install = "0" ]; then
      echo "adding ppa repo for cmake" 
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
        echo "no fix for UBUNTU_CODENAME = ${UBUNTU_CODENAME} if there is key error. try building cmake from repository"
      fi
      sudo apt-get update 
      echo "finally installing cmake using apt-get" 
      sudo apt-get install -y cmake
      echo "adding cmake executible paths to file" 
      CMAKE_EXE_DIR="/usr/bin"
      echo CMAKE_EXE_DIR="${CMAKE_EXE_DIR}" >> ${path_file}
    else
      echo "building of cmake is done in other script as this script is run using 'sudo' to install apt-get libraries"
    fi
fi

# -----------------------
# mpi
# -----------------------
echo "install openmpi libraries" 
sudo apt-get install -y libopenmpi-dev openmpi-bin 

# -----------------------
# git
# -----------------------
echo "install and configure git" 
sudo apt-get install -y git 

# -----------------------
# doxygen/code coverage
# -----------------------
if [ $doxygen_build = "0" ]; then
  echo "doxygen and code coverage related libraries" 
  sudo apt-get install -y \
    doxygen doxygen-latex graphviz ghostscript \
    rpm gcovr ruby-coveralls libproj-dev
fi

# -----------------------
# python
# -----------------------
# first install dependencies for pillow library
echo "installing dependency for python libraries" 
sudo apt-get install -y libjpeg-dev zlib1g-dev 
echo "installing python pip3" 
sudo apt-get install -y python3-pip 
echo "install python libraries using pip" 
if [ "${UBUNTU_CODENAME}" = "noble" ]; then 
    pip3 install numpy pyvista pandas --break-system-packages
else 
    pip3 install numpy pyvista pandas
fi

# -----------------------
# essential computational/hpc libraries
# -----------------------
echo "installing essential computational/hpc libraries"
sudo apt-get install -y \
  libblas-dev liblapack-dev libmpfr-dev libgmp-dev \
  libtbb-dev libasio-dev libglvnd-dev 

# -----------------------
# gmsh
# -----------------------
echo "installing gmsh libraries" 
sudo apt-get install -y libgmsh-dev gmsh

# -----------------------
# flann
# -----------------------
echo "installing flann library" 
sudo apt-get install -y libflann-dev

# -----------------------
# metis
# -----------------------
echo "installing metis" 
sudo apt-get install -y libmetis-dev 
echo "creating symlink of metis library" 
sudo ln -sf /usr/lib/x86_64-linux-gnu/libmetis.so /usr/lib/libmetis.so 
echo "Display metis header and lib files" 
ls /usr/include/metis* 
ls /usr/lib/libmetis* 
echo "adding metis paths to file" 
echo METIS_LIB_DIR="/usr/lib" >> ${path_file}
echo METIS_INCLUDE_DIR="/usr/include" >> ${path_file}

# -----------------------
# yaml-cpp
# -----------------------
echo "installing yaml-cpp" 
sudo apt-get install -y libyaml-cpp-dev

# -----------------------
# vtk (build in focal/bionic and apt-get in noble/jammy)
# -----------------------
# decide if we use apt-get or build vtk
if [ ${vtk_build} == "2" ]; then
  if [ "${UBUNTU_CODENAME}" = "jammy" ]  || [ "${UBUNTU_CODENAME}" = "noble" ]; then
    vtk_build="0"
  elif [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "bionic" ]; then
    vtk_build="1"
  else 
    vtk_build="1"
  fi
fi

echo "installing vtk" 
if [ ${vtk_build} == "0" ]; then 
    echo "apt-get VTK"
    sudo apt-get install -y libvtk9-dev 
    echo "adding vtk paths to file"
    echo VTK_LIB_CMAKE_DIR="/usr/lib/x86_64-linux-gnu/cmake/vtk-9.1" >> ${path_file}
    echo VTK_LIB_DIR="/usr/lib/x86_64-linux-gnu" >> ${path_file}
    echo VTK_INCLUDE_DIR="/usr/include/vtk-9.1" >> ${path_file}
    echo "installing vtk finished"
else
    if [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "bionic" ]; then
      echo "installing dependency first"
      sudo apt-get install -y libgl1-mesa-dev
    fi
    echo "building of vrk is done in other script as this script is run using 'sudo' to install apt-get libraries"
fi

) 2>&1 | tee "${this_script}.log"
