#!/bin/bash
(
MY_PWD=$(pwd)
# locate executable
execsrc="../../../../../bin/PeriDEM"
nts="2"
if [[ $# -gt 0 ]]; then
	nts="$1"
fi

if [[ ! -d "out" ]]; then
	mkdir "out"
fi

# go to input directory
cd "inp"

# get tag from command line
pp_tag="0"
if [[ $# -gt 1 ]]; then
	pp_tag="$2"
fi

f_suf="_$pp_tag"
echo "Setting up problem ... "
python3 -B problem_setup.py $pp_tag

echo "Creating mesh for particles ... "
f_p_1_mesh="mesh_cir_1$f_suf"
gmsh "$f_p_1_mesh.geo" -2 &> /dev/null
gmsh "$f_p_1_mesh.geo" -2 -o "$f_p_1_mesh.vtk" &> /dev/null

f_p_2_mesh="mesh_cir_2$f_suf"
gmsh "$f_p_2_mesh.geo" -2 &> /dev/null
gmsh "$f_p_2_mesh.geo" -2 -o "$f_p_2_mesh.vtk"  &> /dev/null

echo "Running PeriDEM ... "
f_inp="input$f_suf.yaml"
"$execsrc" -i "$f_inp" -nThreads $nts

) 2>&1 |  tee output.log
