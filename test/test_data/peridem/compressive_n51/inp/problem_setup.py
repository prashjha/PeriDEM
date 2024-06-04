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


def does_intersect_rect(p, r, particles, padding, rect):

  # check intersection with rectangle
  pr = [p[0] - r, p[1] - r, p[2], p[0] + r, p[1] + r, p[2]]

  if pr[0] < rect[0] + padding or pr[1] < rect[1] + padding or pr[3] > rect[3] - padding or pr[4] > rect[4] - padding:

    # print('circle (xc = {}, r = {:5.3e}) intersects rect = {} with pad = {:5.3e}'.format(p, r, rect, padding))

    return True

  # loop over particles
  # for pi in particles:
  #   dx = [p[0] - pi[1], p[1] - pi[2], p[2] - pi[3]]
  #   rR = r + pi[4] + padding
  #   if np.linalg.norm(dx) < rR: 
  #     return True

  # print('circle (xc = {}, r = {:5.3e}) does not intersects rect = {} with pad = {:5.3e}'.format(p, r, rect, padding))
  return False


def get_E(K, nu):
  return 3. * K * (1. - 2. * nu)

def get_G(E, nu):
  return E / (2. * (1. + nu))


def get_eff_k(k1, k2):
  return 2. * k1 * k2 / (k1 + k2)


def get_max(l):
  l = np.array(l)
  return l.max()


def particle_locations(inp_dir, pp_tag, R1, R2, rect, mesh_size, padding):
  """Generate particle location data"""

  #
  #
  #   Given rectangle, we want to fill it with particles of R1 and R2 radius
  #   in alternate fashion with padding between each particle
  #
  #
  particles = []

  # find how approximate number of rows and cols we can have
  check_r = R1
  if R1 > R2:
    check_r = R2

  rows = int((rect[3] - rect[0])/ check_r)
  cols = int((rect[4] - rect[1])/ check_r)

  rads = [R1, R2]

  counter = 0
  x_old = rect[0]
  y_old = rect[1]
  pad = padding

  cx = 0.
  cy = 0.
  cz = 0.
  r = 0.

  row_rads = []
  row_rads.append(R1)
  row_rads.append(R2)

  for i in range(rows):

    if i > 0:
      y_old = cy + get_max(row_rads)

    # reset
    row_rads = []
    row_rads.append(R1)
    row_rads.append(R2)

    num_p_cols = 0
    j = 0
    while num_p_cols < cols - 1 and j < 100:

      if j == 0:
        x_old = rect[0]

      ptype = counter % 2
      r0 = rads[ptype]

      # perturb radius
      r = r0 + np.random.uniform(-0.1 * r0, 0.1 * r0)
      # r = r0
      row_rads.append(r)

      # random horizontal perturbation by 10% of radius
      rph = np.random.uniform(-0.1 * r0, 0.1 * r0)

      cx0 = x_old + pad + r 
      cx = cx0 + rph
      cy = y_old + pad + r
      cz = rect[2]

      inters = does_intersect_rect([cx, cy, cz], r, particles, pad, rect)
      inters_parts = does_intersect
      
      if inters == False:
        particles.append([float(ptype), cx, cy, cz, r])

        # set x_old
        x_old = cx + r

        counter += 1

        num_p_cols += 1

      j += 1


  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  inpf.write("i, x, y, z, r\n")
  for p in particles:
    inpf.write("%d, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4]))

  inpf.close()

  print('number of particles created = {}'.format(len(particles)))


def generate_particle_gmsh_input(inp_dir, filename, center, radius, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # center and radius
  sim_Cx = center[0]
  sim_Cy = center[1]
  sim_Cz = center[2]
  sim_radius = radius

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
  geof.write("Point(1) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy, sim_Cz, sim_h));
  geof.write("Point(2) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx + sim_radius, sim_Cy, sim_Cz, sim_h))
  geof.write("Point(3) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx - sim_radius, sim_Cy, sim_Cz, sim_h))
  geof.write("Point(4) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy + sim_radius, sim_Cz, sim_h))
  geof.write("Point(5) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy - sim_radius, sim_Cz, sim_h))

  #
  # circlular arc
  #
  geof.write("Circle(1) = {2, 1, 4};\n")
  geof.write("Circle(2) = {4, 1, 3};\n")
  geof.write("Circle(3) = {3, 1, 5};\n")
  geof.write("Circle(4) = {5, 1, 2};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(1) = {2, 3, 4, 1};\n")

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


def generate_rigid_wall_gmsh_input(inp_dir, filename, outer_rect, inner_rect, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # outer rectangle
  sim_Lx_out1 = outer_rect[0]
  sim_Ly_out1 = outer_rect[1]
  sim_Lz_out1 = outer_rect[2]
  sim_Lx_out2 = outer_rect[3]
  sim_Ly_out2 = outer_rect[4]
  sim_Lz_out2 = outer_rect[5]

  # inner rectangle
  sim_Lx_in1 = inner_rect[0]
  sim_Ly_in1 = inner_rect[1]
  sim_Lz_in1 = inner_rect[2]
  sim_Lx_in2 = inner_rect[3]
  sim_Ly_in2 = inner_rect[4]
  sim_Lz_in2 = inner_rect[5]

  # mesh size
  sim_h = mesh_size

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  #
  #                L7                          L3
  #          4  + --- + 8                 7 + --- +  3
  #             |     |                     |     |
  #             |     |                     |     |
  #             |     | L6              L4  |     |
  #       L8    |     |                     |     |
  #             |     |                     |     | L2
  #             |     |         L5          |     | 
  #             |     + ------------------- +     |
  #             |     5                     6     |
  #             |                                 |
  #          1  + ------------------------------- + 2
  #                             L1
  #
  #
  # for point 8 and 7 --> choose y coordinate from outer rectangle and x 
  # coordinate from inner rectangle

  #
  # points
  #
  geof.write("Point(1) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out1, sim_Ly_out1, sim_Lz_out1, sim_h))
  geof.write("Point(2) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out2, sim_Ly_out1, sim_Lz_out1, sim_h))
  geof.write("Point(3) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out2, sim_Ly_out2, sim_Lz_out1, sim_h))
  geof.write("Point(4) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_out1, sim_Ly_out2, sim_Lz_out1, sim_h))

  geof.write("Point(5) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_in1, sim_Ly_in1, sim_Lz_in1, sim_h))
  geof.write("Point(6) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_in2, sim_Ly_in1, sim_Lz_in1, sim_h))
  geof.write("Point(7) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_in2, sim_Ly_out2, sim_Lz_in1, sim_h))
  geof.write("Point(8) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Lx_in1, sim_Ly_out2, sim_Lz_in1, sim_h))

  #
  # lines
  #
  geof.write("Line(1) = {1, 2};\n")
  geof.write("Line(2) = {2, 3};\n")
  geof.write("Line(3) = {3, 7};\n")
  geof.write("Line(4) = {7, 6};\n")
  geof.write("Line(5) = {6, 5};\n")
  geof.write("Line(6) = {5, 8};\n")
  geof.write("Line(7) = {8, 4};\n")
  geof.write("Line(8) = {4, 1};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(1) = {1, 2, 3, 4, 5, 6, 7, 8};\n")

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


def generate_moving_wall_gmsh_input(inp_dir, filename, rectangle, mesh_size, pp_tag):

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

  ## domain inside the wall
  dx0, dy0, dz0 = 0., 0., 0.
  dx1, dy1, dz1 = 0.014, 0.022, 0.

  ## 
  center = [0., 0., 0.]
  R1 = 0.001
  R2 = 0.001
  mesh_size = R1 / 5.
  if R2 < R1:
    mesh_size = R2 / 5.

  horizon = 3. * mesh_size
  particle_dist = 0.001

  #particle_padding = 2 * mesh_size
  particle_padding = 0.3 * R1

  ## wall

  # rigid wall
  wpd = 1.1 * mesh_size       # particle-wall initial distance
  rwp = horizon + wpd         # padding outside domain (thickness plust wpd)
  rwo_rect = [dx0 - rwp, dy0 - rwp, dz0, dx1 + rwp, dy1 + rwp, dz0]
  rwi_rect = [dx0 - wpd, dy0 - wpd, dz0, dx1 + wpd, dy1 + wpd, dz0]

  # moving wall
  mw_rect = [dx0-wpd, dy1+wpd, dz0, dx1+wpd, dy1+rwp, dz0]

  # maximum distance to stop the simulation
  max_dist_check = 20.*dx1

  ## time 
  final_time = 0.04
  num_steps = 400000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 10
  dt_out_n = num_steps / num_outputs
  test_dt_out_n = dt_out_n / 100
  perform_out = True

  ## material
  poisson1 = 0.25
  rho1 = 1200.
  K1 = 2.16e+7
  E1 = get_E(K1, poisson1)
  G1 = get_G(E1, poisson1)
  Gc1 = 50.

  poisson2 = 0.25
  rho2 = 1200.
  K2 = 2.16e+7
  E2 = get_E(K2, poisson2)
  G2 = get_G(E2, poisson2)
  Gc2 = 50.

  # rigid wall 
  poisson3 = 0.25
  rho3 = 1200.
  K3 = 2.16e+7
  E3 = get_E(K3, poisson3)
  G3 = get_G(E3, poisson3)
  Gc3 = 50.

  # moving wall 
  poisson4 = 0.25
  rho4 = 1200.
  K4 = 2.16e+7
  E4 = get_E(K4, poisson4)
  G4 = get_G(E4, poisson4)
  Gc4 = 50.

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

  beta_n_eps = 0.95
  friction_coeff = 0.5
  damping_active = True
  friction_active = False 
  beta_n_factor = 100.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## moving wall downward velocity
  mw_vy = -0.06


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
  inpf.write("    Parameters: " + print_dbl_list(rwo_rect))

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

  ## zone 3 (rigid wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(rwo_rect)) 

  ## zone 4 (moving wall)
  inpf.write("  Zone_4:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(mw_rect)) 

  #
  # particle info
  #
  inpf.write("Particle:\n")

  inpf.write("  Test_Name: compressive_test\n")
  inpf.write("  Compressive_Test:\n")
  inpf.write("    Wall_Id: 1\n")
  inpf.write("    Wall_Force_Direction: 2\n")

  inpf.write("  Zone_1:\n")
  inpf.write("    Type: circle\n")
  p1_geom = [R1, center[0], center[1], center[2]]
  p2_geom = [R2, center[0], center[1], center[2]]
  inpf.write("    Parameters: " + print_dbl_list(p1_geom)) 
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: circle\n")
  inpf.write("    Parameters: " + print_dbl_list(p2_geom)) 

  #
  # wall info
  #
  inpf.write("Wall:\n")
  inpf.write("  Zone_3:\n")
  inpf.write("    Type: flexible\n")
  inpf.write("    All_Dofs_Constrained: true\n")
  inpf.write("    Mesh: true\n")
  
  inpf.write("  Zone_4:\n")
  inpf.write("    Type: flexible\n")
  inpf.write("    All_Dofs_Constrained: false\n")
  inpf.write("    Mesh: true\n")

  #
  # particle generation
  #
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations_" + str(pp_tag) + ".csv\n")

  #
  # Mesh info
  #
  inpf.write("Mesh:\n")

  ## zone 1
  inpf.write("  Zone_1:\n")
  inpf.write("    File: mesh_cir_1_" + str(pp_tag) + ".msh \n")

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    File: mesh_cir_2_" + str(pp_tag) + ".msh \n")

  ## zone 3 (wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    File: mesh_rigid_wall_" + str(pp_tag) + ".msh \n")

  ## zone 4 (wall)
  inpf.write("  Zone_4:\n")
  inpf.write("    File: mesh_moving_wall_" + str(pp_tag) + ".msh \n")

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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
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
  inpf.write("    Kn_Factor: 1.0\n")
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  ## 4
  inpf.write("  Zone_44:\n")
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn_44))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: 1.0\n")
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

  #
  # Neighbor info
  #
  inpf.write("Neighbor:\n")
  inpf.write("  Update_Criteria: simple_all\n")
  inpf.write("  Search_Factor: 5.0\n")

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
  inpf.write("    Density: %4.6e\n" % (rho4))
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
  # Displacement
  #
  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 2\n")

  inpf.write("  Set_1:\n")
  inpf.write("    Wall_List: [0]\n")
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - 0.0\n")
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("    Zero_Displacement: true\n")

  inpf.write("  Set_2:\n")
  inpf.write("    Wall_List: [1]\n")
  inpf.write("    Direction: [2]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: linear\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - %4.6e\n" % (mw_vy))
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")

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
  inpf.write("    - Contact_Nodes\n")
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
  inpf.write("  Test_Output_Interval: %d\n" % (test_dt_out_n))
  
  inpf.write("  Debug: 1\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))
  inpf.write("  Output_Criteria: \n")
  # inpf.write("    Type: max_particle_dist\n")
  # inpf.write("    Parameters: [%4.6e]\n" % (2. * sim_h))
  inpf.write("    Type: max_node_dist\n")
  inpf.write("    Parameters: [%4.6e]\n" % (max_dist_check))


  inpf.write("HPX:\n")
  inpf.write("  Partitions: 1\n")

  # close file
  inpf.close()


  # generate particle locations
  re_generate_files = False
  if re_generate_files:
    particle_locations(inp_dir, pp_tag, R1, R2, [dx0, dy0, dz0, dx1, dy1, dz1],  mesh_size, particle_padding)

  # generate particle .geo file (large)
  generate_particle_gmsh_input(inp_dir, 'mesh_cir_1', center, R1, mesh_size, pp_tag)
  generate_particle_gmsh_input(inp_dir, 'mesh_cir_2', center, R2, mesh_size, pp_tag)
  generate_moving_wall_gmsh_input(inp_dir, 'mesh_moving_wall', mw_rect, mesh_size, pp_tag)
  generate_rigid_wall_gmsh_input(inp_dir, 'mesh_rigid_wall', rwo_rect, rwi_rect, mesh_size, pp_tag)

##-------------------------------------------------------##
##-------------------------------------------------------##
inp_dir = './'
pp_tag = 0
if len(sys.argv) > 1:
  pp_tag = int(sys.argv[1])

create_input_file(inp_dir, pp_tag)
