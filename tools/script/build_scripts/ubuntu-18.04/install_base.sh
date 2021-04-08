#!/bin/bash

# Install essentials
sudo apt-get update && \
  sudo apt-get install -y build-essential ubuntu-dev-tools rpm gcovr \
  git wget lzip \
  cmake autoconf libtool pkg-config \
  liblapack-dev libblas-dev libopenmpi-dev \
  doxygen doxygen-latex graphviz ghostscript \
  gfortran libmpfr-dev libgmp-dev \
  libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
  libvtk7-dev gmsh libflann-dev python3-pip && \
  sudo apt-get autoremove -y && \
  sudo apt-get autoclean -y

pip3 install numpy pyvista pandas
