import os
import numpy as np
import sys

def print_bool(arg, prefix = ""):

  str = prefix
  if arg == True:
    str += "True\n"
  else:
    str += "False\n"
  return str

def print_dbl(arg, prefix = ""):

  str = prefix + "%4.6e\n" % (arg)
  return str

def print_int(arg, prefix = ""):
  str = prefix + "%d\n" % (arg)
  return str

def print_dbl_list(arg, prefix = ""):
  str = prefix + "["
  N = len(arg)
  for i in range(N):
    str += "%4.6e" % (arg[i])
    if i < N - 1:
      str += ", "
    else:
      str += "]\n"

  return str

def print_int_list(arg, prefix = ""):
  str = prefix + "["
  N = len(arg)
  for i in range(N):
    str += "%d" % (arg[i])
    if i < N - 1:
      str += ", "
    else:
      str += "]\n"

  return str

def does_intersect(p, r, R, particles, padding):

  for q in particles:

    pq = np.array([p[i] - q[i] for i in range(3)])
    if np.linalg.norm(pq) <= r + R + padding:
      return True

  return False


def get_E(K, nu):
  return 3. * K * (1. - 2. * nu)

def get_G(E, nu):
  return E / (2. * (1. + nu))


def get_eff_k(k1, k2):
  return 2. * k1 * k2 / (k1 + k2)

def rect_to_five_param(r):
  return [r[3] - r[0], r[4] - r[1], 0.5*(r[0] + r[3]), 0.5*(r[1] + r[4]), 0.5*(r[2] + r[5])]

def particle_locations(inp_dir, pp_tag, R1, R2, offset, w1_rect_five_format, w2_rect_five_format):
  """Generate particle location data"""

  sim_particles = []
  sim_particles.append([0., R1, R1, 0., R1])
  sim_particles.append([1., R1, 2. * R1 + R2 + offset, 0., R2])

  sim_particles.append([2., w1_rect_five_format[2], w1_rect_five_format[3], w1_rect_five_format[4], w1_rect_five_format[0]])
  sim_particles.append([3., w2_rect_five_format[2], w2_rect_five_format[3], w2_rect_five_format[4], w2_rect_five_format[0]])

  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  inpf.write("i, x, y, z, r\n")
  for p in sim_particles:
    inpf.write("%d, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4]))

  inpf.close()

def particle_locations_orient(inp_dir, pp_tag, R1, R2, offset, w1_rect_five_format, w2_rect_five_format):
  """Generate particle location data"""

  sim_particles = []
  sim_particles.append([0., R1, R1, 0., R1, 0.5*np.pi])
  sim_particles.append([1., R1+1.*R1*np.sin(np.pi/3.), 2. * R1 + R2 + offset, 0., R2, 0.3*np.pi])

  sim_particles.append([2., w1_rect_five_format[2], w1_rect_five_format[3], w1_rect_five_format[4], w1_rect_five_format[0], 0.])
  sim_particles.append([3., w2_rect_five_format[2], w2_rect_five_format[3], w2_rect_five_format[4], w2_rect_five_format[0], 0.])

  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  inpf.write("i, x, y, z, r, o\n")
  for p in sim_particles:
    inpf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4], p[5]))

  inpf.close()

def rotate(p, theta, axis):

  p_np = np.array(p)
  axis_np = np.array(axis)

  ct = np.cos(theta);
  st = np.sin(theta);

  # dot
  p_dot_n = np.dot(p_np,axis_np)

  # cross
  n_cross_p = np.cross(axis_np, p_np)

  return (1. - ct) * p_dot_n * axis_np + ct * p_np + st * n_cross_p


def get_ref_drum_points(center, radius, width, add_center = False):

  # drum2d
  #
  #             v3                                v2
  #               +                               +
  #
  #
  #                      +         o           +
  #                     v4         x            v1
  #
  #               +                                +
  #               v5                               v6
  #
  # Axis is a vector from x to v1
  #

  axis = [1., 0., 0.]

  # center and radius
  sim_Cx = center[0]
  sim_Cy = center[1]
  sim_Cz = center[2]
  sim_radius = radius

  rotate_axis = [0., 0., 1.]

  x1 = rotate(axis, np.pi/3., rotate_axis)
  x2 = rotate(axis, -np.pi/3., rotate_axis)

  points = []
  if add_center:
    points.append(center)

  # v1
  points.append([center[i] + width*0.5*axis[i] for i in range(3)])
  # v2  
  points.append([center[i] + radius*x1[i] for i in range(3)])
  # v3  
  points.append([center[i] + radius*x1[i] - radius*axis[i] for i in range(3)])
  # v4
  points.append([center[i] - width*0.5*axis[i] for i in range(3)])
  # v5
  v6 = [center[i] + radius*x2[i] for i in range(3)]
  points.append([v6[i] - radius*axis[i] for i in range(3)])
  # v6
  points.append(v6)

  return points


def generate_particle_gmsh_input(inp_dir, filename, center, radius, width, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # points
  points = get_ref_drum_points(center, radius, width, True)

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  #
  # points
  #
  for i in range(7):
    p = points[i]
    sts = "Point({}) = ".format(i+1)
    sts += "{"
    sts += "{}, {}, {}, {}".format(p[0], p[1], p[2], mesh_size)
    sts += "};\n"
    geof.write(sts);

  #
  # circlular arc
  #
  geof.write("Line(1) = {2, 3};\n")
  geof.write("Line(2) = {3, 4};\n")
  geof.write("Line(3) = {4, 5};\n")
  geof.write("Line(4) = {5, 6};\n")
  geof.write("Line(5) = {6, 7};\n")
  geof.write("Line(6) = {7, 2};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(1) = {1, 2, 3, 4, 5, 6};\n")

  #
  # plane surface
  #
  geof.write("Plane Surface(1) = {1};\n")

  #
  # physical surface
  #
  # tag = '"' + "a" + '"'
  # geof.write("Physical Surface(%s) = {1};\n" % (tag))

  # # add center point to plane surface
  geof.write("Point{1} In Surface {1};")

  # close file
  geof.close()


def generate_wall_gmsh_input(inp_dir, filename, rectangle, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # outer rectangle
  sim_Lx_out1 = rectangle[0]
  sim_Ly_out1 = rectangle[1]
  sim_Lz_out1 = rectangle[2]
  sim_Lx_out2 = rectangle[3]
  sim_Ly_out2 = rectangle[4]
  sim_Lz_out2 = rectangle[5]

  # mesh size
  sim_h = mesh_size

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  #
  # points
  #
  geof.write("Point(1) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out1, sim_Ly_out1, sim_Lz_out1, sim_h));
  geof.write("Point(2) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out2, sim_Ly_out1, sim_Lz_out1, sim_h))
  geof.write("Point(3) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out2, sim_Ly_out2, sim_Lz_out1, sim_h))
  geof.write("Point(4) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out1, sim_Ly_out2, sim_Lz_out1, sim_h))

  #
  # lines
  #
  geof.write("Line(1) = {1, 2};\n")
  geof.write("Line(2) = {2, 3};\n")
  geof.write("Line(3) = {3, 4};\n")
  geof.write("Line(4) = {4, 1};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(1) = {1, 2, 3, 4};\n")

  #
  # plane surface
  #
  geof.write("Plane Surface(1) = {1};\n")

  #
  # physical surface
  #
  tag = '"' + "a" + '"'
  geof.write("Physical Surface(%s) = {1};\n" % (tag))

  # close file
  geof.close()


def create_input_file(inp_dir, pp_tag):
  """Generates input file for two-particle test"""

  sim_inp_dir = str(inp_dir)

  ## 1 - bottom    2 - top    3 - wall 1    4 - wall 2 
  center = [0., 0., 0.]
  R1 = 0.001
  R2 = 0.0015
  mesh_size = R1 / 5.
  if R2 < R1:
    mesh_size = R2 / 5.

  horizon = 2. * mesh_size
  particle_dist = -R1

  ## wall 1 (at bottom)
  wall1_bottom_particle_dist = 1.5 * mesh_size
  wall1_top_edge_y = 0. - wall1_bottom_particle_dist
  wall1_rect = [-R1, wall1_top_edge_y - 2*horizon, 0., 3.*R1, wall1_top_edge_y, 0.]
  if R2 > R1:
    wall1_rect[0] = R1-2.*R2
    wall1_rect[3] = R1+2.*R2


  # for peridem input cv file for particle locations and orientation,
  # we need to represent rectangle using five parameters
  # [Lx, Ly, xc1, xc2, xc3] where (xc1, xc2, xc3) is center of rectangle
  wall1_five_param_format = rect_to_five_param(wall1_rect)
  wall1_gmsh_input_rect = [-0.5*wall1_five_param_format[0], -0.5*wall1_five_param_format[1], 0., 0.5*wall1_five_param_format[0], 0.5*wall1_five_param_format[1], 0.]
  wall1_peridem_input_param = [wall1_five_param_format[0], wall1_five_param_format[1], 0., 0., 0.]
  wall1_peridem_input_param[2] = 0.
  wall1_peridem_input_param[3] = 0.
  wall1_peridem_input_param[4] = 0.

  ## wall 2 (at top)
  ## center of top wall is (R1, 2R1 + R2 + offset), where
  ## offset = particle_dist - free_fall_dist
  ## free_fall_dist = particle_dist - horizon
  ## so
  ## offset = horizon
  wall2_top_particle_dist = 4 * mesh_size
  top_particle_center_y = 2*R1 + R2 + horizon
  wall2_bottom_edge_y = top_particle_center_y + R2 + wall2_top_particle_dist
  wall2_rect = [-R1, wall2_bottom_edge_y, 0., 3.*R1, wall2_bottom_edge_y + 2*horizon, 0.]
  if R2 > R1:
    wall2_rect[0] = R1-2.*R2
    wall2_rect[3] = R1+2.*R2

  wall2_five_param_format = rect_to_five_param(wall2_rect)
  wall2_gmsh_input_rect = [-0.5*wall2_five_param_format[0], -0.5*wall2_five_param_format[1], 0., 0.5*wall2_five_param_format[0], 0.5*wall2_five_param_format[1], 0.]
  wall2_peridem_input_param = [wall2_five_param_format[0], wall1_five_param_format[1], 0., 0., 0.]
  wall2_peridem_input_param[2] = 0.
  wall2_peridem_input_param[3] = 0.
  wall2_peridem_input_param[4] = 0.

  ## time 
  final_time = 0.016
  num_steps = 80000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 40
  dt_out_n = num_steps / num_outputs
  perform_out = True

  ## material
  poisson1 = 0.25
  rho1 = 1200.
  K1 = 2.16e+5
  E1 = get_E(K1, poisson1)
  G1 = get_G(E1, poisson1)
  Gc1 = 500.

  poisson2 = 0.25
  rho2 = 1200.
  K2 = 2.e+6
  E2 = get_E(K2, poisson2)
  G2 = get_G(E2, poisson2)
  Gc2 = 500.

  # wall 1 (at bottom)
  poisson3 = 0.25
  rho3 = 1200.
  K3 = 2.e+6
  E3 = get_E(K3, poisson3)
  G3 = get_G(E3, poisson3)
  Gc3 = 500.

  # wall 2 (at top) --- same as wall 1 so we will copy in input data
  poisson4 = 0.25
  rho4 = 1200.
  K4 = 2.e+9
  E4 = get_E(K4, poisson4)
  G4 = get_G(E4, poisson4)
  Gc4 = 500.

  ## contact
  # R_contact = 0.95 * mesh_size 
  # R_contact = 1.74e-04
  R_contact_factor = 0.95

  # Kn_V_max = 7.385158e+05
  # Kn = np.power(Kn_V_max, 2)
  # compute from bulk modulus

  # from bulk modulus
  Kn_11 = 18. * get_eff_k(K1, K1) / (np.pi * np.power(horizon, 5))
  Kn_22 = 18. * get_eff_k(K2, K2) / (np.pi * np.power(horizon, 5))
  Kn_33 = 18. * get_eff_k(K3, K3) / (np.pi * np.power(horizon, 5))
  Kn_44 = 18. * get_eff_k(K4, K4) / (np.pi * np.power(horizon, 5))
  Kn_12 = 18. * get_eff_k(K1, K2) / (np.pi * np.power(horizon, 5))
  Kn_13 = 18. * get_eff_k(K1, K3) / (np.pi * np.power(horizon, 5))
  Kn_14 = 18. * get_eff_k(K1, K4) / (np.pi * np.power(horizon, 5))
  Kn_23 = 18. * get_eff_k(K2, K3) / (np.pi * np.power(horizon, 5))
  Kn_24 = 18. * get_eff_k(K2, K4) / (np.pi * np.power(horizon, 5))
  Kn_34 = 18. * get_eff_k(K3, K4) / (np.pi * np.power(horizon, 5))

  Kn_factor = 10.
  beta_n_eps = 0.95
  friction_coeff = 0.5
  damping_active = False
  friction_active = False 
  beta_n_factor = 100.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## assign free fall velocity to second particle
  free_fall_dist = particle_dist - horizon
  free_fall_vel = [0., 0., 0.]
  #free_fall_vel[1] = -np.sqrt(2. * np.abs(gravity[1]) * free_fall_dist) 
  free_fall_vel[1] = -1

  ## neighbor search details
  neigh_search_factor = 10.
  neigh_search_interval = 100
  neigh_search_criteria = "simple_all"


  ### ---------------------------------------------------------------- ###
  # generate YAML file
  ### ---------------------------------------------------------------- ###
  # print('\nGenerating imput file\n')
  inpf = open(sim_inp_dir + 'input_' + str(pp_tag) + '.yaml','w')
  inpf.write("Model:\n")
  inpf.write("  Dimension: 2\n")
  inpf.write("  Discretization_Type:\n")
  inpf.write("    Spatial: finite_difference\n")
  inpf.write("    Time: central_difference\n")
  inpf.write("  Final_Time: %4.6e\n" % (final_time))
  inpf.write("  Time_Steps: %d\n" % (num_steps))

  #
  # container info
  #
  inpf.write("Container:\n")
  inpf.write("  Geometry:\n")
  inpf.write("    Type: rectangle\n")
  contain_params = [-R1, -horizon - wall1_bottom_particle_dist, 0., 3.*R1, 2.*R1 + 2.*R2 + wall2_top_particle_dist + horizon, 0.]
  if R2 > R1:
    contain_params[0] = R1-2.*R2
    contain_params[3] = R1+2.*R2
  contain_peridem_input_param = rect_to_five_param(contain_params)
  inpf.write("    Parameters: " + print_dbl_list(contain_params))

  #
  # zone info
  #
  inpf.write("Zone:\n")
  inpf.write("  Zones: 4\n")

  ## zone 1 (bottom particle)
  inpf.write("  Zone_1:\n")
  inpf.write("    Is_Wall: false\n")

  ## zone 2 (top particle)
  inpf.write("  Zone_2:\n")
  inpf.write("    Is_Wall: false\n")

  ## zone 3 (wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    Is_Wall: true\n")

  ## zone 4 (wall)
  inpf.write("  Zone_4:\n")
  inpf.write("    Is_Wall: true\n")

  #
  # particle info
  #
  inpf.write("Particle:\n")
  inpf.write("  Test_Name: two_particle_two_wall\n")
  
  inpf.write("  Zone_1:\n")
  inpf.write("    Type: drum2d\n")
  drum1_axis = [1., 0., 0.]
  drum1_neck_width = 0.5*R1
  p1_geom = [R1, drum1_neck_width, center[0], center[1], center[2], drum1_axis[0], drum1_axis[1], drum1_axis[2]]
  inpf.write("    Parameters: " + print_dbl_list(p1_geom)) 
  
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: drum2d\n")
  
  drum2_axis = [1., 0., 0.] #[np.cos(0.1*np.pi), np.sin(0.1*np.pi), 0.]
  drum2_neck_width = 0.5*R2
  p2_geom = [R2, drum2_neck_width, center[0], center[1], center[2], drum2_axis[0], drum2_axis[1], drum2_axis[2]]
  inpf.write("    Parameters: " + print_dbl_list(p2_geom)) 

  inpf.write("  Zone_3:\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(wall1_peridem_input_param))
  inpf.write("    All_Dofs_Constrained: true\n")

  inpf.write("  Zone_4:\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(wall2_peridem_input_param))
  inpf.write("    All_Dofs_Constrained: true\n")

  #
  # particle generation
  #
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations_" + str(pp_tag) + ".csv\n")
  # specify that we also provide the orientation information in the file
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

  #
  # Mesh info
  #
  inpf.write("Mesh:\n")

  ## zone 1
  inpf.write("  Zone_1:\n")
  inpf.write("    File: mesh_drum_1_" + str(pp_tag) + ".msh \n")

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    File: mesh_drum_2_" + str(pp_tag) + ".msh \n")

  ## zone 3 (wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    File: mesh_wall_1_" + str(pp_tag) + ".msh \n")

  ## zone 4 (wall)
  inpf.write("  Zone_4:\n")
  inpf.write("    File: mesh_wall_2_" + str(pp_tag) + ".msh \n")

  #
  # Contact info
  #
  inpf.write("Contact:\n")

  ## 11
  inpf.write("  Zone_11:\n")
  # inpf.write("    Contact_Radius: %4.6e\n" % (R_contact))
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_11))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 12
  inpf.write("  Zone_12:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_12))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 13
  inpf.write("  Zone_13:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_13))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 14
  inpf.write("  Zone_14:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_14))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 22
  inpf.write("  Zone_22:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_22))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 23
  inpf.write("  Zone_23:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_23))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 24
  inpf.write("  Zone_24:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_24))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 33
  inpf.write("  Zone_33:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_33))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 34
  inpf.write("  Zone_34:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_34))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 44
  inpf.write("  Zone_44:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_44))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  # Neighbor info
  inpf.write("Neighbor:\n")
  inpf.write("  Update_Criteria: %s\n" % (neigh_search_criteria))
  inpf.write("  Search_Factor: %4.e\n" % (neigh_search_factor))
  inpf.write("  Search_Interval: %d\n" % (neigh_search_interval))

  #
  # Material info
  #
  inpf.write("Material:\n")

  ## zone 1
  inpf.write("  Zone_1:\n")
  inpf.write("    Type: PDState\n")
  inpf.write("    Horizon: %4.6e\n" % (horizon))
  inpf.write("    Density: %4.6e\n" % (rho1))
  inpf.write("    Compute_From_Classical: true\n")
  inpf.write("    K: %4.6e\n" % (K1))
  inpf.write("    G: %4.6e\n" % (G1))
  inpf.write("    Gc: %4.6e\n" % (Gc1))
  inpf.write("    Influence_Function:\n")
  inpf.write("      Type: 1\n")

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: PDState\n")
  inpf.write("    Horizon: %4.6e\n" % (horizon))
  inpf.write("    Density: %4.6e\n" % (rho2))
  inpf.write("    Compute_From_Classical: true\n")
  inpf.write("    K: %4.6e\n" % (K2))
  inpf.write("    G: %4.6e\n" % (G2))
  inpf.write("    Gc: %4.6e\n" % (Gc2))
  inpf.write("    Influence_Function:\n")
  inpf.write("      Type: 1\n")

  ## zone 3
  inpf.write("  Zone_3:\n")
  inpf.write("    Type: PDState\n")
  inpf.write("    Horizon: %4.6e\n" % (horizon))
  inpf.write("    Density: %4.6e\n" % (rho3))
  inpf.write("    Compute_From_Classical: true\n")
  inpf.write("    K: %4.6e\n" % (K3))
  inpf.write("    G: %4.6e\n" % (G3))
  inpf.write("    Gc: %4.6e\n" % (Gc3))
  inpf.write("    Influence_Function:\n")
  inpf.write("      Type: 1\n")

  ## zone 4
  inpf.write("  Zone_4:\n")
  inpf.write("    Type: PDState\n")
  inpf.write("    Horizon: %4.6e\n" % (horizon))
  inpf.write("    Density: %4.6e\n" % (rho3))
  inpf.write("    Compute_From_Classical: true\n")
  inpf.write("    K: %4.6e\n" % (K4))
  inpf.write("    G: %4.6e\n" % (G4))
  inpf.write("    Gc: %4.6e\n" % (Gc4))
  inpf.write("    Influence_Function:\n")
  inpf.write("      Type: 1\n")

  #
  # Force
  #
  if gravity_active == True:
    inpf.write("Force_BC:\n")
    inpf.write("  Gravity: " + print_dbl_list(gravity))

  #
  # IC
  #
  inpf.write("IC:\n")
  inpf.write("  Constant_Velocity:\n")
  inpf.write("    Velocity_Vector: " + print_dbl_list(free_fall_vel))
  inpf.write("    Particle_List: [1]\n")

  #
  # Displacement
  #
  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 1\n")

  inpf.write("  Set_1:\n")
  inpf.write("    Particle_List: [2, 3]\n")
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Zero_Displacement: true\n")

  #
  # Output info
  #
  inpf.write("Output:\n")
  inpf.write("  Path: ../out/\n")
  inpf.write("  Tags:\n")
  inpf.write("    - Displacement\n")
  inpf.write("    - Velocity\n")
  inpf.write("    - Force\n")
  inpf.write("    - Force_Density\n")
  inpf.write("    - Damage_Z\n")
  inpf.write("    - Damage\n")
  inpf.write("    - Nodal_Volume\n")
  inpf.write("    - Zone_ID\n")
  inpf.write("    - Particle_ID\n")
  inpf.write("    - Fixity\n")
  inpf.write("    - Force_Fixity\n")
  inpf.write("    - Contact_Data\n")
  inpf.write("    - No_Fail_Node\n")
  inpf.write("    - Boundary_Node_Flag\n")
  # inpf.write("    - Strain_Stress\n")
  inpf.write("  Output_Interval: %d\n" % (dt_out_n))
  inpf.write("  Compress_Type: zlib\n")
  inpf.write("  Perform_FE_Out: false\n")
  if perform_out:
    inpf.write("  Perform_Out: true\n")
  else:   
    inpf.write("  Perform_Out: false\n")
  inpf.write("  Test_Output_Interval: %d\n" % (dt_out_n))
  
  inpf.write("  Debug: 2\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))

  # close file
  inpf.close()


  # generate particle locations
  particle_locations_orient(inp_dir, pp_tag, R1, R2, particle_dist - free_fall_dist, wall1_five_param_format, wall2_five_param_format)

  # generate particle .geo file (large)
  p_mesh_fname = ['mesh_drum_1', 'mesh_drum_2', 'mesh_wall_1', 'mesh_wall_2']
  generate_particle_gmsh_input(inp_dir, p_mesh_fname[0], center, R1, drum1_neck_width, mesh_size, pp_tag)
  generate_particle_gmsh_input(inp_dir, p_mesh_fname[1], center, R2, drum2_neck_width, mesh_size, pp_tag)
  generate_wall_gmsh_input(inp_dir, p_mesh_fname[2], wall1_gmsh_input_rect, 0.5*mesh_size, pp_tag)
  generate_wall_gmsh_input(inp_dir, p_mesh_fname[3], wall2_gmsh_input_rect, 0.5*mesh_size, pp_tag)

  os.system("mkdir -p ../out")

  for s in p_mesh_fname:
    print('\n\n')
    print(s)
    print("gmsh {}_{}.geo -2".format(s, pp_tag))
    print('\n\n')
    os.system("gmsh {}_{}.geo -2".format(s, pp_tag))
    # os.system("gmsh {}_{}.geo -2 -o {}_{}.vtk".format(s, pp_tag, s, pp_tag))


##-------------------------------------------------------##
##-------------------------------------------------------##
inp_dir = './'
pp_tag = 0
if len(sys.argv) > 1:
  pp_tag = int(sys.argv[1])

create_input_file(inp_dir, pp_tag)
