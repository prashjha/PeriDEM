#!/bin/bash

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="1"
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

echo "SCRIPTPATH = $SCRIPTPATH"
echo "SOURCEDIR = $SOURCEDIR"
echo "BUILDDIR = $BUILDDIR"

## process input
BUILD_TYPE="$1"
if [ "$1" = "" ]; then
    echo "Build Type Not Specified"
    exit -1
fi

if [ "$1" != "Debug" ] && [ "$1" != "Release" ] && [ "$1" != "RelWithDebInfo" ]; then
    echo "Build Type Is Not Correct"
    exit -1
fi

echo "Shell: $(which sh)"

CMAKE_EXE="cmake"

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "HPX"
echo "<<<<<<<<<<< >>>>>>>>>>>"
HPX_VER="1.3.0"
HPX_TAR_FILE="${HPX_VER}.tar.gz"
HPX_INSTALL_PATH="/usr/local/"
HPX_BUILD_PATH=$BUILDDIR/hpx/$HPX_VER/$BUILD_TYPE/
HPX_SOURCE_DIR=$SOURCEDIR/hpx/$HPX_VER

hpx_build="1"
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
  fi

  cd "$HPX_BUILD_PATH"
  make -j -l$BUILDTHREADS
  make install
fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "PCL"
echo "<<<<<<<<<<< >>>>>>>>>>>"
PCL_VER="1.11.1"
PCL_TAR_FILE="pcl-$PCL_VER.tar.gz"
PCL_INSTALL_PATH="/usr/local/"
PCL_BUILD_PATH=$BUILDDIR/pcl/$PCL_VER/$BUILD_TYPE/
PCL_SOURCE_DIR=$SOURCEDIR/pcl/$PCL_VER

pcl_build="1"
if [[ $pcl_build -eq "1" ]]; then
  # download library
  cd $SOURCEDIR
  if [ ! -f "$PCL_TAR_FILE" ]; then
    wget "https://github.com/PointCloudLibrary/pcl/archive/$PCL_TAR_FILE"
  fi

  if [ ! -d "$PCL_SOURCE_DIR" ]; then
    mkdir -p $PCL_SOURCE_DIR
    tar -zxf $PCL_TAR_FILE -C $PCL_SOURCE_DIR --strip-components=1
  fi

  # build library
  cd $BUILDDIR

  if [ ! -d "$PCL_BUILD_PATH" ]; then
    mkdir -p "$PCL_BUILD_PATH"

    cd "$PCL_BUILD_PATH"

    $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE  \
               -DCMAKE_INSTALL_PREFIX=$PCL_INSTALL_PATH \
               -DBUILD_2d=OFF \
               -DBUILD_apps=OFF \
               -DBUILD_common=On \
               -DBUILD_examples=OFF \
               -DBUILD_features=OFF \
               -DBUILD_filters=OFF \
               -DBUILD_geometry=OFF \
               -DBUILD_global_tests=OFF \
               -DBUILD_io=OFF \
               -DBUILD_keypoints=OFF \
               -DBUILD_ml=OFF \
               -DBUILD_outofcore=OFF \
               -DBUILD_people=OFF \
               -DBUILD_recognition=OFF \
               -DBUILD_registration=OFF \
               -DBUILD_sample_consensus=OFF \
               -DBUILD_segmentation=OFF \
               -DBUILD_simulation=OFF \
               -DBUILD_stereo=OFF \
               -DBUILD_surface=OFF \
               -DBUILD_surface_on_nurbs=OFF \
               -DBUILD_tools=OFF \
               -DBUILD_tracking=OFF \
               -DBUILD_visualization=OFF \
               $PCL_SOURCE_DIR

               #-DFLANN_ROOT="$FLANN_INSTALL_PATH" \
               #-DFLANN_LIBRARY="$FLANN_INSTALL_PATH/lib" \
               #-DFLANN_INCLUDE_DIR="$FLANN_INSTALL_PATH/include" \
               # -DBOOST_ROOT=$BOOST_INSTALL_PATH \
               # -DBoost_INCLUDE_DIR=$BOOST_INSTALL_PATH/include \
               # -DVTK_ROOT=$VTK_INSTALL_PATH \
               
  fi
  cd "$PCL_BUILD_PATH"
  make -j -l$BUILDTHREADS
  make install
fi

) |& tee "build.log"
