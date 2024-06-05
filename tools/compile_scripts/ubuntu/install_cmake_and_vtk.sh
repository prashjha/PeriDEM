#!/bin/bash

# get ubuntu codename
UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"

# get name of this script
this_script=$(basename "$0")

(

# -----------------------
# basic variables
# -----------------------
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12" # "$(cat /proc/cpuinfo | grep processor | wc -l)"

install_at_global="0" # 0 - create install path in SOURCEDIR, 1 install in /usr/local

cmake_build="0" # 0 - apt-get, 1 - add cmake repo and install, 2 - apt-get if possible else add cmake repo and install
cmake_add_repo_and_install="0" # 0 - yes, 1 - no; flag used only if cmake_build = 1 or 2
vtk_build="2" # 0 - apt-get, 1 - build and install vtk, 2 - apt-get if possible else build and install
doxygen_build="0" # 0 - yes, 1 - no

# some preliminary setup
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

## open a file to write various key paths for subsequent use
path_file="$SCRIPTPATH/lib_vars_paths_cmake_vtk.txt"

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
      CMAKE_VERSION="3.19.4"
      CMAKE_TAR_FILE="cmake-$CMAKE_VERSION.tar.gz"
      CMAKE_INSTALL_PATH=$SCRIPTPATH/install/cmake/$CMAKE_VERSION/Release
      if [[ "$install_at_global" = "1" ]]; then
        echo "cmake to be installed in /usr/local. This will require 'sudo'"
        CMAKE_INSTALL_PATH="/usr/local"
      fi
      CMAKE_SOURCE_DIR=$SOURCEDIR/cmake/$CMAKE_VER
      CMAKE_EXE_DIR="$CMAKE_INSTALL_PATH/bin"
      cd $SOURCEDIR
      if [ ! -f "$CMAKE_TAR_FILE" ]; then
        wget "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/$CMAKE_TAR_FILE"
      fi

      if [ ! -d $CMAKE_SOURCE_DIR ]; then
        mkdir -p $CMAKE_SOURCE_DIR
        tar -zxf $CMAKE_TAR_FILE -C $CMAKE_SOURCE_DIR --strip-components=1

        cd $CMAKE_SOURCE_DIR
        ./bootstrap --prefix=${CMAKE_INSTALL_PATH} 
        make -j -l$BUILDTHREADS
        if [[ "$install_at_global" = "1" ]]; then
          sudo make install
        else 
          make install
        fi
      fi
      echo "cleaning"
      cd $SCRIPTPATH
      if [ ! -d $CMAKE_SOURCE_DIR ]; then
        rm -rf $CMAKE_SOURCE_DIR
      fi
      echo "installed cmake at path = ${CMAKE_INSTALL_PATH}" 
      echo "adding cmake executible paths to file"
      echo CMAKE_EXE_DIR="${CMAKE_EXE_DIR}" >> ${path_file}
      echo "building cmake finished"
    fi
fi

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
    echo "building VTK"
    if [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "bionic" ]; then
      echo "installing dependency first"
      sudo apt-get install -y libgl1-mesa-dev
    fi
    echo "clone vtk"
    VTK_MAJOR_VERSION=9
    VTK_MINOR_VERSION=3
    VTK_PATCH_VERSION=0
    VTK_VERSION=$VTK_MAJOR_VERSION.$VTK_MINOR_VERSION.$VTK_PATCH_VERSION
    echo "VTK_VERSION = ${VTK_VERSION}"
    VTK_INSTALL_PATH="/usr/local"
    if [ $install_at_global = "0" ]; then
      VTK_INSTALL_PATH=$SCRIPTPATH/install/vtk/$VTK_VERSION/Release
    fi
    VTK_BUILD_PATH=$BUILDDIR/vtk/$VTK_VERSION/Release/
    VTK_SOURCE_RELATIVE_DIR=vtk/$VTK_VERSION
    VTK_SOURCE_DIR=$SOURCEDIR/$VTK_SOURCE_RELATIVE_DIR
    cd $SOURCEDIR
    git clone --recursive https://gitlab.kitware.com/vtk/vtk.git ${VTK_SOURCE_RELATIVE_DIR}
    cd ${VTK_SOURCE_RELATIVE_DIR}
    git checkout v${VTK_VERSION} 
    cd $BUILDDIR
    if [ ! -d "$VTK_BUILD_PATH" ]; then
      mkdir -p "$VTK_BUILD_PATH"
    else 
      rm -rf $VTK_BUILD_PATH
      mkdir -p $VTK_BUILD_PATH
    fi
    cd "$VTK_BUILD_PATH"
    "$CMAKE_EXE_DIR/cmake" -D CMAKE_BUILD_TYPE:STRING=Release \
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
          ${VTK_SOURCE_DIR}
    make -j -l$BUILDTHREADS
    sudo make install
    echo "cleaning"
    cd $SCRIPTPATH
    if [ ! -d $VTK_SOURCE_DIR ]; then
        rm -rf $VTK_SOURCE_DIR
    fi
    if [ ! -d $VTK_BUILD_PATH ]; then
        rm -rf $VTK_BUILD_PATH
    fi
    echo "adding vtk paths to file"
    echo VTK_LIB_CMAKE_DIR="${VTK_INSTALL_PATH}/lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}" >> ${path_file}
    echo VTK_LIB_DIR="${VTK_INSTALL_PATH}/lib" >> ${path_file}
    echo VTK_INCLUDE_DIR="${VTK_INSTALL_PATH}/include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}" >> ${path_file}
    echo "building vtk finished"
fi

) 2>&1 | tee "${this_script}.log"
