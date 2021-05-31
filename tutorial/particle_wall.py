import os
import numpy as np
import sys
from pathlib import Path

sys.path.insert(0, '/home/prashant/work/peridem_works/PeriDEM/tools/script/python_utils/')

# import util functions
from util import *


def particle_locations(filename, radius, centers, pp_tag = None):
    """
    Creates .csv file containing the center, radius, orientation, and zone id of each particle in two-particle setup

    Parameters
    ----------
    filename : str
      Filename of .csv file to be created
    radius: list
      List containing radius of a particle
    centers: list
      List containing the centers of a particle
    pp_tag: str, optional
      Postfix .geo file with this tag
    """
    pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
    inpf = open(filename + pp_tag_str + '.csv','w')
    inpf.write('i, x, y, z, r, o\n')
    for i in range(len(radius)):
        inpf.write('%d, ' % i)
        inpf.write(print_list(centers[i], '%Lf'))
        inpf.write(', %Lf' % radius[i])
        o = 0. if i == 0 else np.pi * 0.5
        inpf.write(', %Lf\n' % o)
    inpf.close()

if __name__ == "__main__":

  fpath = './particle_wall/'
  Path(fpath).mkdir(parents=True, exist_ok=True)

  ## origin (this center is used to discretize the particle and
  ## later discretization is translated and rotated appropriately)
  center = [0., 0., 0.]

  ## gravity
  g = [0., -10., 0.]

  ## radius of p
  R = 0.001

  ## mesh size (use smaller radius to decide)
  h = R / 8.
      
  ## peridynamics horizon (typically taken as 2 to 4 times the mesh size)
  horizon = 3. * h

  ## wall geometry (corner point coordinates)
  wall_rect = [0., 0., 0., 2.*R, horizon, 0.]

  ## initial distance between particle and wall
  d = 0.001
  # reduce compute time by reducing the distance between particle and wall and assigning top particle velocity
  delta = d - horizon 
  v0 = [0, -np.sqrt(2. * np.abs(g[1]) * delta), 0.]

  ## final time and time step
  T = 0.04
  N_steps = 200000 
  # if dt is large, the simulation could produce garbage results so care is needed
  dt = T / N_steps 
  # steps per output, i.e., simulation will generate output every 50 steps
  N_out = N_steps / 50

  ## material
  # for zone 1 (in this case zone 1 has just one particle)
  nu1 = 0.25
  rho1 = 1200.
  K1 = 2.16e+7
  E1 = get_E(K1, nu1)  # see util.py
  G1 = get_G(E1, nu1)
  Gc1 = 50.

  # for zone 2 (in this case zone 2 consists of wall)
  nu2 = 0.25
  rho2 = 1200.
  K2 = 2.16e+7
  E2 = get_E(K2, nu2)
  G2 = get_G(E2, nu2)
  Gc2 = 50.

  ## contact radius is taken as R_contact_factor * h_{min} where
  ## h_{min} is the minimum mesh size over all meshes (it is computed at the begining 
  ## of simulation)
  ## we only specify the R_contact_factor
  R_contact_factor = 0.95

  ## define Kn parameter for normal contact force (see PeriDEM paper for more details)
  Kn_11 = 18. * get_eff_k(K1, K1) / (np.pi * np.power(horizon, 5))
  Kn_22 = 18. * get_eff_k(K2, K2) / (np.pi * np.power(horizon, 5))
  Kn_12 = 18. * get_eff_k(K1, K2) / (np.pi * np.power(horizon, 5))

  ## define beta_n parameter for normal damping
  beta_n_eps = 0.9
  # factor for damping (Constant C in the PeriDEM paper)
  beta_n_factor = 100.
  damping_active = True 

  ## friction coefficient
  friction_coeff = 0.5
  friction_active = False # friction is not active

  # create particle_locations.csv file 
  centers = []
  centers.append([R, wall_rect[4] + (d - delta) + R, 0.])
  particle_locations(fpath + 'particle_locations', [R], centers)

  ## file below creates .geo file (see util.py)
  generate_circle_gmsh_input(fpath + 'p', center, R, h)
  generate_rectangle_gmsh_input(fpath + 'w', wall_rect, h)

  ## open a input file (we choose pretty obvious name input.yaml)
  inpf = open(fpath + 'input.yaml','w')

  ## provide model specific details
  inpf.write("Model:\n")
  inpf.write("  Dimension: 2\n")
  inpf.write("  Discretization_Type:\n")
  inpf.write("    Spatial: finite_difference\n") # we refer to meshfree discretization by finite_difference
  inpf.write("    Time: central_difference\n")   # you can use either central_difference or velocity_verlet
  inpf.write("  Final_Time: %4.6e\n" % (T))
  inpf.write("  Time_Steps: %d\n" % (N_steps))

  ## not used (I should get rid of this soon)
  inpf.write("Policy:\n")
  inpf.write("  Enable_PostProcessing: true\n")

  ## container info
  ## Container is the region in 2d (or in 3d) where you expect all activites 
  ## to take place. If not sure, you can take a wild guess of bounding box of
  ## this simulation.
  inpf.write("Container:\n")
  inpf.write("  Geometry:\n")
  inpf.write("    Type: rectangle\n")
  # we define container to be rectangle. 
  # in the code, we need to provide 6 element vector to define a rectangle
  # first 3 in the vector correspond to coordinate of the left-bottom corner
  # and remaining correspond to coordinates of the right-top corner
  contain_params = [0., 0., 0., 2.*R, 4.*R, 0.]
  inpf.write("    Parameters: " + print_dbl_list(contain_params))

  ## zone info
  ## we have two zones
  inpf.write("Zone:\n")
  inpf.write("  Zones: 2\n")

  ## zone 1 (particle)
  inpf.write("  Zone_1:\n")
  # specify that this is not a zone containing wall
  inpf.write("    Is_Wall: false\n")

  ## zone 2 (wall)
  inpf.write("  Zone_2:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(wall_rect)) 

  ## particle info
  ## as we said earlier, the properites are specific in a `zone-specific` manner
  ## what I really mean by `zone-specific` will be clear now
  inpf.write("Particle:\n")
  # specify reference particle details for zone 1 particles (in this case zone 1 has just one particle)
  inpf.write("  Zone_1:\n")
  # reference particle is of circular type
  inpf.write("    Type: circle\n")
  # in the code, circle is defined using 4 vector array
  # first element in the vector is radius, and rest are the coordinates of the center
  # recall that we fixed referene particles at center = [0,0,0]
  p_geom = [R, center[0], center[1], center[2]]
  inpf.write("    Parameters: " + print_dbl_list(p_geom))

  ## wall info
  inpf.write("Wall:\n")
  inpf.write("  Zone_2:\n")
  inpf.write("    Type: flexible\n")
  inpf.write("    All_Dofs_Constrained: true\n")
  inpf.write("    Mesh: true\n") 

  ## particle generation
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations.csv\n")
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

  ## Mesh info
  inpf.write("Mesh:\n")

  # zone 1
  inpf.write("  Zone_1:\n")
  inpf.write("    File: p.msh \n")

  # zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    File: w.msh \n")

  ## Contact info
  ## we have to provide contact parameters for pair of zones
  inpf.write("Contact:\n")

  # 11
  # this fixes the contact between two particles in zone 1
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

  # 12
  # this fixes the contact between two particles in zone 1 and zone 2
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

  # 22
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

  ## Neighbor info
  ## at present, this block has no impact in the simulation. We have it for later extensions of the library.
  inpf.write("Neighbor:\n")
  inpf.write("  Update_Criteria: simple_all\n")
  inpf.write("  Search_Factor: 5.0\n")

  ## Material info
  ## material properties have to be specified in a `zone-specific` manner
  inpf.write("Material:\n")

  # zone 1
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

  # zone 2
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

  ## Force
  # `Gravity` force is of body force type and all particles and walls experience this force
  inpf.write("Force_BC:\n")
  inpf.write("  Gravity: " + print_dbl_list(g))

  ## IC
  inpf.write("IC:\n")
  inpf.write("  Constant_Velocity:\n")
  inpf.write("    Velocity_Vector: " + print_dbl_list(v0))
  inpf.write("    Particle_List: [0]\n")

  ## Displacement
  inpf.write("Displacement_BC:\n")
  # it is typical to have many different type of boundary conditions so it is conventient to 
  # consider a sets of boundary condition (force boundary condition is similar)
  # in this case we just have 1 set
  inpf.write("  Sets: 1\n")
  inpf.write("  Set_1:\n")
  # this set is applicable to `0` particle (bottom)
  inpf.write("    Wall_List: [0]\n")
  # displacement components along x and y are impacted
  inpf.write("    Direction: [1,2]\n")
  # displacement is fixed by a function which is constant in time and space. Further, the value of this 
  # constant is 0 (see Parameters block which expects single parameter for constant type function)
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: constant\n")
  inpf.write("      Parameters:\n")
  inpf.write("        - 0.0\n")
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: constant\n")
  # for `0` particle we have fixed all degree of freedoms and the displacement is fixed to 0
  # so we explicitly say that for this particle all defs are zero
  # this extra information allows us to not compute this boundary condition every time step as there is nothing
  # really to compute (displacemnts are always fixed to zero)
  inpf.write("    Zero_Displacement: true\n")

  ## Output info
  ## in this block, we take care of simulation output
  inpf.write("Output:\n")
  # where to write files
  inpf.write("  Path: %s\n" %(fpath))
  inpf.write("  Tags:\n")
  # list the variable that you want to output in the .vtu file
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
  # how frequent the output should take place
  inpf.write("  Output_Interval: %d\n" % (N_out))
  inpf.write("  Compress_Type: zlib\n")
  inpf.write("  Perform_FE_Out: false\n")
  inpf.write("  Perform_Out: true\n")
  # how frequent we dump the test specific outputs
  inpf.write("  Test_Output_Interval: %d\n" % (N_out))
  # what is verbosity level (last time I checked, the maximum is 3 so if you have 3 or above it will
  # produce maximum debug information)
  inpf.write("  Debug: 3\n")
  # assign this simulation a tag
  inpf.write("  Tag_PP: 0\n")

  ## HPX specific block (I would recommend to not touch this block unless you know what your are doing!!)
  inpf.write("HPX:\n")
  inpf.write("  Partitions: 1\n")

  ## close file
  ## damn I feel tired already!!
  inpf.close()
