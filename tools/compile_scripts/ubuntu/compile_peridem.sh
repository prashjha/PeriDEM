#!/bin/bash

# get name of this script
this_script=$(basename "$0")

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
BUILDTHREADS="12" # "$(cat /proc/cpuinfo | grep processor | wc -l)"

CMAKE_EXE_DIR="/usr/bin" # modify
if [ ! -f "$CMAKE_EXE_DIR/cmake" ]; then
    echo "Could not find cmake executable in dir = CMAKE_EXE_DIR"
    exit
fi

## open a file to write various key paths for subsequent use
path_file="$SCRIPTPATH/lib_vars_paths.txt"

## get paths from file
. ${path_file}
echo "VTK_LIB_CMAKE_DIR = ${VTK_LIB_CMAKE_DIR}"
echo "METIS_LIB_DIR = ${METIS_LIB_DIR}"

## or set them manually below
## VTK_LIB_CMAKE_DIR="/usr/local/lib/cmake/vtk-9.3"
## METIS_LIB_DIR="/usr/lib"

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "PERIDEM"
echo "<<<<<<<<<<< >>>>>>>>>>>"
cd $SCRIPTPATH
git clone https://github.com/prashjha/PeriDEM.git 
cd PeriDEM 
cd .. 

build_types=(Debug Release)

for build_type in "${build_types[@]}"; do
  echo "Build type = ${build_type}"
  
  mkdir -p peridem_build/${build_type} && cd peridem_build/${build_type}

  cmake \
    -DCMAKE_BUILD_TYPE=${build_type} \
    -DEnable_Tests=ON \
    -DDisable_Docker_MPI_Tests=OFF \
    -DVTK_DIR="${VTK_LIB_CMAKE_DIR}" \
    -DMETIS_DIR="${METIS_LIB_DIR}" \
    ../../PeriDEM 

  make -j -l$BUILDTHREADS

  ctest --verbose

  # cd to base
  cd ../..
done
) 2>&1 | tee "$(basename "$0").log"
