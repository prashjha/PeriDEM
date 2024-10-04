import os
import numpy as np
import csv
import sys

def print_point(point, area, prefix = ""):
	str = prefix + " area = " + "%4.6e" %(area) + ", point = ["
	N = 2
	for i in xrange(N):
		str += "%4.6e" % (point[i])
		if i < N - 1:
			str += ", "
		else:
			str += "]\n"

	print(str)

def mult(matrix, vector):
	return [matrix[0][0] * vector[0] + matrix[0][1] * vector[1], matrix[1][0] * vector[0] + matrix[1][1] * vector[1]]

def discretize_circle(radius, x_one):
	"""Generates discretization of circle so that area of all nodes are same"""

	# particle data
	sim_particle_r = radius

	# store location of nodes 
	sim_particles = []

	# store area of nodes
	sim_particles_area = []

	# get angle
	sim_num_theta = 8
	sim_theta_interval = 2. * np.pi / (float(sim_num_theta))
	angle = 0.

	# get matrix
	matrix = []
	a = [np.cos(angle), -np.sin(angle)]
	matrix.append(a)
	a = [np.sin(angle), np.cos(angle)]
	matrix.append(a)

	# add first point
	p = [1., 0.]		
	p = mult(matrix, p)
	sim_particles.append(p)
	area = sim_theta_interval * (1. - (1. + x_one) * (1. + x_one) * 0.25)
	sim_particles_area.append(area)
	print_point(p, area, "angle = %4.6e\n" %(angle))

	# add first point
	p = [x_one, 0.]		
	p = mult(matrix, p)
	sim_particles.append(p)
	sim_particles_area.append(area)
	print_point(p, area, "angle = %4.6e\n" %(angle))

	continue_add = True
	x_pre_pre = 1.
	x_pre = x_one
	counter = 1
	# loop over internal points
	while continue_add == True:
		# 
		val = (x_pre + x_pre_pre) * (x_pre + x_pre_pre) - 4. * area / (sim_theta_interval)

		if val < 0.:
			continue_add = False

		if continue_add == True:

			counter = counter + 1

			if counter % 2 != 0:

				print('counter %d\n' % (counter))

				# get next internal point
				x_new = np.sqrt(val) - x_pre


				#
				p = [x_new, 0.]
				p = mult(matrix, p)

				# add point and area
				sim_particles.append(p)
				sim_particles_area.append(area)

				area_new = sim_theta_interval * ((x_pre + x_pre_pre) * (x_pre + x_pre_pre) * 0.25 - (x_pre + x_new) * (x_pre + x_new) * 0.25)

				print_point(p, area_new)

				x_pre_pre = x_pre
				x_pre = x_new

				# if len(sim_particles) > 20:
					# continue_add = False
			else:
				print('skipped')


	# end of while loop

	# skip some points
	sim_particles_new = []
	sim_particles_area_new = []
	for i in xrange(len(sim_particles)):
		continue_i = True

		if i > 1:
			if (i+1) % 2 != 0:
				continue_i = False

		if continue_i == True:

			x = [sim_particles[i][0], sim_particles[i][1]]
			sim_particles_new.append(x)
			sim_particles_area_new.append(sim_particles_area[i])

			# skip some points
	sim_particles_nn = []
	sim_particles_area_nn = []
	for i in xrange(len(sim_particles_new)):
		
		x = [sim_particles_new[i][0], sim_particles_new[i][1]]
		sim_particles_nn.append(x)

		x_next = [0., 0.]
		if i < len(sim_particles_new) - 2:
			x_next = [sim_particles_new[i+1][0], sim_particles_new[i+1][1]]

		x_prev = [0., 0.]
		if i > 0:
			x_prev = [sim_particles_new[i-1][0], sim_particles_new[i-1][1]]

		# area
		area_new = 0.
		if i == 0:
			area_new = sim_theta_interval * (1. * 1. - (1. + x[0]) * (1. + x[0]) * 0.25)
		elif i == len(sim_particles) - 1:
			area_new = sim_theta_interval * ((x_prev[0] + x[0]) * (x_prev[0] + x[0]) * 0.25)
		else:
			area_new = sim_theta_interval * ((x_prev[0] + x[0]) * (x_prev[0] + x[0]) * 0.25 - (x_next[0] + x[0]) * (x_next[0] + x[0]) * 0.25)

		sim_particles_area_nn.append(area_new)

	# rotate all points
	sim_particles_new_new = []
	sim_particles_area_new_new = []
	for theta in xrange(sim_num_theta):
		angle = theta * sim_theta_interval

		# get matrix
		matrix = []
		a = [np.cos(angle), -np.sin(angle)]
		matrix.append(a)
		a = [np.sin(angle), np.cos(angle)]
		matrix.append(a)

		for i in xrange(len(sim_particles_nn)):
			x_old = [radius * sim_particles_nn[i][0], radius * sim_particles_nn[i][1]]
			area = radius * radius * sim_particles_area_nn[i]

			x_new = mult(matrix, x_old)

			sim_particles_new_new.append(x_new)
			sim_particles_area_new_new.append(area)

		# loop points in y=0 line

	# loop over angles

	# print points to csv file
	# generate csv file
	inpf = open('circle_mesh.csv','w')

	# header
	# inpf.write("i, x, y, z, area\n")

	for i in xrange(len(sim_particles_new_new)):
		inpf.write("%d, %Lf, %Lf, %Lf, %Lf\n" % (i, sim_particles_new_new[i][0], sim_particles_new_new[i][1], 0., sim_particles_area_new_new[i]))

	inpf.close()

discretize_circle(1., 0.9)
