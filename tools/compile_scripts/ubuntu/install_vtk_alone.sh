#!/bin/sh

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12"
INSTALLATGLOBAL="0" # if 1, then libraries will be installed in /usr/local <-- for containers

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

echo " "
echolog "SCRIPTPATH=\"$SCRIPTPATH\"
SOURCEDIR=\"$SOURCEDIR\"
BUILDDIR=\"$BUILDDIR\""

echo "Shell: $(which sh)
BUILD_TYPE=$BUILD_TYPE
"

echo "<<<<<<<<<<< >>>>>>>>>>>
CMAKE
<<<<<<<<<<< >>>>>>>>>>>
"
CMAKE_EXE="cmake"

echo " 
CMAKE_EXE=$CMAKE_EXE
$($CMAKE_EXE --version)
"

echo "<<<<<<<<<<< >>>>>>>>>>>
VTK
<<<<<<<<<<< >>>>>>>>>>>
"
VTK_MAJOR_VERSION=9
VTK_MINOR_VERSION=3
VTK_PATCH_VERSION=0
VTK_VERSION=$VTK_MAJOR_VERSION.$VTK_MINOR_VERSION.$VTK_PATCH_VERSION
VTK_INSTALL_PATH=$SCRIPTPATH/local/vtk/$VTK_VERSION/Release
if [[ "$INSTALLATGLOBAL" -eq "1" ]]; then
    VTK_INSTALL_PATH="/usr/local"
fi
VTK_BUILD_PATH=$BUILDDIR/vtk/$VTK_VERSION/Release/
VTK_SOURCE_RELATIVE_DIR=vtk/$VTK_VERSION
VTK_SOURCE_DIR=$SOURCEDIR/$VTK_SOURCE_RELATIVE_DIR


# download repository
cd $SOURCEDIR
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git ${VTK_SOURCE_RELATIVE_DIR}
cd ${VTK_SOURCE_RELATIVE_DIR}
git checkout v${VTK_VERSION} 

# build library
cd $BUILDDIR

if [ ! -d "$VTK_BUILD_PATH" ]; then
    mkdir -p "$VTK_BUILD_PATH"
else 
    rm -rf $VTK_BUILD_PATH
    mkdir -p $VTK_BUILD_PATH
fi

cd "$VTK_BUILD_PATH"

$CMAKE_EXE -D CMAKE_BUILD_TYPE:STRING=Release \
    -D CMAKE_INSTALL_PREFIX:STRING=${VTK_INSTALL_PATH} \
    -D BUILD_SHARED_LIBS=ON \
    -D BUILD_TESTING=OFF \
    -D VTK_REQUIRED_OBJCXX_FLAGS='' \
    -D HDF5_BUILD_FRAMEWORKS=OFF \
    -D VTK_BUILD_DOCUMENTATION=OFF \
    -D VTK_BUILD_EXAMPLES=OFF \
    -D VTK_BUILD_SCALED_SOA_ARRAYS=OFF \
    -D VTK_BUILD_SPHINX_DOCUMENTATION=OFF \
    -D VTK_GROUP_ENABLE_MPI=NO \
    -D VTK_GROUP_ENABLE_Qt=DONT_WANT \
    -D VTK_GROUP_ENABLE_Rendering=NO \
    -D VTK_GROUP_ENABLE_Views=NO \
    -D VTK_GROUP_ENABLE_Web=NO \
    -D VTK_Group_MPI=ON \
    -D VTK_USE_MPI=OFF \
    -D VTK_WRAP_PYTHON=OFF \
    ${VTK_SOURCE_DIR}

cd "$VTK_BUILD_PATH"
make -j -l$BUILDTHREADS
make install

echolog "
##
VTK_INSTALL_PATH=\"$VTK_INSTALL_PATH\""

## provide summary
echo " 

>> Dependencies are installed!!

>> Here is summary of various paths (particularly note the VTK path that is needed to install PeriDEM)

"

echo "$(<$path_file)"

) 2>&1 |  tee "build.log"
