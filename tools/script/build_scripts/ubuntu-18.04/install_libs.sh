#!/bin/bash

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12"
INSTALLATGLOBAL="0" # if 1, then libraries will be installed in /usr/local <-- for containers

cmake_build="1" # set to 1 to install
hpx_build="1"
pcl_build="1"

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

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "CMAKE"
echo "<<<<<<<<<<< >>>>>>>>>>>"
CMAKE_VER="3.19.4"
CMAKE_TAR_FILE="cmake-$CMAKE_VER.tar.gz"
CMAKE_INSTALL_PATH=$SCRIPTPATH/local/cmake/$CMAKE_VER/$BUILD_TYPE
if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
  CMAKE_INSTALL_PATH="/usr/local"
fi
CMAKE_BUILD_PATH=$BUILDDIR/cmake/$BUILD_TYPE/
CMAKE_SOURCE_DIR=$SOURCEDIR/cmake/$CMAKE_VER
CMAKE_EXE="$CMAKE_INSTALL_PATH/bin/cmake"


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

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "GMSH"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install gmsh
# brew install gmsh

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "BOOST"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libboost-all-dev
# brew install boost

# export BOOST_BUILD_TYPE=$(echo ${BUILD_TYPE/%WithDebInfo/ease} | tr '[:upper:]' '[:lower:]')

# BOOST_VER="1.68.0"
# BOOST_TAR_FILE="boost_1_68_0.tar.bz2"
# BOOST_INSTALL_PATH=$SCRIPTPATH/local/boost/$BOOST_VER/$BOOST_BUILD_TYPE
# if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
#   BOOST_INSTALL_PATH="/usr/local"
# fi
# BOOST_BUILD_PATH=$BUILDDIR/boost/$BUILD_TYPE/
# BOOST_SOURCE_DIR=$SOURCEDIR/boost/$BOOST_VER

# boost_build="0"
# if [[ $boost_build -eq "1" ]]; then
#   # download library
#   cd $SOURCEDIR
#   if [ ! -f "$BOOST_TAR_FILE" ]; then
#     wget "http://downloads.sourceforge.net/project/boost/boost/${BOOST_VER}/$BOOST_TAR_FILE"
#   fi

#   if [ ! -d $BOOST_SOURCE_DIR ]; then
#     mkdir -p $BOOST_SOURCE_DIR
#     tar xjf $BOOST_TAR_FILE -C $BOOST_SOURCE_DIR --strip-components=1
#   fi

#   cd $BOOST_SOURCE_DIR
#   ./bootstrap.sh --prefix=${BOOST_INSTALL_PATH} --with-toolset=gcc

#   ./b2 -j${BUILDTHREADS} --with-atomic --with-iostreams --with-filesystem --with-program_options --with-regex --with-system --with-chrono --with-date_time --with-thread ${BOOST_BUILD_TYPE} install
# fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "YAML"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libyaml-cpp-dev
# brew install yaml-cpp

# YAML_VER="0.6.3"
# YAML_TAR_FILE="yaml-cpp-${YAML_VER}.tar.gz"
# YAML_INSTALL_PATH=$SCRIPTPATH/local/yaml-cpp/$YAML_VER/$BUILD_TYPE
# if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
#   YAML_INSTALL_PATH="/usr/local"
# fi
# YAML_BUILD_PATH=$BUILDDIR/yaml-cpp/$BUILD_TYPE/
# YAML_SOURCE_DIR=$SOURCEDIR/yaml-cpp/$YAML_VER

# yaml_build="0"
# if [[ $yaml_build -eq "1" ]]; then
#   # download library
#   cd $SOURCEDIR
#   if [ ! -f "$YAML_TAR_FILE" ]; then
#     wget "https://github.com/jbeder/yaml-cpp/archive/$YAML_TAR_FILE"
#   fi

#   if [ ! -d "$YAML_SOURCE_DIR" ]; then
#     mkdir -p $YAML_SOURCE_DIR
#     tar -zxf $YAML_TAR_FILE -C $YAML_SOURCE_DIR --strip-components=1
#   fi

#   # build library
#   cd $BUILDDIR

#   if [ ! -d "$YAML_BUILD_PATH" ]; then
#     mkdir -p "$YAML_BUILD_PATH"
#   else 
#     rm -rf "$YAML_BUILD_PATH"
#     mkdir -p "$YAML_BUILD_PATH"
#   fi

#   cd "$YAML_BUILD_PATH"

#   $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE   \
#         -DCMAKE_INSTALL_PREFIX=$YAML_INSTALL_PATH \
#         -DYAML_BUILD_SHARED_LIBS=ON \
#         $YAML_SOURCE_DIR

#   cd "$YAML_BUILD_PATH"
#   make -j -l$BUILDTHREADS
#   make install
# fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "VTK"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libvtk7-dev
# brew install vtk

# VTK_VER_MAJOR="8.2"
# VTK_VER="${VTK_VER_MAJOR}.0"
# VTK_TAR_FILE="VTK-${VTK_VER}.tar.gz"
# VTK_INSTALL_PATH=$SCRIPTPATH/local/vtk/$VTK_VER/$BUILD_TYPE
# if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
#   VTK_INSTALL_PATH="/usr/local"
# fi
# VTK_BUILD_PATH=$BUILDDIR/vtk/$BUILD_TYPE/
# VTK_SOURCE_DIR=$SOURCEDIR/vtk/$VTK_VER

# vtk_build="0"
# if [[ $vtk_build -eq "1" ]]; then
#   # download library
#   cd $SOURCEDIR
#   if [ ! -f "$VTK_TAR_FILE" ]; then
#     wget "https://www.vtk.org/files/release/${VTK_VER_MAJOR}/${VTK_TAR_FILE}"
#   fi

#   if [ ! -d "$VTK_SOURCE_DIR" ]; then
#     mkdir -p $VTK_SOURCE_DIR
#     tar -zxf $VTK_TAR_FILE -C $VTK_SOURCE_DIR --strip-components=1
#   fi

#   # build library
#   cd $BUILDDIR

#   if [ ! -d "$VTK_BUILD_PATH" ]; then
#     mkdir -p "$VTK_BUILD_PATH"
#   else 
#     rm -rf "$VTK_BUILD_PATH"
#     mkdir -p "$VTK_BUILD_PATH"
#   fi

#   cd "$VTK_BUILD_PATH"

#   $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE  \
#         -DCMAKE_INSTALL_PREFIX=$VTK_INSTALL_PATH \
#         -DVTK_REQUIRE_LARGE_FILE_SUPPORT=ON \
#         $VTK_SOURCE_DIR

#         #-DCMAKE_INSTALL_RPATH=$VTK_INSTALL_PATH \
#         # -DVTK_Group_Qt=ON \
#         # -DVTK_QT_VERSION=5 \
#         # -DVTK_Group_Imaging=ON \
#         # -DVTK_Group_Views=ON \
#         # -DModule_vtkRenderingFreeTypeFontConfig=ON \
#         # -DVTK_WRAP_PYTHON=ON \
#         # -DVTK_PYTHON_VERSION=3 \
#         # -DPYTHON_EXECUTABLE=/usr/bin/python3 \
#         # -DPYTHON_INCLUDE_DIR=/usr/include/python3.6 \
#         # -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.6m.so \
#         # -DBUILD_TESTING=OFF \
#         # -DVTK_USE_SYSTEM_LIBRARIES=ON \
#         # -DVTK_USE_SYSTEM_LIBPROJ4=OFF \
#         # -DVTK_USE_SYSTEM_GL2PS=OFF \
#         # -DVTK_USE_SYSTEM_LIBHARU=OFF \
#         # -DVTK_USE_SYSTEM_PUGIXML=OFF \
#         # $VTK_SOURCE_DIR

#   cd "$VTK_BUILD_PATH"
#   make -j -l$BUILDTHREADS
#   make install
# fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "HWLOC"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libhwloc-dev
# brew install hwloc

# HWLOC_VER="2.2.0"
# HWLOC_TAR_FILE="hwloc-$HWLOC_VER.tar.gz"
# HWLOC_INSTALL_PATH=$SCRIPTPATH/local/hwloc/$HWLOC_VER/$BUILD_TYPE
# if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
#   HWLOC_INSTALL_PATH="/usr/local"
# fi
# HWLOC_BUILD_PATH=$BUILDDIR/hwloc/$BUILD_TYPE/
# HWLOC_SOURCE_DIR=$SOURCEDIR/hwloc/$HWLOC_VER

# hwloc_build="0"
# if [[ $hwloc_build -eq "1" ]]; then
#   # download library
#   cd $SOURCEDIR
#   if [ ! -f "$HWLOC_TAR_FILE" ]; then
#     wget "https://download.open-mpi.org/release/hwloc/v${HWLOC_VER%.*}/hwloc-${HWLOC_VER}.tar.gz"
#   fi

#   if [ ! -d "$HWLOC_SOURCE_DIR" ]; then
#     mkdir -p $HWLOC_SOURCE_DIR
#     tar -zxf $HWLOC_TAR_FILE -C $HWLOC_SOURCE_DIR --strip-components=1
#   fi

#   # build library
#   cd $BUILDDIR

#   if [ ! -d "$HWLOC_BUILD_PATH" ]; then
#     mkdir -p "$HWLOC_BUILD_PATH"

#     cd "$HWLOC_BUILD_PATH"

#     $HWLOC_SOURCE_DIR/configure --prefix=${HWLOC_INSTALL_PATH} --disable-opencl 
#   fi
#   cd "$HWLOC_BUILD_PATH"
#   make -j -l$BUILDTHREADS
#   make install
# fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "JEMALLOC"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libjemalloc-dev
# brew install jemalloc

# JEMALLOC_VER="5.1.0"
# JEMALLOC_TAR_FILE="jemalloc-$JEMALLOC_VER.tar.bz2"
# JEMALLOC_INSTALL_PATH=$SCRIPTPATH/local/jemalloc/$JEMALLOC_VER/$BUILD_TYPE
# if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
#   JEMALLOC_INSTALL_PATH="/usr/local"
# fi
# JEMALLOC_BUILD_PATH=$BUILDDIR/jemalloc/$BUILD_TYPE/
# JEMALLOC_SOURCE_DIR=$SOURCEDIR/jemalloc/$JEMALLOC_VER

# jemalloc_build="0"
# if [[ $jemalloc_build -eq "1" ]]; then
#   # download library
#   cd $SOURCEDIR
#   if [ ! -f "$JEMALLOC_TAR_FILE" ]; then
#     wget "https://github.com/jemalloc/jemalloc/releases/download/${JEMALLOC_VER}/$JEMALLOC_TAR_FILE"
#   fi

#   if [ ! -d "$JEMALLOC_SOURCE_DIR" ]; then
#     mkdir -p $JEMALLOC_SOURCE_DIR
#     tar xjf $JEMALLOC_TAR_FILE -C $JEMALLOC_SOURCE_DIR --strip-components=1
#   fi

#   cd $JEMALLOC_SOURCE_DIR
#   ./autogen.sh
#   ./configure --prefix=${JEMALLOC_INSTALL_PATH}
#   make -j -l$BUILDTHREADS
#   make install
# fi

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "HPX"
echo "<<<<<<<<<<< >>>>>>>>>>>"
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

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "FLANN"
echo "<<<<<<<<<<< >>>>>>>>>>>"
# sudo apt install libflann-dev
# brew install flann

echo "<<<<<<<<<<< >>>>>>>>>>>"
echo "PCL"
echo "<<<<<<<<<<< >>>>>>>>>>>"
PCL_VER="1.11.1"
PCL_TAR_FILE="pcl-$PCL_VER.tar.gz"
PCL_INSTALL_PATH=$SCRIPTPATH/local/pcl/$PCL_VER/$BUILD_TYPE
if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
  PCL_INSTALL_PATH="/usr/local"
fi
PCL_BUILD_PATH=$BUILDDIR/pcl/$PCL_VER/$BUILD_TYPE/
PCL_SOURCE_DIR=$SOURCEDIR/pcl/$PCL_VER

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
  else 
    rm -rf $PCL_BUILD_PATH
    mkdir -p $PCL_BUILD_PATH
  fi

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


             #-DWITH_VTK=OFF \
             #-DVTK_ROOT=$VTK_INSTALL_PATH \
             #-DCMAKE_INSTALL_RPATH=$PCL_INSTALL_PATH \
             #-DFLANN_ROOT="$FLANN_INSTALL_PATH" \
             #-DFLANN_LIBRARY="$FLANN_INSTALL_PATH/lib" \
             #-DFLANN_INCLUDE_DIR="$FLANN_INSTALL_PATH/include" \
             # -DBOOST_ROOT=$BOOST_INSTALL_PATH \
             # -DBoost_INCLUDE_DIR=$BOOST_INSTALL_PATH/include \
             # -DVTK_ROOT=$VTK_INSTALL_PATH \
 
  cd "$PCL_BUILD_PATH"
  make -j -l$BUILDTHREADS
  make install
fi

) |& tee "build.log"
