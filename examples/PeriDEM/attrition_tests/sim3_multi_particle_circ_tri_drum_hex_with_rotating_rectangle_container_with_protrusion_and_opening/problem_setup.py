import os
import numpy as np
# import csv
import sys
# import pyvista as pv

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

def get_tri_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win):
    
    Cx = center[0]
    Cy = center[1]
    Cz = center[2]

    # triangle protrusion on upper-right corner
    p20 = [Cx + 0.5*Lin - 0.15*Lin, Cy + 0.5*Win - 0.3*Win, Cz]
    p19 = [Cx + 0.5*Lin, Cy + 0.5*Win - 0.1*Win, Cz]
    p21 = [Cx + 0.5*Lin - 0.1*Lin, Cy + 0.5*Win, Cz]
    
    return p20, p19, p21


def get_circ_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win):
    
    Cx = center[0]
    Cy = center[1]
    Cz = center[2]

    # circular protrusion at bottom
    circ_protrsn_r = 0.1*Win
    p8 = [Cx - 0.5*Lin + 0.2*Lin, Cy - 0.5*Win, Cz]
    p11 = [Cx - 0.5*Lin + 0.2*Lin + 2*circ_protrsn_r, Cy - 0.5*Win, Cz]
    p9 = [Cx - 0.5*Lin + 0.2*Lin + circ_protrsn_r, Cy - 0.5*Win, Cz]
    p10 = [Cx - 0.5*Lin + 0.2*Lin + circ_protrsn_r, Cy - 0.5*Win + circ_protrsn_r, Cz]

    return circ_protrsn_r, p8, p11, p9, p10


def write_point_geo(geof, p_id, x, h):
    geof.write("Point(%d) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (p_id, x[0], x[1], x[2], h))
    return p_id + 1

def write_line_geo(geof, l_id, p1_id, p2_id):
    geof.write("Line(%d) = {%d, %d};\n" % (l_id, p1_id, p2_id))
    return l_id + 1

def write_cir_line_geo(geof, l_id, p1_id, p2_id, p3_id):
    geof.write("Circle(%d) = {%d, %d, %d};\n" % (l_id, p1_id, p2_id, p3_id))
    return l_id + 1

def write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, zone_string, Kn):
  inpf.write("  Zone_%s:\n" % (zone_string))
  inpf.write("    Contact_Radius_Factor: %4.6e\n" % (R_contact_factor))
  
  if damping_active == False:
    inpf.write("    Damping_On: false\n")
  if friction_active == False:
    inpf.write("    Friction_On: false\n")

  inpf.write("    Kn: %4.6e\n" % (Kn))
  inpf.write("    Epsilon: %4.6e\n" % (beta_n_eps))
  inpf.write("    Friction_Coeff: %4.6e\n" % (friction_coeff))
  inpf.write("    Kn_Factor: %4.6e\n" % (Kn_factor))
  inpf.write("    Beta_n_Factor: %4.6e\n" % (beta_n_factor))

def write_material_zone_part(inpf, zone_string, horizon, rho, K, G, Gc):
  inpf.write("  Zone_%s:\n" % (zone_string))
  inpf.write("    Type: PDState\n")
  inpf.write("    Horizon: %4.6e\n" % (horizon))
  inpf.write("    Density: %4.6e\n" % (rho))
  inpf.write("    Compute_From_Classical: true\n")
  inpf.write("    K: %4.6e\n" % (K))
  inpf.write("    G: %4.6e\n" % (G))
  inpf.write("    Gc: %4.6e\n" % (Gc))
  inpf.write("    Influence_Function:\n")
  inpf.write("      Type: 1\n")

def copy_contact_zone(inpf, zone_numbers, zone_copy_from):
  for s in zone_numbers:
    inpf.write("  Zone_%d:\n" % (s))
    inpf.write("    Copy_Contact_Data: " + print_int_list(zone_copy_from))

def get_E(K, nu):
  return 3. * K * (1. - 2. * nu)

def get_G(E, nu):
  return E / (2. * (1. + nu))


def get_eff_k(k1, k2):
  return 2. * k1 * k2 / (k1 + k2)


def get_max(l):
  l = np.array(l)
  return l.max()


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

def get_ref_rect_points(center, L, W, add_center = False):

  points = []
  if add_center:
    points.append(center)

  points.append([center[0] - 0.5*L, center[1] - 0.5*W, center[2]])
  points.append([center[0] + 0.5*L, center[1] - 0.5*W, center[2]])
  points.append([center[0] + 0.5*L, center[1] + 0.5*W, center[2]])
  points.append([center[0] - 0.5*L, center[1] + 0.5*W, center[2]])

  return points

def get_ref_triangle_points(center, radius, add_center = False):

  # triangle
  #                       2
  #                        +
  #
  #
  #                        o
  #                        1 
  #
  #              +-------------------+
  #             3                    4   

  # center and radius
  sim_Cx = center[0]
  sim_Cy = center[1]
  sim_Cz = center[2]
  sim_radius = radius

  cp = np.cos(np.pi/6.)
  sp = np.sin(np.pi/6.)

  points = []
  if add_center:
    points.append([sim_Cx, sim_Cy, sim_Cz])
  points.append([sim_Cx, sim_Cy + sim_radius, sim_Cz])
  points.append([sim_Cx - sim_radius*cp, sim_Cy - sim_radius*sp, sim_Cz])
  points.append([sim_Cx + sim_radius*cp, sim_Cy - sim_radius*sp, sim_Cz])

  return points

def get_ref_hex_points(center, radius, add_center = False):

  # drum2d
  #
  #                        v3           v2
  #                         +           +
  #
  #
  #                      +         o           +
  #                     v4         x            v1
  #
  #                         +           +
  #                        v5           v6
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

  points = []
  if add_center:
    points.append(center)

  for i in range(6):
    xi = rotate(axis, i*np.pi/3., rotate_axis)
    points.append([center[i] + radius * xi[i] for i in range(3)])

  return points

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

def does_rect_intersect_rect(r1, r2, padding):

  # enlarge rectangle by adding padding
  r1_padding = [r1[0] - padding, r1[1] - padding, r1[2], r1[3] + padding, r1[4] + padding, r1[5]]

  return r1_padding[0] < r2[3] and r1_padding[3] > r2[0] and r1_padding[1] < r2[4] and r1_padding[4] > r2[1]

def does_rect_intersect_rect_use_pair_coord(r1, r2, padding):

  # enlarge rectangle by adding padding
  p1_r1_padding = [r1[0][0] - padding, r1[0][1] - padding, r1[0][2]]
  p2_r1_padding = [r1[1][0] + padding, r1[1][1] + padding, r1[0][2]]
  
  p1_r2 = r2[0]
  p2_r2 = r2[1]

  return p1_r1_padding[0] < p2_r2[0] and p2_r1_padding[0] > p1_r2[0] and p1_r1_padding[1] < p2_r2[1] and p2_r1_padding[1] > p1_r2[1]

def does_intersect_with_rectangle_wall_with_protrusion_and_opening(p, particles, Lin, Win, center, padding):

  # p -> [id, x, y, z, r]

  p_center = [p[i+1] for i in range(3)]
  p_r = p[4]
  p_rect = [[p_center[0] - p_r, p_center[1] - p_r, p_center[2]], [p_center[0] + p_r, p_center[1] + p_r, p_center[2]]]

  for q in particles:

    pq = np.array([p[i+1] - q[i+1] for i in range(3)])
    if np.linalg.norm(pq) <= p[-1] + q[-1] + padding:
      return True

  dx = np.array([p[i+1] - center[i] for i in range(3)])
  if np.abs(dx[0]) > 0.5*Lin - p[-1] - padding:
    return True
  
  if np.abs(dx[1]) > 0.5*Win - p[-1] - padding:
    return True
  
  # get protrusion points and approximate the protruding region using rectangle and check for intersection
  # process triangle protrusion
  p2, p1, p3 = get_tri_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)
  tri_protrsn_rect_left_bottom = p2
  tri_protrsn_rect_right_top = [p2[0] + (p1[0] - p2[0]), p2[1] + (p3[1] - p2[1]), p2[2]]

  if does_rect_intersect_rect_use_pair_coord(p_rect, [tri_protrsn_rect_left_bottom, tri_protrsn_rect_right_top], padding):
    return True
  

  # process circular protrusion
  circ_protrsn_r, p1, p4, p2, p3 = get_circ_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)

  circ_protrsn_rect_left_bottom = p1
  circ_protrsn_rect_right_top = [p1[0] + 2 * circ_protrsn_r, p1[1] + circ_protrsn_r, p1[2]]

  return does_rect_intersect_rect_use_pair_coord(p_rect, [circ_protrsn_rect_left_bottom, circ_protrsn_rect_right_top], padding)

def particle_locations(inp_dir, pp_tag, center, padding, max_y, mesh_size, R1, R2, id_choices1, id_choices2, N1, N2, Lin, Win, z_coord, add_orient = True):

  np.random.seed(30)
  sim_inp_dir = str(inp_dir)
  
  """Generate particle location data"""

  particles = []
  points = []

  method_to_use = 0

  if method_to_use == 0:
    pcount = 0
    count = 0
    while pcount < N2 and count < 100*N2:
      if count%N2 == 0:
        print('large particle iter = ', count)

      # random radius and center location
      r = R2 + np.random.uniform(-0.1 * R2, 0.1 * R2)
      x = center[0] + np.random.uniform(-0.5*Lin + R2 + padding, 0.5*Lin - R2- padding)
      y = np.random.uniform(-0.5*Win + R2+padding, max_y - R2-padding)
      p_zone = np.random.choice(id_choices2, size=1)[0]
      if np.random.uniform(0., 1.) > 0.25:
        p_zone = np.random.choice(id_choices2, size=1)[0]
      p = [p_zone, x, y, center[2], r]

      # check if it collides of existing particles
      pintersect = does_intersect_with_rectangle_wall_with_protrusion_and_opening(p, particles, Lin, Win, center, padding)

      if pintersect == False:
        particles.append(p)
        pcount += 1

      count +=1

    pcount = 0
    count = 0
    while pcount < N1 and count < 100*N1:
      if count%N1 == 0:
        print('small particle iter = ', count)

      # random radius and center location
      r = R1 + np.random.uniform(-0.1 * R1, 0.1 * R1)
      x = center[0] + np.random.uniform(-0.5*Lin + R1 + padding, 0.5*Lin - R1- padding)
      y = np.random.uniform(-0.5*Lin + R1 + padding, max_y - R1 - padding)
      p_zone = np.random.choice(id_choices1, size=1)[0]
      if np.random.uniform(0., 1.) > 0.25:
        p_zone = np.random.choice(id_choices1, size=1)[0]
      p = [p_zone, x,y,center[2], r]

      # check if it collides of existing particles
      pintersect = does_intersect_with_rectangle_wall_with_protrusion_and_opening(p, particles, Lin, Win, center, padding)

      if pintersect == False:
        particles.append(p)
        pcount += 1
      count +=1

  inpf = open(inp_dir + 'particle_locations_' + str(pp_tag) + '.csv','w')
  if add_orient:
    inpf.write("i, x, y, z, r, o\n")
    counter = 0
    for p in particles:
      inpf.write("%d, %Lf, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4], np.random.uniform(0., 2.*np.pi)))
      counter += 1
  else:
    inpf.write("i, x, y, z, r\n")
    for p in particles:
      inpf.write("%d, %Lf, %Lf, %Lf, %Lf\n" % (int(p[0]), p[1], p[2], p[3], p[4]))


  inpf.close()

  # to visualize in paraview
  write_vtu = False
  if write_vtu:
    points = []
    rads = []
    zones = []
    for p in particles:
      points.append([p[1], p[2], p[3]])
      rads.append(p[-1])
      zones.append(int(p[0]))

    points = np.array(points)
    rads = np.array(rads)
    zones = np.array(zones)
    mesh = pv.PolyData(points)
    mesh["radius"] = rads
    mesh["zone"] = zones
    pv.save_meshio(sim_inp_dir + 'particle_locations_' + str(pp_tag) + '.vtu', mesh)

  print('number of particles created = {}'.format(len(particles)))

  return len(particles), particles


def generate_cir_particle_gmsh_input(inp_dir, filename, center, radius, mesh_size, pp_tag):

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


def generate_hex_particle_gmsh_input(inp_dir, filename, center, radius, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  points = get_ref_hex_points(center, radius, True)

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


def generate_tri_particle_gmsh_input(inp_dir, filename, center, radius, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # points
  points = get_ref_triangle_points(center, radius, True)

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  #
  # points
  #
  for i in range(4):
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
  geof.write("Line(3) = {4, 2};\n")

  #
  # surfaces
  #
  geof.write("Line Loop(1) = {1, 2, 3};\n")

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


def generate_drum2d_particle_gmsh_input(inp_dir, filename, center, radius, width, mesh_size, pp_tag):

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

def generate_rect_floor_gmsh_input(inp_dir, filename, center, L, W, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  points = get_ref_rect_points(center, L, W, False)

  h = mesh_size
  hout = h # 5*h

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  # points
  write_point_geo(geof, 1, points[0], hout)
  write_point_geo(geof, 2, points[1], hout)
  write_point_geo(geof, 3, points[2], h)
  write_point_geo(geof, 4, points[3], h)

  # line
  geof.write("Line(1) = {1, 2};\n")
  geof.write("Line(2) = {2, 3};\n")
  geof.write("Line(3) = {3, 4};\n")
  geof.write("Line(4) = {4, 1};\n")

  # surfaces
  geof.write("Line Loop(1) = {1, 2, 3, 4};\n")

  # plane surface
  geof.write("Plane Surface(1) = {1};\n")

  # close file
  geof.close()

def generate_rectangle_with_protrusion_and_opening_wall_gmsh_input_type2(inp_dir, filename, center, Lin, Win, L, W, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # center and radius
  Cx = center[0]
  Cy = center[1]
  Cz = center[2]

  # Domain
  h = mesh_size
  hout = 3*h

  # hole in bottom edge
  hole_radius = 0.5 * (0.5*(W - Win))# half of thickness in vertical direction
  # left side points
  p5 = [Cx + 0.2*L, Cy - 0.5*W, Cz]
  p12 = [Cx + 0.2*L, Cy - 0.5*Win, Cz]
  p13 = [Cx + 0.2*L, Cy - 0.5*W + hole_radius, Cz]
  p14 = [Cx + 0.2*L + hole_radius, Cy - 0.5*W + hole_radius, Cz]

  p6 = [Cx + 0.28*L, Cy - 0.5*W, Cz]
  p16 = [Cx + 0.28*L, Cy - 0.5*Win, Cz]
  p17 = [Cx + 0.28*L, Cy - 0.5*W + hole_radius, Cz]
  p15 = [Cx + 0.28*L - hole_radius, Cy - 0.5*W + hole_radius, Cz]

  # triangle protrusion on upper-right corner
  #p20 = [Cx + 0.5*Lin - 0.15*Lin, Cy + 0.5*Win - 0.3*Win, Cz]
  #p19 = [Cx + 0.5*Lin, Cy + 0.5*Win - 0.1*Win, Cz]
  #p21 = [Cx + 0.5*Lin - 0.1*Lin, Cy + 0.5*Win, Cz]
  p20, p19, p21 = get_tri_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)

  # circular protrusion at bottom
  #circ_protrsn_r = 0.2*Win
  #p8 = [Cx - 0.5*Lin + 0.25*Lin, Cy - 0.5*Win, Cz]
  #p11 = [Cx - 0.5*Lin + 0.25*Lin + 2*circ_protrsn_r, Cy - 0.5*Win, Cz]
  #p9 = [Cx - 0.5*Lin + 0.25*Lin + circ_protrsn_r, Cy - 0.5*Win, Cz]
  #p10 = [Cx - 0.5*Lin + 0.25*Lin + circ_protrsn_r, Cy - 0.5*Win + circ_protrsn_r, Cz]
  circ_protrsn_r, p8, p11, p9, p10 = get_circ_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)

  # create gmsh input file
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  # ---------------------- #
  # points                 #
  # ---------------------- #
  # points (as shown in drawing above)
  p_id = 1
  # points on outer side
  p_id = write_point_geo(geof, p_id, [Cx -0.5*L, Cy - 0.5*W, Cz], hout) 
  p_id = write_point_geo(geof, p_id, [Cx + 0.5*L, Cy - 0.5*W, Cz], hout)
  p_id = write_point_geo(geof, p_id, [Cx + 0.5*L, Cy + 0.5*W, Cz], hout)
  p_id = write_point_geo(geof, p_id, [Cx - 0.5*L, Cy + 0.5*W, Cz], hout)

  p_id = write_point_geo(geof, p_id, p5, hout)
  p_id = write_point_geo(geof, p_id, p6, hout) 

  # points on inner side
  p_id = write_point_geo(geof, p_id, [Cx - 0.5*Lin, Cy - 0.5*Win, Cz], h)

  ## circular protrusion points
  p_id = write_point_geo(geof, p_id, p8, h)
  p_id = write_point_geo(geof, p_id, p9, hout) # treat this internal point as control for coarse mesh
  p_id = write_point_geo(geof, p_id, p10, h)
  p_id = write_point_geo(geof, p_id, p11, h)

  # circular opening points
  p_id = write_point_geo(geof, p_id, p12, h)
  p_id = write_point_geo(geof, p_id, p13, hout)
  p_id = write_point_geo(geof, p_id, p14, h)

  p_id = write_point_geo(geof, p_id, p15, h)
  p_id = write_point_geo(geof, p_id, p16, h)
  p_id = write_point_geo(geof, p_id, p17, hout)

  p_id = write_point_geo(geof, p_id, [Cx + 0.5*Lin, Cy - 0.5*Win, Cz], h)

  # triangular protrusion
  p_id = write_point_geo(geof, p_id, p19, h)
  p_id = write_point_geo(geof, p_id, p20, h)
  p_id = write_point_geo(geof, p_id, p21, h) 

  p_id = write_point_geo(geof, p_id, [Cx - 0.5*Lin, Cy + 0.5*Win, Cz], h)

  # ---------------------- #
  # lines                  #
  # ---------------------- #
  # lines to form outer boundary (careful with the bottom edge as this will be divided into two lines)
  l_id = 1
  l_id = write_line_geo(geof, l_id, 1, 5) 
  l_id = write_line_geo(geof, l_id, 6, 2)
  l_id = write_line_geo(geof, l_id, 2, 3)
  l_id = write_line_geo(geof, l_id, 3, 4)
  l_id = write_line_geo(geof, l_id, 4, 1)

  l_id = write_line_geo(geof, l_id, 7, 8)

  # circular arc for protrusion
  l_id = write_cir_line_geo(geof, l_id, 8, 9, 10)
  l_id = write_cir_line_geo(geof, l_id, 10, 9, 11)

  l_id = write_line_geo(geof, l_id, 11, 12)

  # circular arc for opening
  l_id = write_cir_line_geo(geof, l_id, 12, 13, 14)
  l_id = write_cir_line_geo(geof, l_id, 14, 13, 5)

  l_id = write_cir_line_geo(geof, l_id, 16, 17, 15)
  l_id = write_cir_line_geo(geof, l_id, 15, 17, 6)

  l_id = write_line_geo(geof, l_id, 16, 18)
  l_id = write_line_geo(geof, l_id, 18, 19)
  l_id = write_line_geo(geof, l_id, 19, 20)
  l_id = write_line_geo(geof, l_id, 20, 21)
  l_id = write_line_geo(geof, l_id, 21, 22)
  l_id = write_line_geo(geof, l_id, 22, 7)

  # ---------------------- #
  # line loops, ...        #
  # ---------------------- #
  # line loop to define surface
  # this works too ---> geof.write("Curve Loop(13) = {10, 6, -11, -1, -5, -4, -3, -2, -12, 7, 8, 9};\n")
  geof.write("Curve Loop(20) = {14, 15, 16, 17, 18, 19, 6, 7, 8, 9, 10, 11, -1, -5, -4, -3, -2, -12, -13};\n")

  # define surface
  geof.write("Plane Surface(21) = {20};\n")

  # # define physical groups
  # geof.write("Physical Point(22) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};\n")

  # geof.write("Physical Line(23) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};\n")

  # tag = '"' + "a" + '"'
  # geof.write("Physical Surface(%s) = {21};\n" % (tag))

  # close file
  geof.close()


def generate_rectangle_with_protrusion_and_opening_wall_gmsh_input_type3(inp_dir, filename, center, Lin, Win, L, W, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # center and radius
  Cx = center[0]
  Cy = center[1]
  Cz = center[2]

  # Domain
  h = mesh_size
  hout = h #3*h

  # hole in bottom edge
  # left side points
  p5 = [Cx + 0.2*L, Cy - 0.5*W, Cz]
  p12 = [Cx + 0.2*L, Cy - 0.5*Win, Cz]
  
  p6 = [Cx + 0.3*L, Cy - 0.5*W, Cz]
  p13 = [Cx + 0.3*L, Cy - 0.5*Win, Cz]

  # triangle protrusion on upper-right corner
  #p20 = [Cx + 0.5*Lin - 0.15*Lin, Cy + 0.5*Win - 0.3*Win, Cz]
  #p19 = [Cx + 0.5*Lin, Cy + 0.5*Win - 0.1*Win, Cz]
  #p21 = [Cx + 0.5*Lin - 0.1*Lin, Cy + 0.5*Win, Cz]
  p16, p15, p17 = get_tri_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)

  # circular protrusion at bottom
  #circ_protrsn_r = 0.2*Win
  #p8 = [Cx - 0.5*Lin + 0.25*Lin, Cy - 0.5*Win, Cz]
  #p11 = [Cx - 0.5*Lin + 0.25*Lin + 2*circ_protrsn_r, Cy - 0.5*Win, Cz]
  #p9 = [Cx - 0.5*Lin + 0.25*Lin + circ_protrsn_r, Cy - 0.5*Win, Cz]
  #p10 = [Cx - 0.5*Lin + 0.25*Lin + circ_protrsn_r, Cy - 0.5*Win + circ_protrsn_r, Cz]
  circ_protrsn_r, p8, p11, p9, p10 = get_circ_protrusion_points_for_rectangle_wall_with_protrusion_and_opening(center, Lin, Win)

  # create gmsh input file
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  # ---------------------- #
  # points                 #
  # ---------------------- #
  # points (as shown in drawing above)
  p_id = 1
  # points on outer side
  p_id = write_point_geo(geof, p_id, [Cx -0.5*L, Cy - 0.5*W, Cz], hout) 
  p_id = write_point_geo(geof, p_id, [Cx + 0.5*L, Cy - 0.5*W, Cz], hout)
  p_id = write_point_geo(geof, p_id, [Cx + 0.5*L, Cy + 0.5*W, Cz], hout)
  p_id = write_point_geo(geof, p_id, [Cx - 0.5*L, Cy + 0.5*W, Cz], hout)

  p_id = write_point_geo(geof, p_id, p5, hout)
  p_id = write_point_geo(geof, p_id, p6, hout) 

  # points on inner side
  p_id = write_point_geo(geof, p_id, [Cx - 0.5*Lin, Cy - 0.5*Win, Cz], h)

  ## circular protrusion points
  p_id = write_point_geo(geof, p_id, p8, h)
  p_id = write_point_geo(geof, p_id, p9, hout) # treat this internal point as control for coarse mesh
  p_id = write_point_geo(geof, p_id, p10, h)
  p_id = write_point_geo(geof, p_id, p11, h)

  # opening points
  p_id = write_point_geo(geof, p_id, p12, h)
  p_id = write_point_geo(geof, p_id, p13, h)

  p_id = write_point_geo(geof, p_id, [Cx + 0.5*Lin, Cy - 0.5*Win, Cz], h)

  # triangular protrusion
  p_id = write_point_geo(geof, p_id, p15, h)
  p_id = write_point_geo(geof, p_id, p16, h)
  p_id = write_point_geo(geof, p_id, p17, h) 

  p_id = write_point_geo(geof, p_id, [Cx - 0.5*Lin, Cy + 0.5*Win, Cz], h)

  # ---------------------- #
  # lines                  #
  # ---------------------- #
  # lines to form outer boundary (careful with the bottom edge as this will be divided into two lines)
  l_id = 1
  l_id = write_line_geo(geof, l_id, 1, 5) 
  l_id = write_line_geo(geof, l_id, 6, 2)
  l_id = write_line_geo(geof, l_id, 2, 3)
  l_id = write_line_geo(geof, l_id, 3, 4)
  l_id = write_line_geo(geof, l_id, 4, 1)

  l_id = write_line_geo(geof, l_id, 7, 8)

  # circular arc for protrusion
  l_id = write_cir_line_geo(geof, l_id, 8, 9, 10)
  l_id = write_cir_line_geo(geof, l_id, 10, 9, 11)

  l_id = write_line_geo(geof, l_id, 11, 12)

  # opening
  l_id = write_line_geo(geof, l_id, 5, 12)
  l_id = write_line_geo(geof, l_id, 13, 6)

  l_id = write_line_geo(geof, l_id, 13, 14)
  l_id = write_line_geo(geof, l_id, 14, 15)
  l_id = write_line_geo(geof, l_id, 15, 16)
  l_id = write_line_geo(geof, l_id, 16, 17)
  l_id = write_line_geo(geof, l_id, 17, 18)
  l_id = write_line_geo(geof, l_id, 18, 7)

  # ---------------------- #
  # line loops, ...        #
  # ---------------------- #
  # line loop to define surface
  geof.write("Curve Loop(20) = {12, 13, 14, 15, 16, 17, 6, 7, 8, 9, -10, -1, -5, -4, -3, -2, -11};\n")

  # define surface
  geof.write("Plane Surface(21) = {20};\n")

  # # define physical groups
  # geof.write("Physical Point(22) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};\n")

  # geof.write("Physical Line(23) = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};\n")

  # tag = '"' + "a" + '"'
  # geof.write("Physical Surface(%s) = {21};\n" % (tag))

  # # add center point to plane surface
  geof.write("Point{9} In Surface {21};")

  # close file
  geof.close()

def create_input_file(inp_dir, pp_tag):
  """Generates input file for two-particle test"""

  sim_inp_dir = str(inp_dir)

  # 1 - small particle circle
  # 2 - small particle triangle
  # 3 - small particle drum2d
  # 4 - small particle hex
  # 5 - large particle circle
  # 6 - large particle triangle
  # 7 - large particle drum2d
  # 8 - large particle hex
  # 9 - rectangle wall with protrusion and opening
  # 10 - base wall (floor)

  ## geometry
  center = [0., 0., 0.]
  R_small = 0.001
  R_large = 0.003

  mesh_size = R_small / 5.
  horizon = 2.5 * mesh_size
  
  L = 0.04
  W = 0.05
  Lin = L - horizon
  Win = W - horizon

  Lbase = L*1.25
  Wbase = horizon

  # define geometric parameters

  # base wall (below rotating rectangle container)
  base_wall_rect_center = [center[0], center[1] - 0.8*W, center[2]]
  base_wall_rect = [Lbase, Wbase, base_wall_rect_center[0], base_wall_rect_center[1], base_wall_rect_center[2]] # five parameters Length, Width, and center of rectangle

  # container geometry
  contain_rect = [Lbase, Lbase, center[0], center[1], center[2]]
  
  # small circle
  small_circle = [R_small, center[0], center[1], center[2]]
  # small triangle
  small_triangle = small_circle
  # small drum2d
  w_small_drum2d = R_small * 0.4
  small_drum2d = [R_small, w_small_drum2d, center[0], center[1], center[2]]
  # small hex
  small_hex = small_circle

  # large circle
  large_circle = [R_large, center[0], center[1], center[2]]
  # large triangle
  large_triangle = large_circle
  # large drum2d
  w_large_drum2d = R_large* 0.4
  large_drum2d = [R_large, w_large_drum2d, center[0], center[1], center[2]]
  # large hex
  large_hex = large_circle

  ## time 
  final_time = 0.1
  num_steps = 1000000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 400
  dt_out_n = num_steps / num_outputs
  test_dt_out_n = dt_out_n / 100
  perform_out = True

  ## material
  poisson = 0.25

  rho_wall = 1200.
  K_wall = 1.e+5
  E_wall = get_E(K_wall, poisson)
  G_wall = get_G(E_wall, poisson)
  Gc_wall = 100.

  rho_large = rho_wall
  K_large = K_wall
  E_large = E_wall
  G_large = G_wall
  Gc_large = Gc_wall

  rho_small = 1200.
  K_small = 1.e+4
  E_small = get_E(K_small, poisson)
  G_small = get_G(E_small, poisson)
  Gc_small = 50.

  ## contact
  # R_contact = 0.95 * mesh_size 
  # R_contact = 1.74e-04
  R_contact_factor = 0.95

  # Kn_V_max = 7.385158e+05
  # Kn = np.power(Kn_V_max, 2)
  # compute from bulk modulus

  # from bulk modulus
  Kn_small_small = 18. * get_eff_k(K_small, K_small) / (np.pi * np.power(horizon, 5))
  Kn_large_large = 18. * get_eff_k(K_large, K_large) / (np.pi * np.power(horizon, 5))
  Kn_wall_wall = 18. * get_eff_k(K_wall, K_wall) / (np.pi * np.power(horizon, 5))
  Kn_small_large = 18. * get_eff_k(K_small, K_large) / (np.pi * np.power(horizon, 5))
  Kn_small_wall = 18. * get_eff_k(K_small, K_wall) / (np.pi * np.power(horizon, 5))
  Kn_large_wall = 18. * get_eff_k(K_large, K_wall) / (np.pi * np.power(horizon, 5))

  # we do not want walls to interact
  Kn_wall_wall = 0.

  beta_n_eps = 0.95
  friction_coeff = 0.5
  damping_active = False
  damping_active_floor = True
  friction_active = False 
  beta_n_factor = 100.
  Kn_factor = 5.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## wall rotation rate
  wall_rotation_rate = -40. * np.pi
  wall_rotation_center = [0.15*Lin, 0.15*Win, 0.]

  ## neighbor search details
  neigh_search_factor = 10.
  neigh_search_interval = 40
  neigh_search_criteria = "simple_all"

  ### ---------------------------------------------------------------- ###
  # generate mesh and particle location data
  ### ---------------------------------------------------------------- ###

  # generate particle locations
  padding = 1.1 * R_contact_factor * mesh_size
  max_y = 0.5*Lin - 3*mesh_size
  # number of particles of small and large sizes
  N1, N2 = 50, 10
  id_choices1 = [0, 3, 1, 2]
  id_choices2 = [7, 4, 5, 6]
  num_particles_zones_1_to_8, particles_zones_1_to_8 = particle_locations(inp_dir, pp_tag, center, padding, max_y, mesh_size, R_small, R_large, id_choices1, id_choices2, N1, N2, Lin, Win, z_coord = 0., add_orient = True)

  # generate particle .geo file (large)
  zones_mesh_fnames = ["mesh_cir_small", "mesh_tri_small", "mesh_drum2d_small", "mesh_hex_small", "mesh_cir_large", "mesh_tri_large", "mesh_drum2d_large", "mesh_hex_large", "mesh_rect_protrsn_opening_container", "mesh_rect_floor"]

  ## circle
  generate_cir_particle_gmsh_input(inp_dir, zones_mesh_fnames[0], center, R_small, mesh_size, pp_tag)
  generate_cir_particle_gmsh_input(inp_dir, zones_mesh_fnames[4], center, R_large, mesh_size, pp_tag)

  ## triangle
  generate_tri_particle_gmsh_input(inp_dir, zones_mesh_fnames[1], center, R_small, mesh_size, pp_tag)
  generate_tri_particle_gmsh_input(inp_dir, zones_mesh_fnames[5], center, R_large, mesh_size, pp_tag)

  ## drum2d
  generate_drum2d_particle_gmsh_input(inp_dir, zones_mesh_fnames[2], center, R_small, 2.*w_small_drum2d, mesh_size, pp_tag)
  generate_drum2d_particle_gmsh_input(inp_dir, zones_mesh_fnames[6], center, R_large, 2.*w_large_drum2d, mesh_size, pp_tag)

  ## hex
  generate_hex_particle_gmsh_input(inp_dir, zones_mesh_fnames[3], center, R_small, mesh_size, pp_tag)
  generate_hex_particle_gmsh_input(inp_dir, zones_mesh_fnames[7], center, R_large, mesh_size, pp_tag)

  ## wall
  generate_rectangle_with_protrusion_and_opening_wall_gmsh_input_type3(inp_dir, zones_mesh_fnames[8], center, Lin, Win, L, W, mesh_size, pp_tag)
  generate_rect_floor_gmsh_input(inp_dir, zones_mesh_fnames[9], base_wall_rect_center, Lbase, Wbase, mesh_size, pp_tag)

  os.system("mkdir -p out")

  for s in zones_mesh_fnames:
    print('\n')
    print(s)
    print("gmsh {}_{}.geo -2 &> /dev/null".format(s, pp_tag))
    print('\n')

    os.system("gmsh {}_{}.geo -2".format(s, pp_tag))
    # os.system("gmsh {}_{}.geo -2 &> /dev/null".format(s, pp_tag))
    os.system("gmsh {}_{}.geo -2 -o {}_{}.vtk &> /dev/null".format(s, pp_tag, s, pp_tag))\


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

  # container info
  inpf.write("Container:\n")
  inpf.write("  Geometry:\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(contain_rect))

  # zone info
  inpf.write("Zone:\n")
  inpf.write("  Zones: 10\n")

  for i in range(10):
    inpf.write("  Zone_%d:\n" % (i+1))
    if i > 7: 
      inpf.write("    Is_Wall: true\n")
    else:
      inpf.write("    Is_Wall: false\n")

  # particle info
  inpf.write("Particle:\n")
  inpf.write("  Test_Name: multi_particle_attrition\n")

  particle_data = [['circle', small_circle], ['triangle', small_triangle], ['drum2d', small_drum2d], ['hexagon', small_hex], ['circle', large_circle], ['triangle', large_triangle], ['drum2d', large_drum2d], ['hexagon', large_hex]]
  for i in range(len(particle_data)):
    inpf.write("  Zone_%d:\n" % (i+1))
    inpf.write("    Type: %s\n" % (particle_data[i][0]))
    inpf.write("    Parameters: " + print_dbl_list((particle_data[i][1])))

  ## zone 9 (wall)
  inpf.write("  Zone_9:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(contain_rect))
  inpf.write("    All_Dofs_Constrained: true\n")
  inpf.write("    Create_Particle_Using_ParticleZone_GeomObject: true\n")

  ## zone 10 (wall)
  inpf.write("  Zone_10:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: rectangle\n")
  inpf.write("    Parameters: " + print_dbl_list(base_wall_rect))
  inpf.write("    All_Dofs_Constrained: true\n")
  inpf.write("    Create_Particle_Using_ParticleZone_GeomObject: true\n")

  # particle generation
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations_" + str(pp_tag) + ".csv\n")
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

  # Mesh info
  inpf.write("Mesh:\n")

  for i in range(10):
    inpf.write("  Zone_%d:\n" % (i+1))
    inpf.write("    File: %s\n" % (zones_mesh_fnames[i] + "_" + str(pp_tag) + ".msh"))

  # Contact info
  inpf.write("Contact:\n")

  ## 11
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "11", Kn_small_small)

  ## 12, 13, 14 --> copy from 11
  copy_contact_zone(inpf, [12, 13, 14], [1, 1])
  
  ## 15
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "15", Kn_small_large)

  ## 16, 17, 18 --> copy from 15
  copy_contact_zone(inpf, [16, 17, 18], [1, 5])

  ## 19
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "19", Kn_small_wall)

  ## 110 = (1, 10)
  write_contact_zone_part(inpf, R_contact_factor, damping_active_floor, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "110", Kn_small_wall)

  ## 22, 23, 24 --> copy from 11
  copy_contact_zone(inpf, [22, 23, 24], [1, 1])

  ## 25, 26, 27, 28 --> copy from 15
  copy_contact_zone(inpf, [25, 26, 27, 28], [1, 5])

  ## 29 --> copy from 19
  copy_contact_zone(inpf, [29], [1, 9])

  ## 210 --> copy from 110
  copy_contact_zone(inpf, [210], [1, 10])

  ## 33, 34 --> copy from 11
  copy_contact_zone(inpf, [33, 34], [1, 1])

  ## 35, 36, 37, 38 --> copy from 15
  copy_contact_zone(inpf, [35, 36, 37, 38], [1, 5])

  ## 39 --> copy from 19
  copy_contact_zone(inpf, [39], [1, 9])

  ## 310 --> copy from 110
  copy_contact_zone(inpf, [310], [1, 10])

  ## 44 --> copy from 11
  copy_contact_zone(inpf, [44], [1, 1])

  ## 45, 46, 47, 48 --> copy from 15
  copy_contact_zone(inpf, [45, 46, 47, 48], [1, 5])

  ## 49 --> copy from 19
  copy_contact_zone(inpf, [49], [1, 9])

  ## 410 --> copy from 110
  copy_contact_zone(inpf, [410], [1, 10])

  ## 55
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "55", Kn_large_large)

  ## 56, 57, 58 --> copy from 55
  copy_contact_zone(inpf, [56, 57, 58], [5, 5])

  ## 59
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "59", Kn_large_wall)

  ## 510
  write_contact_zone_part(inpf, R_contact_factor, damping_active_floor, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "510", Kn_large_wall)

  ## 66, 67, 68 --> copy from 55
  copy_contact_zone(inpf, [66, 67, 68], [5, 5])

  ## 69 --> copy from 59
  copy_contact_zone(inpf, [69], [5, 9])

  ## 610 --> copy from 510
  copy_contact_zone(inpf, [610], [5, 10])

  ## 77, 78 --> copy from 55
  copy_contact_zone(inpf, [77, 78], [5, 5])

  ## 79 --> copy from 59
  copy_contact_zone(inpf, [79], [5, 9])

  ## 710 --> copy from 510
  copy_contact_zone(inpf, [710], [5, 10])

  ## 88 --> copy from 55
  copy_contact_zone(inpf, [88], [5, 5])

  ## 89 --> copy from 59
  copy_contact_zone(inpf, [89], [5, 9])

  ## 810 --> copy from 510
  copy_contact_zone(inpf, [810], [5, 10])

  ## 99
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "99", Kn_wall_wall)

  ## 910 --> copy from 99
  copy_contact_zone(inpf, [910], [9, 9])

  ## 1010 --> copy from 99
  copy_contact_zone(inpf, [1010], [9, 9])

  # Neighbor info
  inpf.write("Neighbor:\n")
  inpf.write("  Update_Criteria: %s\n" % (neigh_search_criteria))
  inpf.write("  Search_Factor: %4.e\n" % (neigh_search_factor))
  inpf.write("  Search_Interval: %d\n" % (neigh_search_interval))

  # Material info
  inpf.write("Material:\n")

  ## zone 1
  write_material_zone_part(inpf, "1", horizon, rho_small, K_small, G_small, Gc_small)

  ## zone 2
  inpf.write("  Zone_2:\n")
  inpf.write("    Copy_Material_Data: 1\n")

  ## zone 3
  inpf.write("  Zone_3:\n")
  inpf.write("    Copy_Material_Data: 1\n")

  ## zone 4
  inpf.write("  Zone_4:\n")
  inpf.write("    Copy_Material_Data: 1\n")

  ## zone 5
  write_material_zone_part(inpf, "5", horizon, rho_large, K_large, G_large, Gc_large)

  ## zone 6
  inpf.write("  Zone_6:\n")
  inpf.write("    Copy_Material_Data: 5\n")

  ## zone 7
  inpf.write("  Zone_7:\n")
  inpf.write("    Copy_Material_Data: 5\n")

  ## zone 8
  inpf.write("  Zone_8:\n")
  inpf.write("    Copy_Material_Data: 5\n")

  ## zone 9
  write_material_zone_part(inpf, "9", horizon, rho_wall, K_wall, G_wall, Gc_wall)

  ## zone 10
  inpf.write("  Zone_10:\n")
  inpf.write("    Copy_Material_Data: 9\n")

  # Force
  if gravity_active == True:
    inpf.write("Force_BC:\n")
    inpf.write("  Gravity: " + print_dbl_list(gravity))

  # Displacement
  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 2\n")

  inpf.write("  Set_1:\n")
  # wall particle id will be num_particles_zones_1_to_8
  inpf.write("    Particle_List: [%d]\n" % (num_particles_zones_1_to_8))
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Time_Function:\n")
  inpf.write("      Type: rotation\n")
  inpf.write("      Parameters: "+print_dbl_list([wall_rotation_rate, wall_rotation_center[0], wall_rotation_center[1], wall_rotation_center[2]]))
  inpf.write("    Spatial_Function:\n")
  inpf.write("      Type: rotation\n")
  inpf.write("    Zero_Displacement: false\n")

  inpf.write("  Set_2:\n")
  # wall particle id will be num_particles_zones_1_to_8 + 1
  inpf.write("    Particle_List: [%d]\n" % (num_particles_zones_1_to_8 + 1))
  inpf.write("    Direction: [1,2]\n")
  inpf.write("    Zero_Displacement: true\n")

  #
  # Output info
  #
  inpf.write("Output:\n")
  inpf.write("  Path: ./out/\n")
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
  # inpf.write("    - Contact_Data\n")
  # inpf.write("    - Strain_Stress\n")
  inpf.write("  Output_Interval: %d\n" % (dt_out_n))
  inpf.write("  Compress_Type: zlib\n")
  inpf.write("  Perform_FE_Out: false\n")
  if perform_out:
    inpf.write("  Perform_Out: true\n")
  else:   
    inpf.write("  Perform_Out: false\n")
  inpf.write("  Test_Output_Interval: %d\n" % (test_dt_out_n))
  
  inpf.write("  Debug: 3\n")
  inpf.write("  Tag_PP: %d\n" %(int(pp_tag)))

  # close file
  inpf.close()


##-------------------------------------------------------##
##-------------------------------------------------------##
inp_dir = './'
pp_tag = 0
if len(sys.argv) > 1:
  pp_tag = int(sys.argv[1])

create_input_file(inp_dir, pp_tag)
