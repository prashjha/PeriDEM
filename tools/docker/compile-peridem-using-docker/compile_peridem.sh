#!/bin/bash

(

VTK_DIR="/usr/local/lib/cmake/vtk-9.3"
METIS_DIR="/usr/lib"

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "PERIDEM"
echo "<<<<<<<<<<< >>>>>>>>>>>"
git clone https://github.com/prashjha/PeriDEM.git 
cd PeriDEM 
git checkout remove_hpx 
git submodule update --init --recursive 
cd .. 

build_types=(Debug Release)

for build_type in "${build_types[@]}"; do
  echo "Build type = ${build_type}"
  
  mkdir -p peridem_build/${build_type} && cd peridem_build/${build_type}

  cmake \
    -DCMAKE_BUILD_TYPE=${build_type} \
    -DEnable_Tests=ON \
    -DDisable_Docker_MPI_Tests=OFF \
    -DVTK_DIR="${VTK_DIR}" \
    -DMETIS_DIR="${METIS_DIR}" \
    ../../PeriDEM 

  make -j $(cat /proc/cpuinfo | grep processor | wc -l) VERBOSE=1 

  ctest --extra-verbose

  # cd to base
  cd ../..
done
) |& tee "compile_peridem.log"
