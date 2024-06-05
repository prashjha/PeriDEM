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

CMAKE_EXE_DIR="/usr/bin" # modify
if [ ! -f "$CMAKE_EXE_DIR/cmake" ]; then
    echo "Could not find cmake executable in dir = CMAKE_EXE_DIR"
    exit
fi

install_at_global="0" # 0 - create install path in SOURCEDIR, 1 install in /usr/local

vtk_build="2" # 0 - apt-get, 1 - build and install vtk, 2 - apt-get if possible else build and install

# some preliminary setup
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

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
# vtk
# -----------------------
echo "building VTK"
if [ "${UBUNTU_CODENAME}" = "focal" ] || [ "${UBUNTU_CODENAME}" = "bionic" ]; then
    echo "we expect this libgl1-mesa-dev library to be installed"
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
make install
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

) 2>&1 | tee "${this_script}.log"
