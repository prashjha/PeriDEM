#!/bin/bash

#
# dependencies
#

# boost
# brew install boost

# vtk
# brew install vtk

# yaml-cpp
# brew install yaml-cpp

# hwloc
# brew install hwloc

# jemalloc
# brew install jemalloc

# hpx (Built with jemalloc 4.4.0 with cmake flag -DHPX_WITH_MALLOC=jemalloc)
hpx_dir="${HOME}/Softwares/local_peridem/local/hpx/1.3.0/Release/lib/cmake/HPX"

# target directory where code will be built
target_build=$pwd

# source
source="../../."

# run cmake with flags
cmake 	-DHPX_DIR="$hpx_dir" \
	-DCMAKE_INSTALL_PREFIX="$target_build" \
	-DEnable_Documentation=ON \
	-DEnable_Tests=ON \
	-DCMAKE_BUILD_TYPE=Release \
	"$source"

# make
make -j 10
