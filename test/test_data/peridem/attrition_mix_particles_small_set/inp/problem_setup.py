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

def get_ref_rect_points(center, radius, add_center = False):

  p1 = [center[0] - radius, center[1] - radius, center[2]]
  p2 = [center[0] + radius, center[1] - radius, center[2]]
  p3 = [center[0] + radius, center[1] + radius, center[2]]
  p4 = [center[0] - radius, center[1] + radius, center[2]]

  return [p1, p2, p3, p4]

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

def does_intersect_with_cylindrical_wall(p, particles, R_in, center, bar_rect, padding):

  # p -> [id, x, y, z, r]

  p_center = [p[i+1] for i in range(3)]
  p_r = p[4]

  for q in particles:

    pq = np.array([p[i+1] - q[i+1] for i in range(3)])
    if np.linalg.norm(pq) <= p[-1] + q[-1] + padding:
      return True

  dx = np.array([p[i+1] - center[i] for i in range(3)])
  if np.linalg.norm(dx) > R_in - p[-1] - padding:
    return True

  
  p_rect = [p_center[0] - p_r, p_center[1] - p_r, p_center[2], p_center[0] + p_r, p_center[1] + p_r, p_center[2]]
  
  return does_rect_intersect_rect(p_rect, bar_rect, padding)

def particle_locations(inp_dir, pp_tag, center, padding, max_y, mesh_size, R1, R2, id_choices1, id_choices2, N1, N2, R_in, bar_rect, z_coord, add_orient = True):

  np.random.seed(30)
  sim_inp_dir = str(inp_dir)
  
  """Generate particle location data"""

  particles = []
  points = []

  method_to_use = 0

  if method_to_use == 0:
    pcount = 0
    count = 0
    select_count = 0
    while pcount < N2 and count < 100*N2:
      if count%N2 == 0:
        print('large particle iter = ', count)

      # random radius and center location
      r = R2 + np.random.uniform(-0.1 * R2, 0.1 * R2)
      x = center[0] + np.random.uniform(-R_in + R2 + padding, R_in - R2- padding)
      y = np.random.uniform(-R_in + R2+padding, max_y - R2-padding)
      #p_zone = np.random.choice(id_choices2, size=1)[0]
      p_zone = id_choices2[select_count % len(id_choices2)]
      p = [p_zone, x, y, center[2], r]

      # check if it collides of existing particles
      pintersect = does_intersect_with_cylindrical_wall(p, particles, R_in, center, bar_rect, padding)

      if pintersect == False:
        particles.append(p)
        pcount += 1
        select_count += 1

      count +=1

    pcount = 0
    count = 0
    select_count = 0
    while pcount < N1 and count < 100*N1:
      if count%N1 == 0:
        print('small particle iter = ', count)

      # random radius and center location
      r = R1 + np.random.uniform(-0.1 * R1, 0.1 * R1)
      x = center[0] + np.random.uniform(-R_in + R1 + padding, R_in - R1- padding)
      y = np.random.uniform(-R_in + R1 + padding, max_y - R1 - padding)
      #p_zone = np.random.choice(id_choices1, size=1)[0]
      p_zone = id_choices1[select_count % len(id_choices1)]
      p = [p_zone, x,y,center[2], r]

      # check if it collides of existing particles
      pintersect = does_intersect_with_cylindrical_wall(p, particles, R_in, center, bar_rect, padding)

      if pintersect == False:
        particles.append(p)
        pcount += 1
        select_count += 1

      count +=1

  elif method_to_use == 1:
    Nmax = N1 + N2
    # find how approximate number of rows and cols we can have
    check_r = R1
    if R1 > R2:
      check_r = R2

    rows = int(R_in / check_r)
    cols = int(R_in / check_r)

    rads = [R1, R2]

    counter = 0
    x_old = -R_in
    x_old_right = R_in
    y_old = -R_in
    pad = padding

    cx = 0.
    cy = 0.
    cz = z_coord
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
      while num_p_cols < cols - 1 and j < Nmax:

        if j == 0:
          # for odd row, take x_old as first and for even take as last
          x_old = -R_in
          x_old_right = R_in

        # ptype = counter % 2
        p_type = 0 # 1 or 2
        p_index = 0 # zone this particle it belongs
        if np.random.uniform(0., 1.) < 0.7:
          p_type = 0
          p_index = np.random.choice(id_choices1, size=1)[0]
        else:
          p_type = 1
          p_index = np.random.choice(id_choices2, size=1)[0]
        
        r0 = rads[p_type]

        # perturb radius
        r = r0 + np.random.uniform(-0.1 * r0, 0.1 * r0)
        # r = r0
        row_rads.append(r)

        # random horizontal perturbation by 10% of radius
        rph = np.random.uniform(-0.1 * r0, 0.1 * r0)

        cx0 = x_old + pad + r 
        cx = cx0 + rph
        # for even row, we start from right edge of rectange
        if i%2 == 0:
          cx0 = x_old_right - pad - r
          cx = cx0 - rph
        cy = y_old + pad + r

        # check if it collides of existing particles
        p = [float(p_index), cx, cy, cz, r]
        pintersect = does_intersect_with_cylindrical_wall(p, particles, R_in, center, bar_rect, padding)
        
        if pintersect == False:
          particles.append([float(p_index), cx, cy, cz, r])

          # set x_old
          if i % 2 == 0:
            x_old_right = cx - r
          else:
            x_old = cx + r    

          counter += 1

          num_p_cols += 1

        j += 1


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


def generate_cylindrical_wall_gmsh_input(inp_dir, filename, center, outer_radius, inner_radius, bar_width, bar_length, mesh_size, pp_tag):

  sim_inp_dir = str(inp_dir)

  # center and radius
  Cx = center[0]
  Cy = center[1]
  Cz = center[2]

  #
  # create .geo file for gmsh
  #
  geof = open(sim_inp_dir + filename + '_' + str(pp_tag) + '.geo','w')
  geof.write("cl__1 = 1;\n")
  geof.write("Mesh.MshFileVersion = 2.2;\n")

  #
  # points
  #
  points = []
  points.append([Cx, Cy, Cz])
  points.append([outer_radius, 0., 0.])
  points.append([0., outer_radius, 0.])
  points.append([-outer_radius, 0., 0.])
  points.append([0., -outer_radius, 0.])
  points.append([inner_radius, 0.5*bar_width, 0.])
  points.append([inner_radius, -0.5*bar_width, 0.])
  points.append([0., inner_radius, 0.])
  points.append([-inner_radius, 0., 0.])
  points.append([0., -inner_radius, 0.])
  points.append([inner_radius - bar_length, 0.5*bar_width, 0.])
  points.append([inner_radius - bar_length, -0.5*bar_width, 0.])

  count = 0
  for p in points:
    count += 1
    geof.write("Point(%d) = {%4.6e, %4.6e, %4.6e, %4.6e};\n" % (count, p[0], p[1], p[2], mesh_size))


  # 
  # circular arc
  #
  circles = []
  circles.append([2, 1, 3])
  circles.append([3, 1, 4])
  circles.append([4, 1, 5])
  circles.append([5, 1, 2])

  circles.append([6, 1, 8])
  circles.append([8, 1, 9])
  circles.append([9, 1, 10])
  circles.append([10, 1, 7])

  count = 0
  for c in circles:
    count += 1
    geof.write("Circle(%d) = {%d, %d, %d};\n" % (count, c[0], c[1], c[2]))

  #
  # line
  #
  lines = []
  lines.append([6, 11])
  lines.append([11, 12])
  lines.append([12, 7])
  #count = 0
  for l in lines:
    count += 1
    geof.write("Line(%d) = {%d, %d};\n" % (count, l[0], l[1]))

  # 
  # curve loop
  #
  geof.write("Line Loop(1) = {2, 3, 4, 1};\n")
  geof.write("Line Loop(2) = {6, 7, 8, -11, -10, -9, 5};\n")

  #
  # plane surface and physical tag
  #
  geof.write("Plane Surface(1) = {1, 2};\n")
  #geof.write("Physical Surface(1) = {1};\n")
  #geof.write("Physical Point(2) = {9, 4, 10, 5, 2, 7, 6, 11, 12, 3, 8};\n")
  tag = '"' + "a" + '"'
  geof.write("Physical Surface(%s) = {1};\n" % (tag))

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
  # 9 - cylindrical wall

  ## geometry
  center = [0., 0., 0.]
  R_small = 0.001
  R_large = 0.002

  mesh_size = R_small / 5.
  horizon = 2. * mesh_size
  
  R_in = 0.015
  R_out = R_in + 1.5*mesh_size

  L_bar = 0.004
  W_bar = 1.5*mesh_size
  bar_rectangle_attached_to_cylinder = [R_in - L_bar, -0.5 * W_bar, 0., R_in, 0.5 * W_bar, 0.]

  # define geometric parameters
  # wall
  wall_circle_plus = [R_out, center[0], center[1], center[2]]
  wall_circle_minus = [R_in, center[0], center[1], center[2]]
  wall_rectangle_plus = bar_rectangle_attached_to_cylinder
  wall_params = wall_circle_plus + wall_circle_minus + wall_rectangle_plus

  # small circle
  small_circle = [R_small, center[0], center[1], center[2]]
  # small triangle
  small_triangle = small_circle
  # small drum2d
  w_small_drum2d = R_small * 0.2
  small_drum2d = [R_small, w_small_drum2d, center[0], center[1], center[2]]
  # small hex
  small_hex = small_circle

  # large circle
  large_circle = [R_large, center[0], center[1], center[2]]
  # large triangle
  large_triangle = large_circle
  # large drum2d
  w_large_drum2d = R_large* 0.2
  large_drum2d = [R_large, w_large_drum2d, center[0], center[1], center[2]]
  # large hex
  large_hex = large_circle

  ## time 
  final_time = 0.05
  num_steps = 20000
  # final_time = 0.00002
  # num_steps = 2
  num_outputs = 10
  dt_out_n = num_steps / num_outputs
  test_dt_out_n = dt_out_n / 10
  perform_out = True

  ## material
  poisson = 0.25

  rho_wall = 600.
  K_wall = 1.e+4
  E_wall = get_E(K_wall, poisson)
  G_wall = get_G(E_wall, poisson)
  Gc_wall = 100.

  rho_small = 600.
  K_small = 5.e+3
  E_small = get_E(K_small, poisson)
  G_small = get_G(E_small, poisson)
  Gc_small = 100.

  rho_large = rho_small
  K_large = K_small
  E_large = E_small
  G_large = G_small
  Gc_large = Gc_small

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

  beta_n_eps = 0.95
  friction_coeff = 0.5
  damping_active = False
  friction_active = False 
  beta_n_factor = 100.
  Kn_factor = 1.

  ## gravity
  gravity_active = True
  gravity = [0., -10., 0.]

  ## wall rotation rate
  wall_rotation_rate = -40. * np.pi
  wall_rotation_center = [-0.2*R_in, 0.2*R_in, 0.]

  ## neighbor search details
  neigh_search_factor = 10.
  neigh_search_interval = 100
  neigh_search_criteria = "simple_all"

  ### ---------------------------------------------------------------- ###
  # generate mesh and particle location data
  ### ---------------------------------------------------------------- ###

  # generate particle locations
  padding = 1.1 * R_contact_factor * mesh_size
  max_y = 0.4*R_in
  # number of particles of small and large sizes
  N1, N2 = 8, 4 
  num_particles_zones_1_to_8, particles_zones_1_to_8 = particle_locations(inp_dir, pp_tag, center, padding, max_y, mesh_size, R_small, R_large, [0, 1, 2, 3], [4, 5, 6, 7], N1, N2, R_in, bar_rectangle_attached_to_cylinder, z_coord = 0., add_orient = True)

  # generate particle .geo file (large)
  zones_mesh_fnames = ["mesh_cir_small", "mesh_tri_small", "mesh_drum2d_small", "mesh_hex_small", "mesh_cir_large", "mesh_tri_large", "mesh_drum2d_large", "mesh_hex_large", "mesh_wall"]

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
  generate_cylindrical_wall_gmsh_input(inp_dir, zones_mesh_fnames[8], center, R_out, R_in, W_bar, L_bar, mesh_size, pp_tag)

  os.system("mkdir -p ../out")

  for s in zones_mesh_fnames:
    print('\n')
    print(s)
    print("gmsh {}_{}.geo -2 &> /dev/null".format(s, pp_tag))
    print('\n')

    os.system("gmsh {}_{}.geo -2".format(s, pp_tag))
    #os.system("gmsh {}_{}.geo -2 &> /dev/null".format(s, pp_tag))
    #os.system("gmsh {}_{}.geo -2 -o {}_{}.vtk &> /dev/null".format(s, pp_tag, s, pp_tag))


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
  inpf.write("    Type: complex\n")
  inpf.write("    Vec_Type: [circle, circle, rectangle]\n")
  inpf.write("    Vec_Flag: [plus, minus, plus]\n")
  inpf.write("    Parameters: " + print_dbl_list(wall_params))

  # zone info
  inpf.write("Zone:\n")
  inpf.write("  Zones: 9\n")

  for i in range(9):
    inpf.write("  Zone_%d:\n" % (i+1))
    if i == 8: 
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

  ## last zone (wall)
  inpf.write("  Zone_9:\n")
  inpf.write("    Is_Wall: true\n")
  inpf.write("    Type: complex\n")
  inpf.write("    Vec_Type: [circle, circle, rectangle]\n")
  inpf.write("    Vec_Flag: [plus, minus, plus]\n")
  inpf.write("    Parameters: " + print_dbl_list(wall_params))
  inpf.write("    All_Dofs_Constrained: true\n")
  inpf.write("    Create_Particle_Using_ParticleZone_GeomObject: true\n")

  # particle generation
  inpf.write("Particle_Generation:\n")
  inpf.write("  From_File: particle_locations_" + str(pp_tag) + ".csv\n")
  inpf.write("  File_Data_Type: loc_rad_orient\n") 

  # Mesh info
  inpf.write("Mesh:\n")

  for i in range(9):
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

  ## 22, 23, 24 --> copy from 11
  copy_contact_zone(inpf, [22, 23, 24], [1, 1])

  ## 25, 26, 27, 28 --> copy from 15
  copy_contact_zone(inpf, [25, 26, 27, 28], [1, 5])

  ## 29 --> copy from 19
  copy_contact_zone(inpf, [29], [1, 9])

  ## 33, 34 --> copy from 11
  copy_contact_zone(inpf, [33, 34], [1, 1])

  ## 35, 36, 37, 38 --> copy from 15
  copy_contact_zone(inpf, [35, 36, 37, 38], [1, 5])

  ## 39 --> copy from 19
  copy_contact_zone(inpf, [39], [1, 9])

  ## 44 --> copy from 11
  copy_contact_zone(inpf, [44], [1, 1])

  ## 45, 46, 47, 48 --> copy from 15
  copy_contact_zone(inpf, [45, 46, 47, 48], [1, 5])

  ## 49 --> copy from 19
  copy_contact_zone(inpf, [49], [1, 9])

  ## 55
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "55", Kn_large_large)

  ## 56, 57, 58 --> copy from 55
  copy_contact_zone(inpf, [56, 57, 58], [5, 5])

  ## 59
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "59", Kn_large_wall)

  ## 66, 67, 68 --> copy from 55
  copy_contact_zone(inpf, [66, 67, 68], [5, 5])

  ## 69 --> copy from 59
  copy_contact_zone(inpf, [69], [5, 9])

  ## 77, 78 --> copy from 55
  copy_contact_zone(inpf, [77, 78], [5, 5])

  ## 79 --> copy from 59
  copy_contact_zone(inpf, [79], [5, 9])

  ## 88 --> copy from 55
  copy_contact_zone(inpf, [88], [5, 5])

  ## 89 --> copy from 59
  copy_contact_zone(inpf, [89], [5, 9])

  ## 99
  write_contact_zone_part(inpf, R_contact_factor, damping_active, friction_active, beta_n_eps, friction_coeff, Kn_factor, beta_n_factor, "99", Kn_wall_wall)

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

  # Force
  if gravity_active == True:
    inpf.write("Force_BC:\n")
    inpf.write("  Gravity: " + print_dbl_list(gravity))

  # Displacement
  inpf.write("Displacement_BC:\n")
  inpf.write("  Sets: 1\n")

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
  
  inpf.write("  Debug: 2\n")
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
