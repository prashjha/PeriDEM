import os
import sys
import numpy as np

#sys.path.append('<Path to PeriDEM source>/tools/python_utils')
from gmsh_particles import *
from util import *

def create_input_file(inp_dir, pp_tag):
  """Generates input file for two-particle test"""

  sim_inp_dir = str(inp_dir)

  R1 = 0.001
  mesh_size = R1 / 8.
  horizon = 2.2 * mesh_size
  particle_dist = 0.9*mesh_size #horizon # surface to surface distance 

  ## particle 1 rectangle
  Lx, Ly = 4*R1, 3*mesh_size
  rect_center = [R1, 0., 0.]

  ## particle 2 circle
  cir_center = [R1, rect_center[1] + 0.5*Ly + particle_dist + R1, 0.]

  ## assign free fall velocity to second particle
  high_impact = False
  free_fall_vel = [0., -0.1, 0.]
  if high_impact:
    free_fall_vel = [0., -4., 0.]  # high impact velocity

  ## time 
  final_time = 0.01
  num_steps = 50000
  if high_impact:
    final_time = 0.0002
    num_steps = 10000
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
  damping_active = False
  friction_active = False 
  beta_n_factor = 10.
  Kn_factor = 0.1

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## neighbor search details
  neigh_search_factor = 2.
  neigh_search_interval = 1
  neigh_search_criteria = "simple_all"

  ### ---------------------------------------------------------------- ###
  # generate mesh and particle location data
  ### ---------------------------------------------------------------- ###
  plocf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  plocf.write("i, x, y, z, r, o\n")
  plocf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (0, rect_center[0], rect_center[1], rect_center[2], Lx, 0.))
  plocf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (1, cir_center[0], cir_center[1], cir_center[2], R1, 0.))
  plocf.close()

  zones_mesh_fnames = ["mesh_rect_1", "mesh_cir_2"]

  # generate mesh for particle 1
  rectangle_mesh_symmetric(xc = [0., 0., 0.], Lx = Lx, Ly = Ly, h = mesh_size, filename = zones_mesh_fnames[0] + "_" + str(pp_tag), vtk_out = True, symmetric_mesh = True)

  # generate mesh for particle 2
  circle_mesh_symmetric(xc = [0., 0., 0.], r = R1, h = mesh_size, filename = zones_mesh_fnames[1] + "_" + str(pp_tag), vtk_out = True, symmetric_mesh = True)

  os.system("mkdir -p ../out")

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
  contain_params = [rect_center[0] - 0.5*Lx, rect_center[1] - 0.5*Ly, 0., rect_center[0] + 0.5*Lx, rect_center[1] + 0.5*Ly + particle_dist + 2*R1, 0.]
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
  inpf.write("  Zone_1:\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list([Lx, Ly, rect_center[0], rect_center[1], rect_center[2]])) 
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: circle\n")
  p2_geom = [R1, cir_center[0], cir_center[1], cir_center[2]]
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

  for i in range(len(zones_mesh_fnames)):
    inpf.write("  Zone_%d:\n" % (i+1))
    inpf.write("    File: %s\n" % (zones_mesh_fnames[i] + "_" + str(pp_tag) + ".msh"))

  # Contact info
  inpf.write("Contact:\n")

  ## 11
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "11", Kn_11)

  ## copy from 11
  copy_contact_zone(inpf, [12, 22], [1, 1])

  # Neighbor info
  inpf.write("Neighbor:\n")
  inpf.write("  Update_Criteria: %s\n" % (neigh_search_criteria))
  inpf.write("  Search_Factor: %4.e\n" % (neigh_search_factor))
  inpf.write("  Search_Interval: %d\n" % (neigh_search_interval))

 # Material info
  inpf.write("Material:\n")

  ## zone 1
  write_material_zone_part(inpf, "1", horizon, rho1, K1, G1, Gc1)

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    Copy_Material_Data: 1\n")

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
  
  inpf.write("  Debug: 3\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))

  # close file
  inpf.close()

##-------------------------------------------------------##
##-------------------------------------------------------##
if __name__ == "__main__":
  inp_dir = './'
  pp_tag = 0
  if len(sys.argv) > 1:
    pp_tag = int(sys.argv[1])

  create_input_file(inp_dir, pp_tag)