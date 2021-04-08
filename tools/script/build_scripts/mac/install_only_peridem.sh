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

# flann
# brew install flann

# hpx (Built with jemalloc 4.4.0 with cmake flag -DHPX_WITH_MALLOC=jemalloc)
hpx_dir="${HOME}/work/peridem_works/build_all/hpx/Release/install/lib/cmake/HPX"

# pcl 
pcl_dir="$local/pcl/1.11.1/share/pcl-1.11"

# target directory where code will be built
target_build=$pwd

# source
source="../../."

# run cmake with flags
cmake 	-DHPX_DIR="$hpx_dir/lib/cmake/HPX" \
				-DPCL_DIR="$pcl_dir" \
				-DCMAKE_INSTALL_PREFIX="$target_build" \
				-DEnable_Documentation=ON \
				-DEnable_Tests=ON \
				-DCMAKE_BUILD_TYPE=Release \
				"$source"

# make
make -j 10
