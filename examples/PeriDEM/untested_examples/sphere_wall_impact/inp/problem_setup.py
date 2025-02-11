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


def particle_locations(inp_dir, pp_tag, R1, wall_particle_dist, wall1_cuboid):
  """Generate particle location data"""

  sim_particles = []
  sim_particles.append([0., 0., R1+wall_particle_dist, 0., R1, 0.])
  sim_particles.append([1., 0.5*(wall1_cuboid[0] + wall1_cuboid[3]), 0.5*(wall1_cuboid[1] + wall1_cuboid[4]), 0.5*(wall1_cuboid[2] + wall1_cuboid[5]), wall1_cuboid[3] - wall1_cuboid[0], 0.])

  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  inpf.write("i, x, y, z, r, o\n")
  for p in sim_particles:
    inpf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4], p[5]))

  inpf.close()


def generate_sphere_particle_gmsh_input(inp_dir, filename, center, radius, mesh_size, pp_tag):

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
  geof.write("Point(1) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy, sim_Cz, sim_h))
  geof.write("Point(2) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx + sim_radius, sim_Cy, sim_Cz, sim_h))
  geof.write("Point(3) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx - sim_radius, sim_Cy, sim_Cz, sim_h))
  geof.write("Point(4) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy + sim_radius, sim_Cz, sim_h))
  geof.write("Point(5) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy - sim_radius, sim_Cz, sim_h))
  geof.write("Point(6) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy, sim_Cz + sim_radius, sim_h))
  geof.write("Point(7) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_Cx, sim_Cy, sim_Cz - sim_radius, sim_h))

  #
  # circlular arc
  #
  geof.write("Circle(1) = {2, 1, 4};\n")
  geof.write("Circle(2) = {4, 1, 3};\n")
  geof.write("Circle(3) = {3, 1, 5};\n")
  geof.write("Circle(4) = {5, 1, 2};\n")

  geof.write("Circle(5) = {2, 1, 6};\n")
  geof.write("Circle(6) = {6, 1, 3};\n")
  geof.write("Circle(7) = {3, 1, 7};\n")
  geof.write("Circle(8) = {7, 1, 2};\n")

  geof.write("Circle(9) = {4, 1, 6};\n")
  geof.write("Circle(10) = {6, 1, 5};\n")
  geof.write("Circle(11) = {5, 1, 7};\n")
  geof.write("Circle(12) = {7, 1, 4};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(14) = {2, 7, 12};\n")
  geof.write("Ruled Surface(14) = {14};\n")

  geof.write("Line Loop(16) = {2, -6, -9};\n")
  geof.write("Ruled Surface(16) = {16};\n")
  geof.write("Line Loop(18) = {3, -10, 6};\n")
  geof.write("Ruled Surface(18) = {18};\n")
  geof.write("Line Loop(20) = {3, 11, -7};\n")
  geof.write("Ruled Surface(20) = {20};\n")
  geof.write("Line Loop(22) = {4, -8, -11};\n")
  geof.write("Ruled Surface(22) = {22};\n")
  geof.write("Line Loop(24) = {4, 5, 10};\n")
  geof.write("Ruled Surface(24) = {24};\n")
  geof.write("Line Loop(26) = {1, 9, -5};\n")
  geof.write("Ruled Surface(26) = {26};\n")
  geof.write("Line Loop(28) = {1, -12, 8};\n")
  geof.write("Ruled Surface(28) = {28};\n")

  geof.write("Surface Loop(30) = {14, 16, 18, 20, 22, 24, 26, 28};\n")
  # tag = '"' + "a" + '"'
  # geof.write("Physical Surface(%s) = {30};\n" % (tag))
  geof.write("Volume(30) = {30};\n")
  geof.write("Physical Volume(1) = {30};\n")

  # add center point to plane surface
  geof.write("Point{1} In Volume {1};")

  # close file
  geof.close()

def generate_cuboid_wall_gmsh_input(inp_dir, filename, cuboid, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # center and radius
  sim_Cx = 0.5*(cuboid[0] + cuboid[3])
  sim_Cy = 0.5*(cuboid[1] + cuboid[4])
  sim_Cz = 0.5*(cuboid[2] + cuboid[5])
  sim_Lx, sim_Ly, sim_Lz = cuboid[3] - cuboid[0], cuboid[4] - cuboid[1], cuboid[5] - cuboid[2]

  sim_corner_p1 = [cuboid[0], cuboid[1], cuboid[2]]
  sim_corner_p2 = [cuboid[3], cuboid[1], cuboid[2]]

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
  geof.write("Point(1) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_corner_p1[0], sim_corner_p1[1], sim_corner_p1[2], sim_h))
  geof.write("Point(2) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_corner_p1[0]+sim_Lx, sim_corner_p1[1], sim_corner_p1[2], sim_h))
  geof.write("Point(3) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_corner_p1[0]+sim_Lx, sim_corner_p1[1]+sim_Ly, sim_corner_p1[2], sim_h))
  geof.write("Point(4) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (sim_corner_p1[0], sim_corner_p1[1]+sim_Ly, sim_corner_p1[2], sim_h))


  #
  # Line
  #
  geof.write("Line(1) = {4,3};\n")
  geof.write("Line(2) = {3,2};\n")
  geof.write("Line(3) = {2,1};\n")
  geof.write("Line(4) = {1,4};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(5) = {2,3,4,1};\n")
  geof.write("Plane Surface(6) = {5};\n")
  geof.write("Extrude {0, 0.0, %4.6e}{ Surface {6}; }\n" % (sim_Lz))

  # close file
  geof.close()


def create_input_file(inp_dir, pp_tag):
  """Generates input file for two-particle test"""

  sim_inp_dir = str(inp_dir)

  ## R1 - bottom    R2 - top
  center = [0., 0., 0.]
  R1 = 0.001
  mesh_size = R1 / 5.

  horizon = 3. * mesh_size
  wall_particle_dist = 0.2*R1

  # corner points
  wall1_cuboid = [-3*R1, -2*horizon, -2*R1, 3*R1, 0, 2*R1]
  wall1_cuboid_ref = [-3*R1, -horizon, -2*R1, 3*R1, horizon, 2*R1]

  ## time 
  final_time = 0.0008
  num_steps = 100000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 100
  dt_out_n = num_steps / num_outputs
  perform_out = True

  ## material
  poisson1 = 0.25
  rho1 = 1200.
  K1 = 2.16e+9
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
  damping_active = False
  friction_active = False 
  beta_n_factor = 100.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## assign free fall velocity to second particle
  free_fall_vel = [0., -2., 0.]

  ## neighbor search details
  neigh_search_factor = 10.
  neigh_search_interval = 40
  neigh_search_criteria = "simple_all"


  ### ---------------------------------------------------------------- ###
  # generate YAML file
  ### ---------------------------------------------------------------- ###
  # print('\nGenerating imput file\n')
  inpf = open(sim_inp_dir + 'input_' + str(pp_tag) + '.yaml','w')
  inpf.write("Model:\n")
  inpf.write("  Dimension: 3\n")
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
  inpf.write("    Type: cuboid\n")
  contain_params = [-2*R1, -2*horizon, -2*R1, 2*R1, 3*R1, 2*R1]
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
  inpf.write("  Test_Name: sphere_wall_impact\n")
  inpf.write("  Zone_1:\n")
  inpf.write("    Type: sphere\n")
  p1_geom = [R1, center[0], center[1], center[2]]
  inpf.write("    Parameters: " + print_dbl_list(p1_geom)) 
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: cuboid\n")
  inpf.write("    Parameters: " + print_dbl_list(wall1_cuboid_ref))

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
  inpf.write("    File: mesh_sphere_" + str(pp_tag) + ".msh \n")

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    File: mesh_cuboid_" + str(pp_tag) + ".msh \n")

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
  inpf.write("    Particle_List: [0]\n")

  #
  # Displacement
  #
  # clamp left and right parts
  wall1_left = [-2*R1, -2*horizon, -2*R1, -2*R1+horizon, 0., 2*R1]
  wall1_right = [2*R1-horizon, -2*horizon, -2*R1, 2*R1, 0., 2*R1]

  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 2\n")

  inpf.write("  Set_1:\n")
  inpf.write("    Region:\n")
  inpf.write("      Geometry:\n")
  inpf.write("        Type: cuboid\n")
  inpf.write("        Parameters: " + print_dbl_list(wall1_left))
  inpf.write("    Particle_List: [1]\n")
  inpf.write("    Direction: [1,2,3]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - 0.0\n")
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("    Zero_Displacement: true\n")

  inpf.write("  Set_2:\n")
  inpf.write("    Region:\n")
  inpf.write("      Geometry:\n")
  inpf.write("        Type: cuboid\n")
  inpf.write("        Parameters: " + print_dbl_list(wall1_right))
  inpf.write("    Particle_List: [1]\n")
  inpf.write("    Direction: [1,2,3]\n")
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
  inpf.write("  Perform_FE_Out: true\n")
  if perform_out:
    inpf.write("  Perform_Out: true\n")
  else:   
    inpf.write("  Perform_Out: false\n")
  inpf.write("  Test_Output_Interval: %d\n" % (dt_out_n))
  
  inpf.write("  Debug: 3\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))

  # close file
  inpf.close()


  # generate particle locations
  particle_locations(inp_dir, pp_tag, R1, wall_particle_dist, wall1_cuboid)

  p_mesh_fname = ['mesh_sphere', 'mesh_cuboid']
  # generate particle .geo file (large)
  generate_sphere_particle_gmsh_input(inp_dir, 'mesh_sphere', center, R1, mesh_size, pp_tag)
  generate_cuboid_wall_gmsh_input(inp_dir, 'mesh_cuboid', wall1_cuboid_ref, mesh_size, pp_tag)

  os.system("mkdir -p ../out")

  for s in p_mesh_fname:
    print('\n\n')
    print(s)
    print("gmsh {}_{}.geo -3".format(s, pp_tag))
    print('\n\n')
    os.system("gmsh {}_{}.geo -3".format(s, pp_tag))
    os.system("gmsh {}_{}.geo -3 -o {}_{}.vtk".format(s, pp_tag, s, pp_tag))


##-------------------------------------------------------##
##-------------------------------------------------------##
inp_dir = './'
pp_tag = 0
if len(sys.argv) > 1:
  pp_tag = int(sys.argv[1])

create_input_file(inp_dir, pp_tag)
