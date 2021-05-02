import os
import numpy as np
import sys
sys.path.insert(0, '../')
from util import *

def two_particle_locations(filename, radius, offset, pp_tag = None):
  """
  Creates .csv file containing the center, radius, orientation, and zone id of each particle in two-particle setup

  Parameters
  ----------
  filename : str
      Filename of .csv file to be created
  radius: list
      List containing radius of two particles
  offset: float
      Distance between two particles (center to center distance will be offset + R1 + R2 where R1,R2 are radii)
  pp_tag: str, optional
      Postfix .geo file with this tag
  """
  R1, R2 = radius[0], radius[1]
  particles = []
  # entries: i, x, y, z, r, o
  # i - zone (group) id, r - radius, o - orientation
  particles.append([0., R1, R1, 0., R1, 0.5*np.pi])
  particles.append([1., R1+1.*R1*np.sin(np.pi/3.), 2. * R1 + R2 + offset, 0., R2, 0.5*np.pi])

  pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
  inpf = open(filename + pp_tag_str + '.csv','w')
  inpf.write('i, x, y, z, r, o\n')
  for p in particles:
    inpf.write('%d, ' % int(p[0]) + print_list(p[1:], '%Lf') + '\n')

  inpf.close()

if __name__=="__main__":
  """
  Generates input files for two-particle setup. Input files created are
    - input.yaml
    - particle_1.geo
    - particle_2.geo
    - particle_locations.csv

  Gmsh should be used to create particle_1.msh and particle_2.msh from two .geo files. After this the simulation can be set to run using

  PeriDEM -i input.yaml --hpx:threads=2
  """

  inp_dir = './'
  """ when we have multiple simulations, we use pp_tag to assign a tag to individual files. If set to valid string or number, all the input files created will have postfix given by this variable.
  """
  pp_tag = None 
  pp_tag_str =  '_{}'.format(str(pp_tag)) if pp_tag is not None else ''

  ### ---------------------------------------------------------------- ###
  # Simulation parameters
  ### ---------------------------------------------------------------- ###
  ## 1 - bottom    2 - top    3 - wall
  center = [0., 0., 0.]
  R1 = 0.001
  R2 = 0.0015
  mesh_size = R1 / 8.
  if R2 < R1:
    mesh_size = R2 / 8.

  # peridynamic horizon
  horizon = 3. * mesh_size

  """ distance between top and bottom particles (boundary to boundary, for noncircular particle, one has to check desired value of this variable)
  """
  particle_dist = 0.0007

  ## wall

  # distance between bottom particle and wall
  wall_bottom_particle_dist = 1.5 * mesh_size
  # specify wall geometry (left-bottom and right-top corner points)
  wall_rect = [-R1, -horizon - wall_bottom_particle_dist, 0., 3.*R1, -wall_bottom_particle_dist, 0.]
  if R2 > R1:
    wall_rect[0] = R1-2.*R2
    wall_rect[3] = R1+2.*R2

  ## time 
  final_time = 0.005
  num_steps = 800000
  num_outputs = 50
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
  K2 = 2e+9
  E2 = get_E(K2, poisson2)
  G2 = get_G(E2, poisson2)
  Gc2 = 500.

  # wall 
  poisson3 = 0.25
  rho3 = 1200.
  K3 = 2e+9
  E3 = get_E(K3, poisson3)
  G3 = get_G(E3, poisson3)
  Gc3 = 500.

  ## contact
  R_contact_factor = 0.95

  # Kn parameter for normal contact (compue from bulk modulus)
  Kn_11 = 18. * get_eff_k(K1, K1) / (np.pi * np.power(horizon, 5))
  Kn_22 = 18. * get_eff_k(K2, K2) / (np.pi * np.power(horizon, 5))
  Kn_33 = 18. * get_eff_k(K3, K3) / (np.pi * np.power(horizon, 5))
  Kn_12 = 18. * get_eff_k(K1, K2) / (np.pi * np.power(horizon, 5))
  Kn_13 = 18. * get_eff_k(K1, K3) / (np.pi * np.power(horizon, 5))
  Kn_23 = 18. * get_eff_k(K2, K3) / (np.pi * np.power(horizon, 5))

  # beta_n_eps parameter for normal damping contact
  beta_n_eps = 0.95

  # friction coefficient 
  friction_coeff = 0.5

  # on/off damping and frication
  damping_active = True
  friction_active = False 

  """ constant C in normal damping force (we use center-center damping model in the PeriDEM paper). 
  """
  beta_n_factor = 100.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## assign velocity to second particle
  impact_vel = [0., 0., 0.]
  impact_vel[1] = -5.

  ### ---------------------------------------------------------------- ###
  # generate YAML file
  ### ---------------------------------------------------------------- ###
  inpf = open(inp_dir + 'input' + pp_tag_str + '.yaml','w')

  #
  # Model information
  # Specify general simulation information
  #
  inpf.write("Model:\n")
  inpf.write("  Dimension: 2\n")
  inpf.write("  Discretization_Type:\n")
  inpf.write("    Spatial: finite_difference\n")
  inpf.write("    Time: central_difference\n")
  inpf.write("  Final_Time: %4.6e\n" % (final_time))
  inpf.write("  Time_Steps: %d\n" % (num_steps))

  #
  # Container info
  # Container is the region (box) in 2d plane where you expect all activites 
  # to take place. If not sure, you can take a wild guess of bounding box of
  # this simulation.
  #
  inpf.write("Container:\n")
  inpf.write("  Geometry:\n")
  inpf.write("    Type: rectangle\n")
  # take a wild guess if not sure (at present this has no impact in the simulation)
  fact_R1 = 5*R1
  contain_params = [-fact_R1, -fact_R1, 0., fact_R1, fact_R1, 0.]
  inpf.write("    Parameters: " + print_dbl_list(contain_params))

  #
  # zone info
  #
  """ we group particles and walls using zone id. In this example, we consider three zones, one each for two particles and one for wall. There could be many reasons for grouping the particles and walls. Some of them are:
    - functional: put particles in one group and wall in other
    - material: group particles by the material properties. For example, in N particles, if say m particles have material property of glass and remaining N-m have property of steel, we will create two zones; in one zone, we will have m particles made of glass and second zone, we will have remaining particles
    - contact: For some reason, we may want to apply contacts between certain groups of particles differently compared to other groups. 
    - mesh: Suppose we have N circular particle and we may want to use coarse mesh for m particles (so they will have a reference circle with coarse mesh) and fine mesh remaining particles. In such a situation, we will group m particles with coarse mesh in first group and other in second group.
    - we may for the sake of it want to have as man zones as particles, i.e., each particle will have its associated zone
  """
  inpf.write("Zone:\n")
  inpf.write("  Zones: 3\n")

  ## zone 1 (bottom particle)
  inpf.write("  Zone_1:\n")
  inpf.write("    Is_Wall: false\n")

  ## zone 2 (top particle)
  inpf.write("  Zone_2:\n")
  inpf.write("    Is_Wall: false\n")

  ## zone 3 (wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(wall_rect)) 

  #
  # particle info
  #
  inpf.write("Particle:\n")
  # if there is postprocessing function implemented for particle test, you can specify the name of the test, and after every time step, the associated post-processing function will be called in the PeriDEM. If want, you can comment out this line.
  inpf.write("  Test_Name: two_particle_wall\n")
  # describe particle in zone 1 (bottom particle)
  inpf.write("  Zone_1:\n")
  # we gave the concave particle (see get_ref_drum_points() in util.py for geometry) name 'drum2d'
  inpf.write("    Type: drum2d\n")
  # need to specify details of the reference drum particle
  # the required details vary from particle type to particle type. Compare this with tests where we use circular/hexagon particles.
  drum_axis = [1., 0., 0.]
  drum1_neck_width = 0.5*R1
  p1_geom = [R1, drum1_neck_width, center[0], center[1], center[2], drum_axis[0], drum_axis[1], drum_axis[2]]
  inpf.write("    Parameters: " + print_dbl_list(p1_geom)) 
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: drum2d\n")
  
  drum2_neck_width = 0.5*R2
  p2_geom = [R2, drum2_neck_width, center[0], center[1], center[2], drum_axis[0], drum_axis[1], drum_axis[2]]
  inpf.write("    Parameters: " + print_dbl_list(p2_geom)) 

  #
  # wall info
  #
  inpf.write("Wall:\n")
  inpf.write("  Zone_3:\n")
  inpf.write("    Type: flexible\n")
  # below means that we will not compute forces on this wall as it has no consequence on the simulation
  inpf.write("    All_Dofs_Constrained: true\n")
  # below means that we will use mesh for this wall. In future, we may implement a model of wall that does not require meshing and in that case we may set Mesh: false.
  inpf.write("    Mesh: true\n")

  #
  # particle generation
  #
  inpf.write("Particle_Generation:\n")
  # this says that read center, radius, and orientation of particles from .csv file
  inpf.write("  From_File: particle_locations" + pp_tag_str + ".csv\n")
  # specify data type to be expected in .csv file above.
  # below means .csv file contains 'locations', 'radius', 'orientation' information in the .csv file
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

  

  #
  # Mesh info
  #
  inpf.write("Mesh:\n")

  ## zone 1
  inpf.write("  Zone_1:\n")
  inpf.write("    File: mesh_particle_1" + pp_tag_str + ".msh \n")

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    File: mesh_particle_2" + pp_tag_str + ".msh \n")

  ## zone 3 (wall)
  inpf.write("  Zone_3:\n")
  inpf.write("    File: mesh_wall" + pp_tag_str + ".msh \n")

  #
  # Contact info
  #
  """ Contact is specified for pair of zones. I.e. contact between particles in zone i and j, does not depend on the individual particles of the zone i and j. So in scenario where we want to have different contact between different groups of particles, we will have to put the particles in different zones, and then prescribe the contact parameters for these zones.

  In this example, we have 3 zones, so we need to provide <1,1>, <1,2>, <1,3>, <2,2>, <2,3>, <3,3> number of contact parameters. <i,j> is equivalent to <j,i>.
  """
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

  #
  # Neighbor info
  #
  # NOTE: At present, these variables are not uses so it is fine to get rid of this from the input file.
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
  # this is peridynamic influence function. See 
  # https://github.com/prashjha/PeriDEM/blob/e38d2c801c95555fa98f4be45a15276d3532a871/src/material/mparticle/material.h#L250
  # to know what type of influence functions are currently used.
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

  #
  # Force
  #
  if gravity_active == True:
    # gravity is added as force boundary condition (body force)
    inpf.write("Force_BC:\n")
    inpf.write("  Gravity: " + print_dbl_list(gravity))

  #
  # IC
  #
  inpf.write("IC:\n")
  inpf.write("  Constant_Velocity:\n")
  inpf.write("    Velocity_Vector: " + print_dbl_list(impact_vel))
  inpf.write("    Particle_List: [1]\n")

  #
  # Displacement
  #
  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 1\n")

  inpf.write("  Set_1:\n")
  # appy only to first (and only) wall
  inpf.write("    Wall_List: [0]\n")
  # x and y coordinates are affected
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - 0.0\n")
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")
  # this means that we don't actually have to compute the boundary condition
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
  # code will output every dt_out_n steps
  inpf.write("  Output_Interval: %d\n" % (dt_out_n))
  inpf.write("  Compress_Type: zlib\n")
  # specify if we want to retain the unstructured mesh in .vtu files
  inpf.write("  Perform_FE_Out: false\n")
  if perform_out:
    inpf.write("  Perform_Out: true\n")
  else:   
    inpf.write("  Perform_Out: false\n")
  # specify how often test output will be performed
  # this may also affect the frequency of information outputs
  inpf.write("  Test_Output_Interval: %d\n" % (dt_out_n))
  
  # specify debug level (if want all of the information, choose some large number. I think currently, choosing 3 will result in maximum info output)
  inpf.write("  Debug: 3\n")
  # specify if simulation has some tag (used for postfix certain output files)
  if pp_tag is not None:
    inpf.write("  Tag_PP: %s\n" %(str(pp_tag)))
  else:
    inpf.write("  Tag_PP: 0\n")

  inpf.write("HPX:\n")
  inpf.write("  Partitions: 1\n")

  # close file
  inpf.close()


  # generate particle locations
  two_particle_locations("particle_locations", [R1, R2], particle_dist)

  # generate particle .geo file 
  generate_drum_gmsh_input("mesh_particle_1", center, R1, drum1_neck_width, mesh_size, pp_tag)
  generate_drum_gmsh_input("mesh_particle_2", center, R2, drum2_neck_width, mesh_size, pp_tag)
  generate_rectangle_gmsh_input("mesh_wall", wall_rect, mesh_size, pp_tag)
