#!/bin/bash
MY_PWD=$(pwd)

declare -a dirs=( "attrition_tests/attrition_mix_particles_large_set" \
				  "attrition_tests/attrition_mix_particles_small_set" \
				  "compressive_test/compression_large_set" \
				  "compressive_test/compression_small_set" \
				  "single_particle/single_particle_circle" \
				  "single_particle/single_particle_rectangle_inbuilt_mesh" \
				  "two_particles/twop_circ" \
				  "two_particles/twop_concave_and_hex" \
				  "two_particles/twop_wall_concave_diff_material_diff_size")

# loop over directories
for d in ${dirs[@]}; do
	
	cd $MY_PWD

	pd="$MY_PWD""/""$d"
	echo "****** Directory = $d ******"
	echo " "
	cd $d

	# Optional: specify number of threads by passing the argument to script
	# default is 2
	./run.sh 2
	
done
