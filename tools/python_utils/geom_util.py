import os
import sys
import numpy as np
import gmsh
import math

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

def get_ref_rect_points(center, Lx, Ly, add_center = False):
  """
  Returns size points on reference rectangle

  Reference rectangle:
  
  +-------------------+
  |                   |
  |                   |
  |                   |
  +-------------------+

  Parameters
  ----------
  center: list
      Coordinates of center of rectangle
  Lx : float
      Length of rectangle in x-direction
  Ly : float
      Length of rectangle in y-direction
  add_center : bool
      True if we include the center to the returned list (first point in the returned list will be the center if the flag is true)

  Returns
  -------
  list
      Coordinates of points
  """

  p1 = [center[0] - Lx/2., center[1] - Ly/2., center[2]]
  p2 = [center[0] + Lx/2., center[1] - Ly/2., center[2]]
  p3 = [center[0] + Lx/2., center[1] + Ly/2., center[2]]
  p4 = [center[0] - Lx/2., center[1] + Ly/2., center[2]]

  return [p1, p2, p3, p4] if not add_center else [center, p1, p2, p3, p4]

def get_ref_triangle_points(center, radius, add_center = False):
  """
  Returns size points on reference triangle

  Reference triangle:
              
      + v2
      | \
      |      \
      |            \  
      |    o           + v1
      |            /
      |      /
      | /
      + v3
  
  Radius: o-v1 
  Axis vector: o-v1
  Rotation axis: [0, 0, 1]
  
  Parameters
  ----------
  center: list
      Coordinates of center of triangle
  radius : float
      Radius of triangle
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
  
  for i in range(3):
    xi = rotate(axis, i*2*np.pi/3., rotate_axis)
    points.append([center[i] + radius * xi[i] for i in range(3)])

  return points

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
  
  Radius: x-v1
  Axis vector: x-v1
  Rotation axis: [0, 0, 1]

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
  
  Radius: x-v2
  Width: v1-v4
  Axis vector: x-v1
  Rotation axis: [0, 0, 1]

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


def get_gmsh_entities():
    m = {}
    for e in gmsh.model.getEntities():
        bnd = gmsh.model.getBoundary([e])
        nod = gmsh.model.mesh.getNodes(e[0], e[1])
        ele = gmsh.model.mesh.getElements(e[0], e[1])
        m[e] = (bnd, nod, ele)
    
    return m

# transform the mesh and create new discrete entities to store it
# source - https://gitlab.onelab.info/gmsh/gmsh/-/blob/master/examples/api/mirror_mesh.py
def gmsh_transform(m, offset_entity, offset_node, offset_element, tx, ty, tz):
    for e in sorted(m):
        gmsh.model.addDiscreteEntity(
            e[0], e[1] + offset_entity,
            [(abs(b[1]) + offset_entity) * int(math.copysign(1, b[1])) for b in m[e][0]])
        coord = []
        for i in range(0, len(m[e][1][1]), 3):
            x = m[e][1][1][i] * tx
            y = m[e][1][1][i + 1] * ty
            z = m[e][1][1][i + 2] * tz
            coord.append(x)
            coord.append(y)
            coord.append(z)
        gmsh.model.mesh.addNodes(e[0], e[1] + offset_entity,
                                 [n + offset_node for n in m[e][1][0]], coord)
        gmsh.model.mesh.addElements(e[0], e[1] + offset_entity, m[e][2][0],
                                    [[t + offset_element for t in typ] for typ in m[e][2][1]],
                                    [[n + offset_node for n in typ] for typ in m[e][2][2]])
        if (tx * ty * tz) < 0: # reverse the orientation
            gmsh.model.mesh.reverse([(e[0], e[1] + offset_entity)])

def gmsh_transform_general(m, offset_entity, offset_node, offset_element, angle, axis=[0, 0, 1]):
    """Rotate mesh by angle (in radians) around specified axis
    
    Parameters:
    m - mesh data dictionary
    offset_entity - offset for entity numbers
    offset_node - offset for node numbers 
    offset_element - offset for element numbers
    angle - rotation angle in radians
    axis - rotation axis vector (default is z-axis [0,0,1])
    """
    
    for e in sorted(m):
        gmsh.model.addDiscreteEntity(
            e[0], e[1] + offset_entity,
            [(abs(b[1]) + offset_entity) * int(math.copysign(1, b[1])) for b in m[e][0]])
        coord = []
        # Apply rotation to each point
        for i in range(0, len(m[e][1][1]), 3):
            xi = np.array([m[e][1][1][i], m[e][1][1][i + 1], m[e][1][1][i + 2]])

            xi_rot = rotate(xi, angle, axis)

            coord.append(xi_rot[0])
            coord.append(xi_rot[1])
            coord.append(xi_rot[2])

        gmsh.model.mesh.addNodes(e[0], e[1] + offset_entity,
                                [n + offset_node for n in m[e][1][0]], coord)
        gmsh.model.mesh.addElements(e[0], e[1] + offset_entity, m[e][2][0],
                                  [[t + offset_element for t in typ] for typ in m[e][2][1]],
                                  [[n + offset_node for n in typ] for typ in m[e][2][2]])

def gmsh_translate(xc):
    """Translate mesh by vector xc
    
    Parameters:
    xc - translation vector [x, y, z]
    """
    # Get the nodes
    nodeTags, coord, paramCoord = gmsh.model.mesh.getNodes()
    numNodes = len(nodeTags)

    # Translation vector
    translation_vector = np.array([xc[0], xc[1], xc[2]])

    # Update each node individually
    for i in range(numNodes):
        nodeTag = nodeTags[i]
        old_coord = coord[3*i:3*(i+1)]
        new_coord = old_coord + translation_vector
        # Handle paramCoord properly - use empty list if paramCoord is None or empty
        param = paramCoord[3*i:3*(i+1)] if paramCoord is not None and len(paramCoord) > 0 else []
        gmsh.model.mesh.setNode(nodeTag, new_coord, param)