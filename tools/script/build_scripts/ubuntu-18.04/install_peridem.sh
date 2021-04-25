#!/bin/bash

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12"
INSTALLATGLOBAL="0" # if 1, then libraries will be installed in /usr/local <-- for containers

# some preliminary setup
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

echo "SCRIPTPATH = $SCRIPTPATH"
echo "SOURCEDIR = $SOURCEDIR"
echo "BUILDDIR = $BUILDDIR"
echo "Shell: $(which sh)"

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

# echo "<<<<<<<<<<< >>>>>>>>>>>"
# echo "CMAKE"
# echo "<<<<<<<<<<< >>>>>>>>>>>"
CMAKE_EXE="cmake"

# echo "<<<<<<<<<<< >>>>>>>>>>>"
# echo "HPX"
# echo "<<<<<<<<<<< >>>>>>>>>>>"
HPX_INSTALL_PATH="0"
if [[ $HPX_INSTALL_PATH -eq "0" ]]; then
  echo "Please provide path where HPX is installed in HPX_INSTALL_PATH variable and then rerun the script again."
fi


echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "PERIDEM"
echo "<<<<<<<<<<< >>>>>>>>>>>"
PERIDEM_VER="0.1.0"
PERIDEM_PATH=$SCRIPTPATH/peridem/$1/
PERIDEM_SOURCE_DIR=$SOURCEDIR/peridem/$PERIDEM_VER
PERIDEM_INSTALL_PATH=$SCRIPTPATH/local/peridem/$PERIDEM_VER/$BUILD_TYPE

peridem_build="1"
if [[ $peridem_build -eq "1" ]]; then
  # download library
  cd $SOURCEDIR
  
  if [ ! -d $PERIDEM_SOURCE_DIR ]; then
    mkdir -p $PERIDEM_SOURCE_DIR
    git clone git@github.com:prashjha/PeriDEM.git $PERIDEM_SOURCE_DIR
  fi

  # build library
  cd $PERIDEM_PATH

  if [ ! -d "$PERIDEM_PATH" ]; then
    mkdir -p "$PERIDEM_PATH"
  else 
    rm -rf $PERIDEM_PATH
    mkdir -p $PERIDEM_PATH
  fi

  cd "$PERIDEM_PATH"

  # add when building in mac
  # -DYAML_CPP_DIR="/usr/local" \
  $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE   \
        -DCMAKE_INSTALL_PREFIX=$PERIDEM_INSTALL_PATH \
        -DCMAKE_INSTALL_RPATH=$PERIDEM_INSTALL_PATH \
        -DHPX_DIR="$HPX_INSTALL_PATH/lib/cmake/HPX" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        $PERIDEM_SOURCE_DIR

        #-DCMAKE_INSTALL_RPATH=$PERIDEM_INSTALL_PATH \
        #-DYAML_CPP_DIR="$YAML_INSTALL_PATH" \
        #-DVTK_DIR="$VTK_INSTALL_PATH/lib/cmake/vtk-${VTK_VER_MAJOR}" \

  cd "$PERIDEM_PATH"
  make -j -l$BUILDTHREADS
  ctest --verbose
fi 

) |& tee "build_peridem.log"
