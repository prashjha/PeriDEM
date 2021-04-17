#!/bin/bash
MY_PWD=$(pwd)

declare -a dirs=("two_particles/circ_damp" \
								 "two_particles/circ_damp_diff_radius" \
								 "two_particles/circ_diff_material" \
								 "two_particles/circ_diff_radius_diff_material" \
								 "two_particles/circ_no_damp" \
								 "two_particles_wall/concave_diff_material_diff_size" \
								 "compressive_test/n500_circ_hex/init_config") # \
								 #"compressive_test/n500_circ_hex/run1" \
								 #"compressive_test/n500_circ_hex/run2")

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
