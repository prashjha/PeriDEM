#!/bin/bash

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12"
INSTALLATGLOBAL="0" # if 1, then libraries will be installed in /usr/local <-- for containers

cmake_build="0" # set to 1 to install
hpx_build="0"

# some preliminary setup
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

## open a file to write various key paths for subsequent use
path_file="$SCRIPTPATH/paths.txt"

# if path file exists, delete it so that it is created a fresh
if [[ -f  $path_file ]]; then
  echo "Path file exists, deleting it"
  rm $path_file
fi

echolog()
(
  echo "$@"
  echo "$@" >> $path_file
)

## process input
BUILD_TYPE="$1"
if [ "$1" = "" ]; then
    echo "Build Type Not Specified. Setting build type to Release."
    BUILD_TYPE="Release"
fi

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ] && [ "$BUILD_TYPE" != "RelWithDebInfo" ]; then
    echo "Build Type Is Not Correct"
    exit -1
fi

echo " "
echolog "SCRIPTPATH=\"$SCRIPTPATH\"
SOURCEDIR=\"$SOURCEDIR\"
BUILDDIR=\"$BUILDDIR\""

echo "Shell: $(which sh)
BUILD_TYPE=$BUILD_TYPE
"

echo "<<<<<<<<<<< >>>>>>>>>>>
CMAKE
<<<<<<<<<<< >>>>>>>>>>>"
CMAKE_VER="3.19.4"
CMAKE_TAR_FILE="cmake-$CMAKE_VER.tar.gz"
CMAKE_INSTALL_PATH=$SCRIPTPATH/local/cmake/$CMAKE_VER/$BUILD_TYPE
if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
  CMAKE_INSTALL_PATH="/usr/local"
fi
CMAKE_BUILD_PATH=$BUILDDIR/cmake/$BUILD_TYPE/
CMAKE_SOURCE_DIR=$SOURCEDIR/cmake/$CMAKE_VER
CMAKE_EXE="$CMAKE_INSTALL_PATH/bin/cmake"

# if we are not building cmake, then we most probably be using cmake
# installed either at /usr/local/bin or /usr/bin (path of cmake is not important as long as it can be used simply by typing cmake in shell)
if [[ $cmake_build -eq "0" ]]; then
  CMAKE_EXE="cmake"
  CMAKE_INSTALL_PATH="/usr/bin"
fi


if [[ $cmake_build -eq "1" ]]; then
  # download library
  cd $SOURCEDIR
  if [ ! -f "$CMAKE_TAR_FILE" ]; then
    wget "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VER/$CMAKE_TAR_FILE"
  fi

  if [ ! -d $CMAKE_SOURCE_DIR ]; then
    mkdir -p $CMAKE_SOURCE_DIR
    tar -zxf $CMAKE_TAR_FILE -C $CMAKE_SOURCE_DIR --strip-components=1

    cd $CMAKE_SOURCE_DIR
    ./bootstrap --prefix=${CMAKE_INSTALL_PATH} 
    make -j -l$BUILDTHREADS
    make install
  fi
fi

echo " 
CMAKE_INSTALL_PATH=$CMAKE_INSTALL_PATH 
CMAKE_EXE=$CMAKE_EXE
$($CMAKE_EXE --version)
"
echolog "
## cmake
CMAKE_INSTALL_PATH=\"$CMAKE_INSTALL_PATH\""

echo "<<<<<<<<<<< >>>>>>>>>>>
GMSH
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install gmsh
mac: brew install gmsh
"

echo "<<<<<<<<<<< >>>>>>>>>>>
BOOST
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install libboost-all-dev
mac: brew install boost
"

echo "<<<<<<<<<<< >>>>>>>>>>>
YAML
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install libyaml-cpp-dev
mac: brew install yaml-cpp
"

echo "<<<<<<<<<<< >>>>>>>>>>>
VTK
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install libvtk7-dev
mac: brew install vtk
"

echo "<<<<<<<<<<< >>>>>>>>>>>
HWLOC
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install libhwloc-dev
mac: brew install hwloc
"

echo "<<<<<<<<<<< >>>>>>>>>>>
JEMALLOC
<<<<<<<<<<< >>>>>>>>>>>
ubuntu: sudo apt install libjemalloc-dev
mac: brew install jemalloc
"

echo "<<<<<<<<<<< >>>>>>>>>>>
HPX
<<<<<<<<<<< >>>>>>>>>>>
"
HPX_VER="1.3.0"
HPX_TAR_FILE="${HPX_VER}.tar.gz"
HPX_INSTALL_PATH=$SCRIPTPATH/local/hpx/$HPX_VER/$BUILD_TYPE
if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
  HPX_INSTALL_PATH="/usr/local"
fi
HPX_BUILD_PATH=$BUILDDIR/hpx/$HPX_VER/$BUILD_TYPE/
HPX_SOURCE_DIR=$SOURCEDIR/hpx/$HPX_VER

if [[ $hpx_build -eq "1" ]]; then
  # download library
  cd $SOURCEDIR
  if [ ! -f "$HPX_TAR_FILE" ]; then
    wget "https://github.com/STEllAR-GROUP/hpx/archive/${HPX_VER}.tar.gz"
  fi

  if [ ! -d "$HPX_TAR_FILE" ]; then
    mkdir -p $HPX_SOURCE_DIR
    tar -zxf $HPX_TAR_FILE -C $HPX_SOURCE_DIR --strip-components=1
  fi

  # build library
  cd $BUILDDIR

  if [ ! -d "$HPX_BUILD_PATH" ]; then
    mkdir -p "$HPX_BUILD_PATH"
  else 
    rm -rf $HPX_BUILD_PATH
    mkdir -p $HPX_BUILD_PATH
  fi

  cd "$HPX_BUILD_PATH"

  $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE   \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DHPX_WITH_THREAD_IDLE_RATES=ON \
        -DHPX_WITH_DISABLED_SIGNAL_EXCEPTION_HANDLERS=ON \
        -DHPX_WITH_MALLOC=jemalloc \
        -DHPX_WITH_EXAMPLES=OFF \
        -DHPX_WITH_NETWORKING=OFF \
        -DCMAKE_INSTALL_PREFIX=$HPX_INSTALL_PATH \
        $HPX_SOURCE_DIR
        
        # -DBOOST_ROOT=$BOOST_INSTALL_PATH \
        # -DHWLOC_ROOT=$HWLOC_INSTALL_PATH \
        # -DJEMALLOC_ROOT=$JEMALLOC_INSTALL_PATH \


  cd "$HPX_BUILD_PATH"
  make -j -l$BUILDTHREADS
  make install
fi

echolog "
##
HPX_INSTALL_PATH=\"$HPX_INSTALL_PATH\""

## provide summary
echo " 

>> Dependencies are installed!!

>> Here is summary of various paths (particularly note the HPX path that is needed to install PeriDEM)

"

echo "$(<$path_file)"

echo " 

>> Next, install PeriDEM using install_peridem.sh script

"

) |& tee "build.log"
