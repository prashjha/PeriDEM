# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#
# Build minimal ubuntu-18.04 image with few essential libraries 
#

FROM ubuntu:bionic

# Set metadata
LABEL maintainer="Prashant K. Jha <pjha.sci@gmail.com>"
LABEL version="0.1.0"
LABEL description="Ubuntu 18.04 with essential libraries for building HPX, PCL, PeriDEM"

# Set noninteractive mode (otherwise the build stucks at step where ubuntu asks for geographical location)
ENV DEBIAN_FRONTEND=nonintercative

# Install essentials
RUN apt-get update && \
  apt-get install -y build-essential ubuntu-dev-tools rpm gcovr \
  git wget lzip \
  cmake autoconf libtool pkg-config \
  liblapack-dev libblas-dev libopenmpi-dev \
  doxygen doxygen-latex graphviz ghostscript \
  gfortran libmpfr-dev libgmp-dev \
  libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
  libvtk7-dev gmsh libflann-dev python3-pip && \
  apt-get autoremove -y && \
  apt-get autoclean -y

RUN pip3 install numpy pyvista pandas


