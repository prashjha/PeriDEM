import os
import numpy as np
import sys

def print_const(arg, fmt = '%4.6e', prefix = ""):
  return prefix + fmt % arg + '\n'

def print_list(arg, fmt = '%4.6e', delim = ', '):
    a = ''
    for i in range(len(arg)):
      a += fmt % arg[i] 
      if i < len(arg) - 1:
        a += delim
    return a

def print_bool(arg, prefix = ""):
  return prefix + 'True\n' if arg == True else 'False\n'

def print_dbl(arg, prefix = ""):
  return print_const(arg, '%4.6e', prefix)

def print_int(arg, prefix = ""):
  return print_const(arg, '%d', prefix)

def print_dbl_list(arg, prefix = ""):
  a = prefix + '[' + print_list(arg, '%4.6e') + ']\n'
  return a

def print_int_list(arg, prefix = ""):
  a = prefix + '[' + print_list(arg, '%4.6e') + ']\n'
  return a

def print_point_gmsh(arg, n):
  return 'Point(%d) = {' % n + print_list(arg) + '};\n'

def print_cir_gmsh(arg, n):
  return 'Circle(%d) = {' % n + print_list(arg, '%d') + '};\n'

def print_line_gmsh(arg, n):
  return 'Line(%d) = {' % n + print_list(arg, '%d') + '};\n'

def print_lineloop_gmsh(arg, n):
  return 'Line Loop(%d) = {' % n + print_list(arg, '%d') + '};\n'

def gmsh_file_hdr(f):
  f.write('cl__1 = 1;\n')
  f.write('Mesh.MshFileVersion = 2.2;\n')

def get_E(K, nu):
  """
  Returns Young's modulus given bulk modulus and Poisson's ratio

  Parameters
  ----------
  K : float
      Bulk modulus
  nu : float
      Poisson's ratio

  Returns
  -------
  float
      Young's modulus
  """
  return 3. * K * (1. - 2. * nu)

def get_G(E, nu):
  """
  Returns shear modulus given Young's modulus and Poisson's ratio

  Parameters
  ----------
  E: float
      Young's modulus
  nu : float
      Poisson's ratio

  Returns
  -------
  float
      Shear modulus
  """
  return E / (2. * (1. + nu))


def get_eff_k(k1, k2):
  """
  Returns effective bulk modulus

  Parameters
  ----------
  k1: float
      First bulk modulus
  k2 : float
      Second bulk modulus

  Returns
  -------
  float
      Effective bulk modulus
  """
  return 2. * k1 * k2 / (k1 + k2)

def get_max(l):
  """
  Returns maximum value in list

  Parameters
  ----------
  l: list
     List of values

  Returns
  -------
  float
      Maximum value
  """
  l = np.array(l)
  return l.max()

def rotate(p, theta, axis):
  """
  Returns rotation of vector about specified axis by specified angle

  Parameters
  ----------
  p: list
      Coordinates of vector
  theta : float
      Angle of rotation
  axis : list
      Axis of rotation

  Returns
  -------
  list
      Coordinates of rotated vector
  """
  p_np, axis_np = np.array(p), np.array(axis)
  ct, st = np.cos(theta), np.sin(theta)
  p_dot_n = np.dot(p_np,axis_np)
  n_cross_p = np.cross(axis_np, p_np)

  return (1. - ct) * p_dot_n * axis_np + ct * p_np + st * n_cross_p

def get_ref_hex_points(center, radius, add_center = False):
  """
  Returns size points on reference hexagon

  Reference hexagon:
  
                          v3           v2
                           +           +
  
  
                        +         o           +
                       v4         x            v1
  
                           +           +
                          v5           v6
  
  Axis is a vector from x to v1 and radius is distance between x and v1.

  Parameters
  ----------
  center: list
      Coordinates of center of hexagon
  radius : float
      Radius of hexagon
  add_center : bool
      True if we include the center to the returned list (first point in the returned list will be the center if the flag is true)

  Returns
  -------
  list
      Coordinates of points
  """
  axis = [1., 0., 0.]
  rotate_axis = [0., 0., 1.]
  points = []
  if add_center:
    points.append(center)

  for i in range(6):
    xi = rotate(axis, i*np.pi/3., rotate_axis)
    points.append([center[i] + radius * xi[i] for i in range(3)])

  return points

def get_ref_drum_points(center, radius, width, add_center = False):
  """
  Returns size points on reference concave polygon (we refer to it as drum)

  Reference concave polygon:
  
               v3                                v2
                 + -------------------------------- +
                  \                               /
                     \                         /
                       +         o           +
                      /v4        x          v1 \
                   /                              \ 
                 + -------------------------------- +
                 v5                               v6
  
  Axis is a vector from x to v1, radius is distance between x and v2, and width of neck is the distance between v2 and v4.

  Parameters
  ----------
  center: list
      Coordinates of center of polygon
  radius : float
      Radius of polygon (distance between center x and vertex v2)
  width : float
      Width of neck, i.e. distance between vertex v2 and v4
  add_center : bool
      True if we include the center to the returned list (first point in the returned list will be the center if the flag is true)

  Returns
  -------
  list
      Coordinates of points
  """
  axis = [1., 0., 0.]
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


def does_intersect(p, particles, padding):
  """
  Returns true if particle p intersects or is near enough to existing particles

  Parameters
  ----------
  p : list
      Coordinates of center and radius of particle [x,y,z,r]
  particles : list
      List of center + radius of multiple particles. E.g. particles[0] is a list containing coordinates of center and radius.
  padding: float
      Minimum distance between circle boundaries such that if two circles

  Returns
  -------
  bool
      True if particle intersects or is near enough to one of the particle in the list
  """
  if len(p) < 4: raise Exception('p = {} must have atleast 4 elements'.format(p))
  if len(particles) == 0: raise Exception('particles = {} can not be empty'.format(particles))
  if padding < 0.: raise Exception('padding = {} can not be negative'.format(padding))
    
  for q in particles:
    if len(q) < 4: raise Exception('q = {} in particles must have atleast 4 elements'.format(q))
    pq = np.array([p[i] - q[i] for i in range(3)])
    if np.linalg.norm(pq) <= p[3] + q[3] + padding:
      return True
  return False

def does_intersect_rect(p, particles, padding, rect, is_3d = False):
  """
  Returns true if particle p is sufficiently close or outside the rectangle (in 2d) or cuboid (in 3d)

  Parameters
  ----------
  p : list
      Coordinates of center and radius of particle [x,y,z,r]
  particles : list
      List of center + radius of multiple particles. E.g. particles[0] is a list containing coordinates of center and radius.
  padding: float
      Minimum distance between circle boundaries such that if two circles
  rect: list
      Coordinates of left-bottom and right-top corner points of rectangle (2d) or cuboid (3d). E.g. [x1 y1, z1, x2, y2, z2]
  is_3d: bool
      True if we are dealing with cuboid

  Returns
  -------
  bool
      True if particle intersects or is near enough to the rectangle
  """
  if len(p) < 4: raise Exception('p = {} must have atleast 4 elements'.format(p))
  if len(particles) == 0: raise Exception('particles = {} can not be empty'.format(particles))
  if padding < 0.: raise Exception('padding = {} can not be negative'.format(padding))
  if len(rect) < 6: raise Exception('rect = {} must have 6 elements'.format(rect))

  pr = [p[0] - p[3], p[1] - p[3], p[2], p[0] + p[3], p[1] + p[3], p[2]]
  if is_3d:
    pr[2] -= p[3]
    pr[5] += p[3]

  if pr[0] < rect[0] + padding or pr[1] < rect[1] + padding or pr[3] > rect[3] - padding or pr[4] > rect[4] - padding:
    if is_3d:
      if pr[2] < rect[2] + padding or pr[5] > rect[5] - padding:
        return True
    else:
      return True
  return False

def generate_circle_gmsh_input(filename, center, radius, mesh_size, pp_tag = None):
  """
  Creates .geo file for discretization of circle using gmsh

  Parameters
  ----------
  filename : str
      Filename of .geo file to be created
  center : list
      Coordinates of center of circle
  radius: float
      Radius of circle
  mesh_size: float
      Mesh size
  pp_tag: str, optional
      Postfix .geo file with this tag
  """
  pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
  geof = open(filename + pp_tag_str + '.geo','w')
  gmsh_file_hdr(geof)

  ## points
  points = []
  points.append([center[0], center[1], center[2]])
  points.append([center[0] + radius, center[1], center[2]])
  points.append([center[0] - radius, center[1], center[2]])
  points.append([center[0], center[1] + radius, center[2]])
  points.append([center[0], center[1] - radius, center[2]])
  for i in range(len(points)):
    geof.write(print_point_gmsh([points[i][0], points[i][1], points[i][2], mesh_size], i+1))

  ## circular arc
  geof.write(print_cir_gmsh([2,1,4], 1))
  geof.write(print_cir_gmsh([4,1,3], 2))
  geof.write(print_cir_gmsh([3,1,5], 3))
  geof.write(print_cir_gmsh([5,1,2], 4))

  ## line loop
  geof.write(print_lineloop_gmsh([2, 3, 4, 1], 1))

  ## plane surface
  geof.write("Plane Surface(1) = {1};\n")

  ## add center point to plane surface
  geof.write("Point{1} In Surface {1};")

  ## close file
  geof.close()

def generate_rectangle_gmsh_input(filename, rectangle, mesh_size, pp_tag = None):
  """
  Creates .geo file for discretization of rectangle using gmsh

  Parameters
  ----------
  filename : str
      Filename of .geo file to be created
  rectangle : list
      Coordinates of left-bottom and right-top corner points of rectangle
  mesh_size: float
      Mesh size
  pp_tag: str, optional
      Postfix .geo file with this tag
  """
  pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
  geof = open(filename + pp_tag_str + '.geo','w')
  gmsh_file_hdr(geof)

  ## points
  points = []
  points.append([rectangle[0], rectangle[1], rectangle[2]])
  points.append([rectangle[3], rectangle[1], rectangle[2]])
  points.append([rectangle[3], rectangle[4], rectangle[2]])
  points.append([rectangle[0], rectangle[4], rectangle[2]])
  for i in range(len(points)):
    geof.write(print_point_gmsh([points[i][0], points[i][1], points[i][2], mesh_size], i+1))

  ## lines
  for i in range(4):
    if i < 3:
      geof.write(print_line_gmsh([i+1,i+2], i+1))
    else:
      geof.write(print_line_gmsh([i+1,1], i+1))

  ## line loop
  geof.write(print_lineloop_gmsh([i+1 for i in range(4)], 1))

  ## plane surface
  geof.write("Plane Surface(1) = {1};\n")

  ## add center point to plane surface
  geof.write("Point{1} In Surface {1};")

  ## physical surface
  tag = '"' + "a" + '"'
  geof.write("Physical Surface(%s) = {1};\n" % (tag))

  ## close file
  geof.close()

def generate_hexagon_gmsh_input(filename, center, radius, mesh_size, pp_tag = None):
  """
  Creates .geo file for discretization of hexagon using gmsh

  Parameters
  ----------
  filename : str
      Filename of .geo file to be created
  center : list
      Coordinates of center of hexagon
  radius: float
      Radius 
  mesh_size: float
      Mesh size
  pp_tag: str, optional
      Postfix .geo file with this tag
  """
  pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
  geof = open(filename + pp_tag_str + '.geo','w')
  gmsh_file_hdr(geof)

  ## points
  points = get_ref_hex_points(center, radius, True)
  for i in range(len(points)):
    geof.write(print_point_gmsh([points[i][0], points[i][1], points[i][2], mesh_size], i+1))

  ## lines
  for i in range(6):
    if i < 5:
      geof.write(print_line_gmsh([i+2,i+3], i+1))
    else:
      geof.write(print_line_gmsh([i+2,2], i+1))

  ## line loop
  geof.write(print_lineloop_gmsh([i+1 for i in range(6)], 1))

  ## plane surface
  geof.write("Plane Surface(1) = {1};\n")

  ## add center point to plane surface
  geof.write("Point{1} In Surface {1};")

  ## close file
  geof.close()


def generate_drum_gmsh_input(filename, center, radius, width, mesh_size, pp_tag = None):
  """
  Creates .geo file for discretization of drum (concave polygon, see get_ref_drum_points) using gmsh

  Parameters
  ----------
  filename : str
      Filename of .geo file to be created
  center : list
      Coordinates of center
  radius: float
      Radius 
  width: float
      Neck width (see get_ref_drum_points())
  mesh_size: float
      Mesh size
  pp_tag: str, optional
      Postfix .geo file with this tag
  """
  pp_tag_str = '_{}'.format(str(pp_tag)) if pp_tag is not None else ''
  geof = open(filename + pp_tag_str + '.geo','w')
  gmsh_file_hdr(geof)

  ## points
  points = get_ref_drum_points(center, radius, width, True)
  for i in range(len(points)):
    geof.write(print_point_gmsh([points[i][0], points[i][1], points[i][2], mesh_size], i+1))

  ## lines
  for i in range(6):
    if i < 5:
      geof.write(print_line_gmsh([i+2,i+3], i+1))
    else:
      geof.write(print_line_gmsh([i+2,2], i+1))

  ## line loop
  geof.write(print_lineloop_gmsh([i+1 for i in range(6)], 1))

  ## plane surface
  geof.write("Plane Surface(1) = {1};\n")

  ## add center point to plane surface
  geof.write("Point{1} In Surface {1};")

  ## close file
  geof.close()
