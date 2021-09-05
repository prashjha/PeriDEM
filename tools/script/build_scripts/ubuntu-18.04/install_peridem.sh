#!/bin/bash

(

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
SOURCEDIR="$SCRIPTPATH/source/"
BUILDDIR="$SCRIPTPATH/build/"
BUILDTHREADS="12"
INSTALLATGLOBAL="0" # if 1, then libraries will be installed in /usr/local <-- for containers

peridem_build="1" # set to 1 to install and 0 to do nothing

# some preliminary setup
if [ ! -d "$SOURCEDIR" ]; then
    mkdir -p "$SOURCEDIR"
fi
if [ ! -d "$BUILDDIR" ]; then
    mkdir -p "$BUILDDIR"
fi

## open a file to write various key paths for subsequent use
path_file="$SCRIPTPATH/paths_peridem.txt"

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

# echo "<<<<<<<<<<< >>>>>>>>>>>"
# echo "CMAKE"
# echo "<<<<<<<<<<< >>>>>>>>>>>"
CMAKE_EXE="cmake"
CMAKE_INSTALL_PATH="/usr/bin"

# echo "<<<<<<<<<<< >>>>>>>>>>>"
# echo "HPX"
# echo "<<<<<<<<<<< >>>>>>>>>>>"
HPX_INSTALL_PATH="/home/prashant/work/peridem_works/test_PeriDEM_install_scripts/ubuntu-18.04/local/hpx/1.3.0/Release"
if [[ $HPX_INSTALL_PATH -eq "0" ]]; then
  echo "Please provide path where HPX is installed in HPX_INSTALL_PATH variable and then rerun the script again."
  exit
fi


echo "<<<<<<<<<<< >>>>>>>>>>>
PERIDEM
<<<<<<<<<<< >>>>>>>>>>>"
PERIDEM_VER="0.1.0"
PERIDEM_BUILD_PATH=$SCRIPTPATH/peridem/$1/
PERIDEM_SOURCE_DIR=$SOURCEDIR/peridem/$PERIDEM_VER

if [[ $peridem_build -eq "1" ]]; then
  # download library
  cd $SOURCEDIR
  
  if [ ! -d $PERIDEM_SOURCE_DIR ]; then
    echo "Cloning PeriDEM"
    mkdir -p $PERIDEM_SOURCE_DIR
    git clone git@github.com:prashjha/PeriDEM.git $PERIDEM_SOURCE_DIR
  fi

  # build library
  if [ ! -d "$PERIDEM_BUILD_PATH" ]; then
    echo "Creating PeriDEM build path"
    mkdir -p "$PERIDEM_BUILD_PATH"
  else 
    echo "PeriDEM build path exist, deleting and then creating the dir again"
    rm -rf $PERIDEM_BUILD_PATH
    mkdir -p $PERIDEM_BUILD_PATH
  fi

  cd "$PERIDEM_BUILD_PATH"

  # add when building in mac
  # -DYAML_CPP_DIR="/usr/local" \
  $CMAKE_EXE -DCMAKE_BUILD_TYPE=$BUILD_TYPE   \
        -DHPX_DIR="$HPX_INSTALL_PATH/lib/cmake/HPX" \
        -DEnable_Documentation=ON \
        -DEnable_Tests=ON \
        $PERIDEM_SOURCE_DIR

  cd "$PERIDEM_BUILD_PATH"
  echo "
  Building library
  "
  make -j -l$BUILDTHREADS
  echo "
  Running ctest
  "
  ctest --verbose
fi 

if [[ -f ${PERIDEM_BUILD_PATH}/bin/PeriDEM ]]; then
  echo "

>> PeriDEM is built in dir 

$PERIDEM_BUILD_PATH

  "
else
  echo "

>> PeriDEM executible not found! 

>> Check if paths are specified correctly and HPX is installed

>> Report issue if unable to fix the error!

  "
fi

echo ">> Here is summary of key paths

"

echolog "
## cmake
CMAKE_INSTALL_PATH=\"$CMAKE_INSTALL_PATH\"

## HPX
HPX_INSTALL_PATH=\"$HPX_INSTALL_PATH\"

## PeriDEM
PERIDEM_SOURCE_DIR=\"$PERIDEM_SOURCE_DIR\"
PERIDEM_BUILD_PATH=\"$PERIDEM_BUILD_PATH\"
"

) |& tee "build_peridem.log"
