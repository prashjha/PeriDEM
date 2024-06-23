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


def particle_locations(inp_dir, pp_tag, R1, R2, offset):
  """Generate particle location data"""

  sim_particles = []
  sim_particles.append([0., R1, R1, 0., R1, 0.])
  sim_particles.append([1., R1, 2. * R1 + R2 + offset, 0., R2, np.pi*0.5])

  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  inpf.write("i, x, y, z, r, o\n")
  for p in sim_particles:
    inpf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4], p[5]))

  inpf.close()


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



def create_input_file(inp_dir, pp_tag):
  """Generates input file for two-particle test"""

  sim_inp_dir = str(inp_dir)

  ## R1 - bottom    R2 - top
  center = [0., 0., 0.]
  R1 = 0.003
  R2 = 0.001
  mesh_size = R1 / 5.
  if R2 < R1:
    mesh_size = R2 / 5.

  horizon = 3. * mesh_size
  particle_dist = 0.001

  ## time 
  final_time = 0.04
  num_steps = 200000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 100
  dt_out_n = num_steps / num_outputs
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
  Kn_12 = 18. * get_eff_k(K1, K2) / (np.pi * np.power(horizon, 5))

  beta_n_eps = 0.9
  friction_coeff = 0.5
  damping_active = True
  friction_active = False 
  beta_n_factor = 100.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## assign free fall velocity to second particle
  free_fall_dist = particle_dist - horizon
  free_fall_vel = [0., 0., 0.]
  free_fall_vel[1] = -np.sqrt(2. * np.abs(gravity[1]) * free_fall_dist) 
  # free_fall_vel[1] = -2.


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

  inpf.write("Policy:\n")
  inpf.write("  Enable_PostProcessing: true\n")

  #
  # container info
  #
  inpf.write("Container:\n")
  inpf.write("  Geometry:\n")
  inpf.write("    Type: rectangle\n")
  contain_params = [0., 0., 0., 2.*R1, 2.*R1 + 2.*R2 + particle_dist, 0.]
  if R2 > R1:
    contain_params[3] = 2.*R2
  inpf.write("    Parameters: " + print_dbl_list(contain_params))

  #
  # zone info
  #
  inpf.write("Zone:\n")
  inpf.write("  Zones: 2\n")

  ## zone 1 (bottom particle)
  inpf.write("  Zone_1:\n")
  inpf.write("    Is_Wall: false\n")
  
  ## zone 2 (top particle)
  inpf.write("  Zone_2:\n")
  inpf.write("    Is_Wall: false\n")

  #
  # particle info
  #
  inpf.write("Particle:\n")
  inpf.write("  Test_Name: two_particle\n")
  inpf.write("  Zone_1:\n")
  inpf.write("    Type: circle\n")
  p1_geom = [R1, center[0], center[1], center[2]]
  inpf.write("    Parameters: " + print_dbl_list(p1_geom)) 
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: circle\n")
  p2_geom = [R2, center[0], center[1], center[2]]
  inpf.write("    Parameters: " + print_dbl_list(p2_geom)) 

  #
  # particle generation
  #
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations_" + str(pp_tag) + ".csv\n")
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

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

  #
  # Contact info
  #
  inpf.write("Contact:\n")

  ## 11
  inpf.write("  Zone_11:\n")
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
  inpf.write("    Particle_List: [0]\n")
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - 0.0\n")
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")
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
  inpf.write("    - Contact_Nodes\n")
  inpf.write("    - No_Fail_Node\n")
  inpf.write("    - Boundary_Node_Flag\n")
  inpf.write("    - Theta\n")
  inpf.write("  Output_Interval: %d\n" % (dt_out_n))
  inpf.write("  Compress_Type: zlib\n")
  inpf.write("  Perform_FE_Out: false\n")
  if perform_out:
    inpf.write("  Perform_Out: true\n")
  else:   
    inpf.write("  Perform_Out: false\n")
  inpf.write("  Test_Output_Interval: %d\n" % (dt_out_n))
  
  inpf.write("  Debug: 1\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))


  inpf.write("HPX:\n")
  inpf.write("  Partitions: 1\n")

  # close file
  inpf.close()


  # generate particle locations
  particle_locations(inp_dir, pp_tag, R1, R2, particle_dist - free_fall_dist)

  # generate particle .geo file (large)
  generate_particle_gmsh_input(inp_dir, 'mesh_cir_1', center, R1, mesh_size, pp_tag)
  generate_particle_gmsh_input(inp_dir, 'mesh_cir_2', center, R2, mesh_size, pp_tag)


##-------------------------------------------------------##
##-------------------------------------------------------##
inp_dir = './'
pp_tag = 0
if len(sys.argv) > 1:
  pp_tag = int(sys.argv[1])

create_input_file(inp_dir, pp_tag)
