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

def discretize_circle(radius, num_r_points):
	"""Generates discretization of circle so that area of all nodes are same"""

	# particle data
	sim_particle_r = radius

	# store location of nodes 
	sim_particles = []

	# store area of nodes
	sim_particles_area = []

	# width of annuli
	sim_num_r_points = num_r_points
	sim_annlui_width = 1. / sim_num_r_points

	# loop over annulus
	for i in xrange(sim_num_r_points):

		radius_inner = (i + 1.) * sim_annlui_width
		radius_outer = (i) * sim_annlui_width

		# num divisions in angle
		dn = 6 * (i + 1) - 3
		dtheta = 2. * np.pi / float(dn)

		# loop over theta
		for theta in xrange(dn):

			# get point
			radius_quad = (i - 1. / 2.) * sim_annlui_width
			x = [radius_quad * np.cos(theta * dtheta), radius_quad * np.sin(theta * dtheta)]

			area = sim_annlui_width * sim_annlui_width * np.pi / 3.

			sim_particles.append(x)
			sim_particles_area.append(area)
			print_point(x, area)

		# End loop over theta

	# End loop over annulus

	# 
	# loop over points again and scale them to circle of given radius
	#
	sim_particles_new = []
	sim_particles_area_new = []
	for i in xrange(len(sim_particles)):

		x = [radius * sim_particles[i][0], radius * sim_particles[i][1]]
		area = radius * radius * sim_particles_area[i]
		sim_particles_new.append(x)
		sim_particles_area_new.append(area)

	# End loop over particles

	# print points to csv file
	# generate csv file
	inpf = open('mesh_cir.csv','w')

	# header
	inpf.write("id, x, y, volume\n")

	for i in xrange(len(sim_particles_new)):
		inpf.write("%d, %4.6e, %4.6e, %4.6e\n" % (i, sim_particles_new[i][0], sim_particles_new[i][1], sim_particles_area_new[i]))

	inpf.close()

discretize_circle(0.001, 10)
