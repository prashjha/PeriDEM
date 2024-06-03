#!/bin/bash

VTK_DIR="/usr/local/lib/cmake/vtk-9.3"
METIS_DIR="/usr/lib"

## STEP 1 build PeriDEM documentation
git clone https://github.com/prashjha/PeriDEM.git 
cd PeriDEM 
git checkout remove_hpx 
cd .. 

mkdir -p peridem_build && cd peridem_build

# run cmake with flags
cmake 	\
	-DCMAKE_INSTALL_PREFIX="./" \
	-DCMAKE_BUILD_TYPE=Release \
	-DEnable_Documentation=ON \
	-DEnable_Tests=ON \
	-DEnable_High_Load_Tests=ON \
	-DDisable_Docker_MPI_Tests=OFF \
  -DVTK_DIR="${VTK_DIR}" \
  -DMETIS_DIR="${METIS_DIR}" \
	../PeriDEM

# make
make -j 12

make doc

## STEP 2 pull gh-pages branch and clean it
echo "clone for gh-pages. pwd = $(pwd)"
cd ../
git clone https://github.com/prashjha/PeriDEM.git build_docs

cd build_docs
echo "checkout gh-pages branch"
git checkout gh-pages || git checkout --orphan gh-pages

echo "clean the branch"
git rm -rf . #1> /dev/null

## STEP 3 bring new files to gh-branch
echo "create .nojekyll file"
touch .nojekyll

echo "create docs directory"
mkdir -p docs
ls -a

echo "begin copying. pwd = $(pwd)"
echo "copy repo directory except 'docs' and '.git' directories"
rsync -a --stats --progress ../PeriDEM/ ./ --exclude docs --exclude .git #1> /dev/null
rsync -a --stats --progress ../PeriDEM/docs/ ./docs/ --exclude doxy --exclude doxy.log 

echo "copying docs/doxy/html into ./"
rsync -a --stats --progress ../PeriDEM/docs/doxy/html/ ./

echo "check if files are copied"
if [ -f "./index.html" ]; then 
  echo "files copied successfully."
else
  echo "index.html file does not exist"
fi

## STEP 4 add, commit, and push
echo "add changes and commit"
git add -A
git commit -m "automated deployment to GitHub Pages" --allow-empty

echo "push"
git push origin gh-pages
