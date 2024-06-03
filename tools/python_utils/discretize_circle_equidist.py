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

def discretize_circle(radius, num_r_points, num_theta):
	"""Generates discretization of circle so that area of all nodes are same"""

	# particle data
	sim_particle_r = radius

	# store location of nodes 
	sim_particles = []

	# store area of nodes
	sim_particles_area = []

	# get angle
	sim_num_theta = num_theta
	sim_theta_interval = 2. * np.pi / (float(sim_num_theta))
	angle = 0.

	# loop over internal points
	for i in xrange(num_r_points+1):

		x = [radius - i * radius / (float(num_r_points)), 0.]

		sim_particles.append(x)
		
		x_next = [0., 0.]
		if i < num_r_points - 1:
			x_next = [radius - (i+1.) * radius / (float(num_r_points)), 0.]

		x_prev = [0., 0.]
		if i > 0:
			x_prev = [radius - (i-1.) * radius / (float(num_r_points)), 0.]

		# compute area
		area_new = 0.
		if i == 0:
			area_new = sim_theta_interval * (1. - (x[0] + x_next[0]) * (x[0] + x_next[0]) * 0.25)
		elif i == num_r_points:
			area_new = np.pi * ((x_prev[0] + x[0]) * (x_prev[0] + x[0]) * 0.25)
		else:
			area_new = sim_theta_interval * ((x_prev[0] + x[0]) * (x_prev[0] + x[0]) * 0.25 - (x_next[0] + x[0]) * (x_next[0] + x[0]) * 0.25)

		sim_particles_area.append(area_new)
		print_point(x, area_new)

	# rotate all points
	sim_particles_new = []
	sim_particles_area_new  = []
	for theta in xrange(sim_num_theta):
		angle = theta * sim_theta_interval

		# get matrix
		matrix = []
		a = [np.cos(angle), -np.sin(angle)]
		matrix.append(a)
		a = [np.sin(angle), np.cos(angle)]
		matrix.append(a)

		for i in xrange(len(sim_particles)):

			x_old = [sim_particles[i][0], sim_particles[i][1]]
			area = sim_particles_area[i]

			x_new = mult(matrix, x_old)

			continue_i = True
			if i == len(sim_particles) - 1 and theta > 0:
				continue_i = False

			if continue_i == True:
				sim_particles_new.append(x_new)
				sim_particles_area_new.append(area)

	# print points to csv file
	# generate csv file
	inpf = open('circle_mesh.csv','w')

	# header
	inpf.write("id, x, y, volume\n")

	for i in xrange(len(sim_particles_new)):
		inpf.write("%d, %Lf, %Lf,%Lf\n" % (i, sim_particles_new[i][0], sim_particles_new[i][1], sim_particles_area_new[i]))

	inpf.close()

discretize_circle(0.001, 10, 20)
