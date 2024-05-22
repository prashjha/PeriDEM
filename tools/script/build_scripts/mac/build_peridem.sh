#!/bin/bash

## dependencies

# vtk
# brew install vtk

# yaml-cpp
# brew install yaml-cpp

# target directory where code will be built
target_build=$pwd

# source
source="../../."

# run cmake with flags
cmake 	-DCMAKE_INSTALL_PREFIX="$target_build" \
	-DEnable_Documentation=OFF \
	-DEnable_Tests=ON \
	-DCMAKE_BUILD_TYPE=Release \
	"$source" 
	#--trace-expand

# make
make -j 10

# test
ctest --verbose
