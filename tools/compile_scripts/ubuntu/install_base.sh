#!/bin/bash

# get ubuntu codename
UBUNTU_CODENAME="$(cat /etc/os-release | grep UBUNTU_CODENAME | cut -d = -f 2)"

# Install essentials
sudo apt-get update 

sudo apt-get install -y \
    git less wget curl lzip bzip2 unzip autoconf libtool pkg-config \
    rpm gcovr ruby-coveralls libproj-dev m4 \
    software-properties-common ubuntu-dev-tools \
    gfortran make build-essential libssl-dev clang-format-10 clang-tidy \
    openssh-server rsync \
    doxygen doxygen-latex graphviz ghostscript \
    lldb valgrind \
    python3-pip \
    ibopenmpi-dev openmpi-bin \
    libblas-dev liblapack-dev libmpfr-dev libgmp-dev \
    libhwloc-dev libjemalloc-dev libboost-all-dev libyaml-cpp-dev \
    libgmsh-dev gmsh libflann-dev \
    libmetis-dev \
    libtbb-dev libasio-dev libglvnd-dev \
    libjpeg-dev zlib1g-dev # for pillow python library \
    libgl1-mesa-dev # for VTK

if [ "${UBUNTU_CODENAME}" = "focal" ]; then
  sudo apt-get install -y gcc-10 g++-10 
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100 
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100
  echo "Display gcc version"  && gcc -v
fi

# clean
sudo apt-get autoremove -y && sudo apt-get autoclean -y

# python packages
pip3 install numpy pyvista pandas

# create symlink for metis
sudo ln -sf /usr/lib/x86_64-linux-gnu/libmetis.so /usr/lib/libmetis.so 
echo "Display metis header and lib files" 
ls /usr/include/metis* 
ls /usr/lib/libmetis*
