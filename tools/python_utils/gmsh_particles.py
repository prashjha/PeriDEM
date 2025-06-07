import os
import sys
import numpy as np
import gmsh
import math

from util import *
from geom_util import *



def circle_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a circular mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left corner for non-symmetric mesh
  r - radius of the circle
  h - mesh size
  symmetric_mesh - if True, creates 1/4 mesh and mirrors it. If False, creates full circle
  """

  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # we first assume circle center is at origin and then translate the symmetric mesh to the desired location
    xc_mesh = [0., 0., 0.]
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)
    p2 = gmsh.model.geo.addPoint(xc_mesh[0] + r, xc_mesh[1], xc_mesh[2], h)
    p3 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + r, xc_mesh[2], h)

    l1 = gmsh.model.geo.addCircleArc(p2, p1, p3)
    l2 = gmsh.model.geo.addLine(p1, p2)
    l3 = gmsh.model.geo.addLine(p3, p1)

    c1 = gmsh.model.geo.addCurveLoop([l2, l1, l3])

    p1 = gmsh.model.geo.addPlaneSurface([c1])

    #gmsh.model.occ.addBox(0,0,0, 1,0.5,0.5)
    gmsh.model.geo.synchronize()
    # gmsh.model.mesh.setSize(gmsh.model.getEntities(0), 0.1)
    # gmsh.model.mesh.setSize([(0, 2)], 0.01)
    gmsh.model.mesh.generate(3)
    # gmsh.write('mesh.vtk')

    # get the mesh data
    m = get_gmsh_entities()

    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1) # x-axis
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1) # y-axis
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1) # z-axis

    # remove the duplicate nodes that will have been created on the internal
    # boundaries
    gmsh.model.mesh.removeDuplicateNodes()

    # translate the mesh to the specified center coordinates
    gmsh_translate(xc)
    
  else:
    # Create circle using built-in gmsh circle
    c = gmsh.model.occ.addCircle(xc[0], xc[1], xc[2], r)
    
    # Create curve loop from circle
    cl = gmsh.model.occ.addCurveLoop([c])
    
    # Create surface from curve loop
    s = gmsh.model.occ.addPlaneSurface([cl])
    
    # Add center point
    p = gmsh.model.occ.addPoint(xc[0], xc[1], xc[2], h)
    
    # Synchronize
    gmsh.model.occ.synchronize()
    
    # Embed center point in surface
    gmsh.model.mesh.embed(0, [p], 2, s)

    # generate mesh
    gmsh.model.mesh.generate(3)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # write
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def ellipse_mesh_symmetric(xc = [0., 0., 0.], rx = 1., ry = 0.5, h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create an elliptical mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left corner for non-symmetric mesh
  rx - radius in x direction
  ry - radius in y direction
  h - mesh size
  symmetric_mesh - if True, creates 1/4 mesh and mirrors it. If False, creates full ellipse
  """

  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # we first assume ellipse center is at origin and then translate the symmetric mesh to the desired location
    xc_mesh = [0., 0., 0.]
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)  # Center
    p2 = gmsh.model.geo.addPoint(xc_mesh[0] + rx, xc_mesh[1], xc_mesh[2], h)  # Right
    p3 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + ry, xc_mesh[2], h)  # Top

    # Create elliptical arc using 3 points
    l1 = gmsh.model.geo.addEllipseArc(p2, p1, p2, p3)  # Right to top
    l2 = gmsh.model.geo.addLine(p1, p2)  # Center to right
    l3 = gmsh.model.geo.addLine(p3, p1)  # Top to center

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l2, l1, l3])
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)

    # get the mesh data
    m = get_gmsh_entities()

    # Mirror the mesh to create full ellipse
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # x-axis
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # y-axis
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # z-axis

    # Remove duplicate nodes on internal boundaries
    gmsh.model.mesh.removeDuplicateNodes()

    # Translate to specified center coordinates
    gmsh_translate(xc)
      
  else:
    # Create points for full ellipse
    p1 = gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h)  # Center
    p2 = gmsh.model.geo.addPoint(xc[0] + rx, xc[1], xc[2], h)  # Right
    p3 = gmsh.model.geo.addPoint(xc[0], xc[1] + ry, xc[2], h)  # Top
    p4 = gmsh.model.geo.addPoint(xc[0] - rx, xc[1], xc[2], h)  # Left
    p5 = gmsh.model.geo.addPoint(xc[0], xc[1] - ry, xc[2], h)  # Bottom

    # Create elliptical arcs
    l1 = gmsh.model.geo.addEllipseArc(p2, p1, p2, p3)  # Right to top
    l2 = gmsh.model.geo.addEllipseArc(p3, p1, p3, p4)  # Top to left
    l3 = gmsh.model.geo.addEllipseArc(p4, p1, p4, p5)  # Left to bottom
    l4 = gmsh.model.geo.addEllipseArc(p5, p1, p5, p2)  # Bottom to right

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    # synchronize
    gmsh.model.geo.synchronize()

      # embed p0 (dim 0) in surface 1 (dim 2)
    gmsh.model.mesh.embed(0, [p1], 2, s1)

    # generate mesh
    gmsh.model.mesh.generate(3)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def sphere_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a spherical mesh, either symmetric or full.
  xc - center point coordinates [x, y, z]
  r - radius of the sphere
  h - mesh size
  symmetric_mesh - if True, creates 1/8 mesh and mirrors it. If False, creates full sphere
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/8th of sphere at origin, then mirror it
    xc_mesh = [0., 0., 0.]
    
    # Create first octant of sphere using OpenCASCADE
    sphere = gmsh.model.occ.addSphere(xc_mesh[0], xc_mesh[1], xc_mesh[2], r)

    # cubes to cut the sphere
    cut_left = gmsh.model.occ.addBox(xc_mesh[0]-r, xc_mesh[1]-r, xc_mesh[2]-r, r, 2*r, 2*r)
    cut_back = gmsh.model.occ.addBox(xc_mesh[0]-r, xc_mesh[1]-r, xc_mesh[2]-r, 2*r, r, 2*r)
    cut_bottom = gmsh.model.occ.addBox(xc_mesh[0]-r, xc_mesh[1]-r, xc_mesh[2]-r, 2*r, 2*r, r)

    # Cut sphere to get first octant using 3 planes
    gmsh.model.occ.cut([(3,sphere)], [(3,cut_left), (3,cut_back), (3,cut_bottom)])

    gmsh.model.occ.synchronize()

    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)

    # Generate 3D mesh
    gmsh.model.mesh.generate(3)

    # gmsh.fltk.run()

    # Get mesh data for mirroring
    m = get_gmsh_entities()

    # Mirror mesh in x direction
    gmsh_transform(m, 1000, 10000, 100000, -1, 1, 1)
    # Mirror in y direction 
    gmsh_transform(m, 2000, 20000, 200000, 1, -1, 1)
    # Mirror in z direction
    gmsh_transform(m, 3000, 30000, 300000, 1, 1, -1)
    # Mirror in xy plane
    gmsh_transform(m, 4000, 40000, 400000, -1, -1, 1)
    # Mirror in yz plane
    gmsh_transform(m, 5000, 50000, 500000, 1, -1, -1)
    # Mirror in xz plane
    gmsh_transform(m, 6000, 60000, 600000, -1, 1, -1)
    # Mirror in xyz
    gmsh_transform(m, 7000, 70000, 700000, -1, -1, -1)

    # Translate to specified center if not at origin
    gmsh_translate(xc)
      
  else:
    # Create points for full sphere
    # Create full sphere using OpenCASCADE
    sphere = gmsh.model.occ.addSphere(xc[0], xc[1], xc[2], r)
    
    # Synchronize the OpenCASCADE geometry
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Synchronize
    gmsh.model.geo.synchronize()
    
    # Embed center point in volume
    p1 = gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h)
    gmsh.model.mesh.embed(0, [p1], 3, sphere)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def rectangle_mesh_symmetric(xc = [0., 0., 0.], Lx = 1., Ly = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a rectangular mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left corner for non-symmetric mesh
  Lx - length in x direction 
  Ly - length in y direction
  h - mesh size
  symmetric_mesh - if True, creates 1/4 mesh and mirrors it. If False, creates full rectangle
  """
  # Initialize gmsh
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # center at origin
    xc_mesh = [0., 0., 0.]

    # Create points for 1/4 rectangle
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)
    p2 = gmsh.model.geo.addPoint(xc_mesh[0]+0.5*Lx, xc_mesh[1], xc_mesh[2], h)
    p3 = gmsh.model.geo.addPoint(xc_mesh[0]+0.5*Lx, xc_mesh[1]+0.5*Ly, xc_mesh[2], h)
    p4 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1]+0.5*Ly, xc_mesh[2], h)

    # Create lines
    l1 = gmsh.model.geo.addLine(p1, p2)
    l2 = gmsh.model.geo.addLine(p2, p3)
    l3 = gmsh.model.geo.addLine(p3, p4)
    l4 = gmsh.model.geo.addLine(p4, p1)

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    # Synchronize and generate mesh
    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)

    # Get the mesh data for mirroring
    m = get_gmsh_entities()

    # Mirror the mesh in all quadrants
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)

    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()

    # translate the mesh to the specified center coordinates
    gmsh_translate(xc)

  else:
    # Create points for full rectangle
    p0 = gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h)  # Center
    p1 = gmsh.model.geo.addPoint(xc[0] - 0.5*Lx, xc[1] - 0.5*Ly, xc[2], h)  # Bottom left
    p2 = gmsh.model.geo.addPoint(xc[0] + 0.5*Lx, xc[1] - 0.5*Ly, xc[2], h)  # Bottom right
    p3 = gmsh.model.geo.addPoint(xc[0] + 0.5*Lx, xc[1] + 0.5*Ly, xc[2], h)  # Top right
    p4 = gmsh.model.geo.addPoint(xc[0] - 0.5*Lx, xc[1] + 0.5*Ly, xc[2], h)  # Top left

    # Create lines
    l1 = gmsh.model.geo.addLine(p1, p2)  # Bottom
    l2 = gmsh.model.geo.addLine(p2, p3)  # Right
    l3 = gmsh.model.geo.addLine(p3, p4)  # Top
    l4 = gmsh.model.geo.addLine(p4, p1)  # Left

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    # synchronize
    gmsh.model.geo.synchronize()

    # embed p0 (dim 0) in surface 1 (dim 2)
    gmsh.model.mesh.embed(0, [p0], 2, s1)

    # generate mesh
    gmsh.model.mesh.generate(3)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # write
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def hexagon_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a hexagon mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left corner for non-symmetric mesh
  r - radius of the hexagon
  h - mesh size
  symmetric_mesh - if True, creates 1/6 mesh and mirrors it. If False, creates full hexagon
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/6 hexagon mesh at origin first, then rotate and translate
    xc_mesh = [0., 0., 0.]
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)  # Center (origin)
    p2 = gmsh.model.geo.addPoint(xc_mesh[0] + r, xc_mesh[1], xc_mesh[2], h)  # Point on x-axis (p1)
    p3 = gmsh.model.geo.addPoint(xc_mesh[0] + r*np.cos(np.pi/3), xc_mesh[1] + r*np.sin(np.pi/3), xc_mesh[2], h)  # p2

    # Create lines for 1/6th of hexagon
    l1 = gmsh.model.geo.addLine(p1, p2)  # Center to p1
    l2 = gmsh.model.geo.addLine(p2, p3)  # p1 to p2
    l3 = gmsh.model.geo.addLine(p3, p1)  # p2 to center

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l1, l2, l3])
    s1 = gmsh.model.geo.addPlaneSurface([c1])
    
    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)

    # Get mesh data for rotation
    m = get_gmsh_entities()

    # Rotate the mesh 5 times by 60 degrees
    for i in range(5):
        angle = (i + 1) * np.pi/3
        gmsh_transform_general(m, 1000*(i+1), 1000000*(i+1), 1000000*(i+1), angle)

    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()

    # Translate to specified center
    gmsh_translate(xc)

  else:
    # For non-symmetric mesh, create full hexagon directly
    points = []
    for i in range(6):
        angle = i * np.pi/3
        px = xc[0] + r * np.cos(angle)
        py = xc[1] + r * np.sin(angle)
        points.append(gmsh.model.geo.addPoint(px, py, xc[2], h))
    
    points.append(gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h))

    # Create lines connecting points
    lines = []
    for i in range(5):
        lines.append(gmsh.model.geo.addLine(points[i], points[i+1]))
    lines.append(gmsh.model.geo.addLine(points[5], points[0]))

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop(lines)
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    # synchronize
    gmsh.model.geo.synchronize()

    # embed p1 (dim 0) in surface 1 (dim 2)
    gmsh.model.mesh.embed(0, [points[-1]], 2, s1)

    # generate mesh
    gmsh.model.mesh.generate(3)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def drum2d_mesh_symmetric(xc = [0., 0., 0.], r = 1., width = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a drum2d mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left corner for non-symmetric mesh
  r - radius of the drum2d
  width - width of the drum2d
  h - mesh size
  symmetric_mesh - if True, creates 1/4 mesh and mirrors it. If False, creates full drum2d
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/4 drum2d mesh at origin first, then rotate and translate
    xc_mesh = [0., 0., 0.]
    x_drum = get_ref_drum_points(xc_mesh, r, width)
    v1, v2, v3 = x_drum[0], x_drum[1], x_drum[2]
    v23 = [0.5*(v2[0] + v3[0]), 0.5*(v2[1] + v3[1]), 0.5*(v2[2] + v3[2])]

    # 1/4th mesh is a polygon with 4 points (o, v1, v2, (v2+v3)/2)
    # Create points for 1/4th mesh
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)  # Center point
    p2 = gmsh.model.geo.addPoint(v1[0], v1[1], v1[2], h)
    p3 = gmsh.model.geo.addPoint(v2[0], v2[1], v2[2], h)
    p4 = gmsh.model.geo.addPoint(v23[0], v23[1], v23[2], h)

    # Create lines
    lines = []
    points = [p1, p2, p3, p4, p1]  # Add p1 again at end to complete loop
    for i in range(4):
        lines.append(gmsh.model.geo.addLine(points[i], points[i+1]))

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop(lines)
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)

    # Get mesh data for mirroring
    m = get_gmsh_entities()

    # Mirror mesh to create full drum2d
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # x-axis
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # y-axis
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # z-axis

    # Remove duplicate nodes on internal boundaries
    gmsh.model.mesh.removeDuplicateNodes()

    # Translate to specified center
    gmsh_translate(xc)

  else:
    # For non-symmetric mesh, create full drum2d directly
    x_drum = get_ref_drum_points(xc, r, width)
    # add points
    points = []
    points.append(gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h))
    for i in range(len(x_drum)):
        points.append(gmsh.model.geo.addPoint(x_drum[i][0], x_drum[i][1], x_drum[i][2], h))

    # Create lines connecting vertices
    lines = []
    points.append(points[1]) # to close the loop
    for i in range(len(x_drum)):
        lines.append(gmsh.model.geo.addLine(points[i+1], points[i+2]))

    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop(lines)
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    # synchronize
    gmsh.model.geo.synchronize()

    # embed p1 (dim 0) in surface 1 (dim 2)
    gmsh.model.mesh.embed(0, [points[0]], 2, s1)

    # generate mesh
    gmsh.model.mesh.generate(3)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')

  gmsh.finalize()

def polygon_mesh_symmetric(points, theta, xc=[0., 0., 0.], h=0.1, filename='mesh', vtk_out=False, symmetric_mesh = True):
  """
  Create a symmetric mesh by rotating a polygon segment n times, where n = 360/theta.
  
  Parameters:
  points - List of [x,y,z] coordinates defining the polygon segment. First point must be [0,0,0]
          and first two edges must form angle theta at origin
  theta - Angle in radians between first two edges at origin. Must divide 2pi evenly.
  xc - Center point for final translation
  h - Mesh size
  filename - Output filename
  vtk_out - Whether to output VTK file
  """
  # Validate inputs
  if not points or len(points) < 3:
    raise ValueError("Need at least 3 points to define a polygon segment")
  
  if not np.allclose(points[0], [0., 0., 0.]):
    raise ValueError("First point must be at origin [0,0,0]")
  
  # Check if theta divides 360 evenly
  n = int(round(2*np.pi/theta))
  if not np.isclose(2*np.pi, n * theta):
    raise ValueError(f"Angle {theta} degrees must divide 360 evenly")
  
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)
  
  # Create points for the polygon segment
  gmsh_points = []
  for pt in points:
    gmsh_points.append(gmsh.model.geo.addPoint(pt[0], pt[1], pt[2], h))
  
  # Create lines connecting points
  lines = []
  for i in range(len(gmsh_points)-1):
    lines.append(gmsh.model.geo.addLine(gmsh_points[i], gmsh_points[i+1]))
  # Close the polygon segment
  lines.append(gmsh.model.geo.addLine(gmsh_points[-1], gmsh_points[0]))
  
  # Create curve loop and surface
  c1 = gmsh.model.geo.addCurveLoop(lines)
  s1 = gmsh.model.geo.addPlaneSurface([c1])
  
  # Synchronize and generate initial mesh
  gmsh.model.geo.synchronize()
  gmsh.model.mesh.generate(3)
  
  # Get mesh data for rotation
  m = get_gmsh_entities()
  
  # Rotate n-1 times by theta
  for i in range(1, n):
    angle = i * theta  # Convert to radians 
    gmsh_transform_general(m, 1000*i, 1000000*i, 1000000*i, angle)
  
  # Remove duplicate nodes
  gmsh.model.mesh.removeDuplicateNodes()
  
  # Translate to final position if needed
  gmsh_translate(xc)

  # Check for hanging nodes before writing
  check_hanging_nodes()
  
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def cylindrical2d_wall_mesh(center=[0., 0., 0.], outer_radius=1.0, inner_radius=0.8, 
                          bar_width=0.2, bar_length=0.3, h=0.1, filename='mesh', vtk_out=False):
  """
  Create a cylindrical wall mesh with bars.
  
  Parameters:
  center - Center point coordinates [x, y, z]
  outer_radius - Outer radius of the wall
  inner_radius - Inner radius of the wall
  bar_width - Width of the bars
  bar_length - Length of the bars
  h - Mesh size
  filename - Output filename
  vtk_out - Whether to output VTK file
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)
  
  # Create points
  # Center point
  p1 = gmsh.model.geo.addPoint(center[0], center[1], center[2], h)
  
  # Outer circle points
  p2 = gmsh.model.geo.addPoint(center[0] + outer_radius, center[1], center[2], h)
  p3 = gmsh.model.geo.addPoint(center[0], center[1] + outer_radius, center[2], h)
  p4 = gmsh.model.geo.addPoint(center[0] - outer_radius, center[1], center[2], h)
  p5 = gmsh.model.geo.addPoint(center[0], center[1] - outer_radius, center[2], h)
  
  # Inner points with bar
  p6 = gmsh.model.geo.addPoint(center[0] + inner_radius, center[1] + 0.5*bar_width, center[2], h)
  p7 = gmsh.model.geo.addPoint(center[0] + inner_radius, center[1] - 0.5*bar_width, center[2], h)
  p8 = gmsh.model.geo.addPoint(center[0], center[1] + inner_radius, center[2], h)
  p9 = gmsh.model.geo.addPoint(center[0] - inner_radius, center[1], center[2], h)
  p10 = gmsh.model.geo.addPoint(center[0], center[1] - inner_radius, center[2], h)
  
  # Bar end points
  p11 = gmsh.model.geo.addPoint(center[0] + inner_radius - bar_length, center[1] + 0.5*bar_width, center[2], h)
  p12 = gmsh.model.geo.addPoint(center[0] + inner_radius - bar_length, center[1] - 0.5*bar_width, center[2], h)
  
  # Create circular arcs for outer circle
  c1 = gmsh.model.geo.addCircleArc(p2, p1, p3)
  c2 = gmsh.model.geo.addCircleArc(p3, p1, p4)
  c3 = gmsh.model.geo.addCircleArc(p4, p1, p5)
  c4 = gmsh.model.geo.addCircleArc(p5, p1, p2)
  
  # Create circular arcs for inner circle
  c5 = gmsh.model.geo.addCircleArc(p6, p1, p8)
  c6 = gmsh.model.geo.addCircleArc(p8, p1, p9)
  c7 = gmsh.model.geo.addCircleArc(p9, p1, p10)
  c8 = gmsh.model.geo.addCircleArc(p10, p1, p7)
  
  # Create lines for the bar
  l1 = gmsh.model.geo.addLine(p6, p11)
  l2 = gmsh.model.geo.addLine(p11, p12)
  l3 = gmsh.model.geo.addLine(p12, p7)
  
  # Create curve loops
  outer_loop = gmsh.model.geo.addCurveLoop([c1, c2, c3, c4])
  inner_loop = gmsh.model.geo.addCurveLoop([c5, c6, c7, c8, -l3, -l2, -l1])
  
  # Create surface with hole
  s1 = gmsh.model.geo.addPlaneSurface([outer_loop, inner_loop])
  
  # Synchronize geometry before adding physical groups
  gmsh.model.geo.synchronize()
  
  # Add physical group for the surface
  gmsh.model.addPhysicalGroup(2, [s1], 1)
  
  # Generate mesh
  gmsh.model.mesh.generate(2)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()
  
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
      gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def triangle_mesh_symmetric(xc=[0., 0., 0.], r=1., h=0.1, filename='mesh', vtk_out=False, symmetric_mesh=True):
  """
  Create a triangle mesh, either symmetric or full.
  
  Parameters:
  xc - center point coordinates [x, y, z]
  r - radius of the circumscribed circle
  h - mesh size
  filename - output filename
  vtk_out - whether to output VTK file
  symmetric_mesh - if True, creates 1/3 mesh and rotates it twice. If False, creates full triangle
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)
  
  if symmetric_mesh:
    # Create 1/3 triangle mesh at origin first, then rotate and translate
    xc_mesh = [0., 0., 0.]
    # Get reference triangle points
    x_tri = get_ref_triangle_points(xc_mesh, r)
    v1, v2, v3 = x_tri[0], x_tri[1], x_tri[2]
    
    # Create points for 1/3rd mesh (triangle from center to first two vertices)
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)  # Center
    p2 = gmsh.model.geo.addPoint(v1[0], v1[1], v1[2], h)  # First vertex
    p3 = gmsh.model.geo.addPoint(v2[0], v2[1], v2[2], h)  # Second vertex
    
    # Create lines
    l1 = gmsh.model.geo.addLine(p1, p2)  # Center to first vertex
    l2 = gmsh.model.geo.addLine(p2, p3)  # First to second vertex
    l3 = gmsh.model.geo.addLine(p3, p1)  # Second vertex back to center
    
    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop([l1, l2, l3])
    s1 = gmsh.model.geo.addPlaneSurface([c1])
    
    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)
    
    # Get mesh data for rotation
    m = get_gmsh_entities()
    
    # Rotate twice by 120 degrees to complete the triangle
    for i in range(2):
        angle = (i + 1) * 2*np.pi/3  # 120 degrees in radians
        gmsh_transform_general(m, 1000*(i+1), 1000000*(i+1), 1000000*(i+1), angle)
    
    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()
    
    # Translate to specified center
    gmsh_translate(xc)
      
  else:
    # For non-symmetric mesh, create full triangle directly
    x_tri = get_ref_triangle_points(xc, r)
    
    # Create points for the full triangle
    points = []
    points.append(gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h))  # Center point
    for v in x_tri:
        points.append(gmsh.model.geo.addPoint(v[0], v[1], v[2], h))
    
    # Create lines connecting vertices
    lines = []
    for i in range(2):
        lines.append(gmsh.model.geo.addLine(points[i+1], points[i+2]))
    lines.append(gmsh.model.geo.addLine(points[3], points[1]))
    
    # Create curve loop and surface
    c1 = gmsh.model.geo.addCurveLoop(lines)
    s1 = gmsh.model.geo.addPlaneSurface([c1])
    
    # Synchronize
    gmsh.model.geo.synchronize()
    
    # Embed center point in surface
    gmsh.model.mesh.embed(0, [points[0]], 2, s1)
    
    # Generate mesh
    gmsh.model.mesh.generate(3)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()
  
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def cuboid_mesh_symmetric(xc = [0., 0., 0.], Lx = 1., Ly = 1., Lz = 1., h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a cuboid mesh, either symmetric or full.
  xc - center point for symmetric mesh, or bottom-left-back corner for non-symmetric mesh
  Lx - length in x direction
  Ly - length in y direction
  Lz - length in z direction
  h - mesh size
  symmetric_mesh - if True, creates 1/8 mesh and mirrors it. If False, creates full cuboid
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/8th of cuboid at origin, then mirror it
    xc_mesh = [0., 0., 0.]
    
    # Create box for first octant
    box = gmsh.model.occ.addBox(xc_mesh[0], xc_mesh[1], xc_mesh[2], 
                                0.5*Lx, 0.5*Ly, 0.5*Lz)
            
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
    
    # Get mesh data for mirroring
    m = get_gmsh_entities()
    
    # Mirror the mesh to create full cuboid
    # First create the other octants in the positive z hemisphere
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # Mirror across yz plane
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # Mirror across xz plane
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # Mirror across z axis
    
    # Then mirror the entire upper hemisphere to create lower hemisphere
    gmsh_transform(m, 4000, 4000000, 4000000, 1, 1, -1)  # Mirror across xy plane
    gmsh_transform(m, 5000, 5000000, 5000000, -1, 1, -1)  # Mirror across yz plane
    gmsh_transform(m, 6000, 6000000, 6000000, 1, -1, -1)  # Mirror across xz plane
    gmsh_transform(m, 7000, 7000000, 7000000, -1, -1, -1)  # Mirror across z axis
    
    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()
    
    # Translate to specified center coordinates
    gmsh_translate(xc)
      
  else:
    # Create full cuboid directly using OpenCASCADE
    # Create box centered at xc
    box = gmsh.model.occ.addBox(xc[0] - 0.5*Lx, xc[1] - 0.5*Ly, xc[2] - 0.5*Lz,
                                Lx, Ly, Lz)
    
    # Add center point
    p1 = gmsh.model.occ.addPoint(xc[0], xc[1], xc[2], h)
    
    # Synchronize
    gmsh.model.occ.synchronize()
    
    # Embed center point in volume
    gmsh.model.mesh.embed(0, [p1], 3, box)
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def ellipsoid_mesh_symmetric(xc = [0., 0., 0.], rx = 1., ry = 0.5, rz = 0.3, h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create an ellipsoid mesh, either symmetric or full.
  xc - center point coordinates [x, y, z]
  rx - radius in x direction
  ry - radius in y direction
  rz - radius in z direction
  h - mesh size
  symmetric_mesh - if True, creates 1/8 mesh and mirrors it. If False, creates full ellipsoid
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/8th of ellipsoid at origin, then mirror it
    xc_mesh = [0., 0., 0.]
    
    # Create sphere of radius 1
    sphere = gmsh.model.occ.addSphere(xc_mesh[0], xc_mesh[1], xc_mesh[2], 1.0)
    
    # Scale it to create ellipsoid
    gmsh.model.occ.dilate([(3, sphere)], xc_mesh[0], xc_mesh[1], xc_mesh[2], rx, ry, rz)
    
    # Create cutting boxes for first octant
    cut_left = gmsh.model.occ.addBox(xc_mesh[0]-rx, xc_mesh[1]-ry, xc_mesh[2]-rz, rx, 2*ry, 2*rz)
    cut_back = gmsh.model.occ.addBox(xc_mesh[0]-rx, xc_mesh[1]-ry, xc_mesh[2]-rz, 2*rx, ry, 2*rz)
    cut_bottom = gmsh.model.occ.addBox(xc_mesh[0]-rx, xc_mesh[1]-ry, xc_mesh[2]-rz, 2*rx, 2*ry, rz)
    
    # Cut sphere to get first octant
    gmsh.model.occ.cut([(3,sphere)], [(3,cut_left), (3,cut_back), (3,cut_bottom)])
    
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
    
    # Get mesh data for mirroring
    m = get_gmsh_entities()
    
    # Mirror the mesh to create full ellipsoid
    # First create the other octants in the positive z hemisphere
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # Mirror across yz plane
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # Mirror across xz plane
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # Mirror across z axis
    
    # Then mirror the entire upper hemisphere to create lower hemisphere
    gmsh_transform(m, 4000, 4000000, 4000000, 1, 1, -1)  # Mirror across xy plane
    gmsh_transform(m, 5000, 5000000, 5000000, -1, 1, -1)  # Mirror across yz plane
    gmsh_transform(m, 6000, 6000000, 6000000, 1, -1, -1)  # Mirror across xz plane
    gmsh_transform(m, 7000, 7000000, 7000000, -1, -1, -1)  # Mirror across z axis
    
    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()
    
    # Translate to specified center coordinates
    gmsh_translate(xc)
      
  else:
    # Create full ellipsoid directly using OpenCASCADE
    # First create sphere of radius 1
    sphere = gmsh.model.occ.addSphere(xc[0], xc[1], xc[2], 1.0)
    
    # Scale it to create ellipsoid
    gmsh.model.occ.dilate([(3, sphere)], xc[0], xc[1], xc[2], rx, ry, rz)
    
    # Synchronize
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()
  
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def cylinder_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 1., mesh_size = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create a cylinder mesh, either symmetric or full.
  xc - center point coordinates [x, y, z] (center of bottom face)
  r - radius of cylinder
  h - height of cylinder
  mesh_size - mesh element size
  symmetric_mesh - if True, creates 1/8 mesh and mirrors it. If False, creates full cylinder
  """
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/8th of cylinder at origin, then mirror it
    xc_mesh = [0., 0., 0.]
    
    # Create full cylinder first
    cyl = gmsh.model.occ.addCylinder(xc_mesh[0], xc_mesh[1], xc_mesh[2], 
                                    0, 0, h,  # height vector in z direction
                                    r)
    
    # Create cutting boxes for first octant
    cut_left = gmsh.model.occ.addBox(xc_mesh[0]-r, xc_mesh[1]-r, xc_mesh[2], r, 2*r, h)
    cut_back = gmsh.model.occ.addBox(xc_mesh[0]-r, xc_mesh[1]-r, xc_mesh[2], 2*r, r, h)
    
    # Cut cylinder to get first quarter
    gmsh.model.occ.cut([(3,cyl)], [(3,cut_left), (3,cut_back)])
    
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), mesh_size)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
    
    # Get mesh data for mirroring
    m = get_gmsh_entities()
    
    # Mirror the mesh to create full cylinder
    # First create the other quarters
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # Mirror across yz plane
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # Mirror across xz plane
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # Mirror across z axis
    
    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()
    
    # Translate to specified center coordinates
    gmsh_translate(xc)
      
  else:
    # Create full cylinder directly using OpenCASCADE
    cyl = gmsh.model.occ.addCylinder(xc[0], xc[1], xc[2],  # base center
                                    0, 0, h,  # height vector in z direction
                                    r)  # radius
    
    # Add center points at top and bottom faces
    p1 = gmsh.model.occ.addPoint(xc[0], xc[1], xc[2], mesh_size)  # bottom center
    p2 = gmsh.model.occ.addPoint(xc[0], xc[1], xc[2] + h, mesh_size)  # top center
    
    # Synchronize
    gmsh.model.occ.synchronize()
    
    # Embed center points in respective faces
    # First get all surfaces
    surfaces = gmsh.model.getEntities(2)
    # Find top and bottom surfaces (they are parallel to xy-plane)
    for s in surfaces:
      com = gmsh.model.occ.getCenterOfMass(s[0], s[1])
      if abs(com[2] - xc[2]) < 1e-6:  # bottom face
        gmsh.model.mesh.embed(0, [p1], 2, s[1])
      elif abs(com[2] - (xc[2] + h)) < 1e-6:  # top face
        gmsh.model.mesh.embed(0, [p2], 2, s[1])
  
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), mesh_size)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)
  
  # Check for hanging nodes before writing
  check_hanging_nodes()
  
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def annulus_circle_mesh_symmetric(xc = [0., 0., 0.], r_outer = 1., r_inner = 0.5, h = 0.1, filename = 'mesh', vtk_out = False, symmetric_mesh = True):
  """
  Create an annulus (ring) mesh, either symmetric or full.
  xc - center point coordinates [x, y, z]
  r_outer - radius of outer circle
  r_inner - radius of inner circle
  h - mesh size
  symmetric_mesh - if True, creates 1/4 mesh and mirrors it. If False, creates full annulus
  """
  if r_inner >= r_outer:
    raise ValueError("Inner radius must be smaller than outer radius")

  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    xc_mesh = [0., 0., 0.]

    # add points for the quadrant of annulus circle
    p1 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h)
    p2 = gmsh.model.geo.addPoint(xc_mesh[0] + r_outer, xc_mesh[1], xc_mesh[2], h)
    p3 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + r_outer, xc_mesh[2], h)
    p4 = gmsh.model.geo.addPoint(xc_mesh[0] + r_inner, xc_mesh[1], xc_mesh[2], h)
    p5 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + r_inner, xc_mesh[2], h)

    l1 = gmsh.model.geo.addCircleArc(p2, p1, p3)
    l2 = gmsh.model.geo.addCircleArc(p4, p1, p5)
    l3 = gmsh.model.geo.addLine(p4, p2)
    l4 = gmsh.model.geo.addLine(p5, p3)

    c1 = gmsh.model.geo.addCurveLoop([l3, l1, -l4,  -l2])
    s1 = gmsh.model.geo.addPlaneSurface([c1])

    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(3)

    m = get_gmsh_entities()

    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1) # x-axis
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1) # y-axis
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1) # z-axis

    # remove the duplicate nodes that will have been created on the internal
    # boundaries
    gmsh.model.mesh.removeDuplicateNodes()

    # translate the mesh to the specified center coordinates
    gmsh_translate(xc)

  else:
    
    # Outer circle points
    p1 = gmsh.model.geo.addPoint(xc[0], xc[1], xc[2], h)
    p2 = gmsh.model.geo.addPoint(xc[0] + r_outer, xc[1], xc[2], h)
    p3 = gmsh.model.geo.addPoint(xc[0], xc[1] + r_outer, xc[2], h)
    p4 = gmsh.model.geo.addPoint(xc[0] - r_outer, xc[1], xc[2], h)
    p5 = gmsh.model.geo.addPoint(xc[0], xc[1] - r_outer, xc[2], h)

    # Inner circle points
    p6 = gmsh.model.geo.addPoint(xc[0] + r_inner, xc[1], xc[2], h)
    p7 = gmsh.model.geo.addPoint(xc[0], xc[1] + r_inner, xc[2], h)
    p8 = gmsh.model.geo.addPoint(xc[0] - r_inner, xc[1], xc[2], h)
    p9 = gmsh.model.geo.addPoint(xc[0], xc[1] - r_inner, xc[2], h)
    
    # Create circular arcs for outer circle
    c1 = gmsh.model.geo.addCircleArc(p2, p1, p3)
    c2 = gmsh.model.geo.addCircleArc(p3, p1, p4)
    c3 = gmsh.model.geo.addCircleArc(p4, p1, p5)
    c4 = gmsh.model.geo.addCircleArc(p5, p1, p2)
    
    # Create circular arcs for inner circle
    c5 = gmsh.model.geo.addCircleArc(p6, p1, p7)
    c6 = gmsh.model.geo.addCircleArc(p7, p1, p8)
    c7 = gmsh.model.geo.addCircleArc(p8, p1, p9)
    c8 = gmsh.model.geo.addCircleArc(p9, p1, p6)

    # Create curve loops
    outer_loop = gmsh.model.geo.addCurveLoop([c1, c2, c3, c4])
    inner_loop = gmsh.model.geo.addCurveLoop([c5, c6, c7, c8])

    #gmsh.fltk.run()
    
    # surface
    s1 = gmsh.model.geo.addPlaneSurface([outer_loop, inner_loop])

    # Synchronize geometry before adding physical groups
    gmsh.model.geo.synchronize()

    # Add physical group for the surface
    gmsh.model.addPhysicalGroup(2, [s1], 1)

    # Generate mesh
    gmsh.model.mesh.generate(2)

  # Check for hanging nodes before writing
  check_hanging_nodes()
    
  # Write output files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def annulus_rectangle_mesh(xc=[0., 0., 0.], Lx=1., Ly=1., hole_Lx=0.3, hole_Ly=0.3, h=0.1, filename='mesh', vtk_out=False, symmetric_mesh=True):
  """
  Create a rectangular mesh with a rectangular hole in the center (annulus rectangle).
  
  Parameters
  ----------
  xc : list
      Center coordinates [x, y, z]
  Lx : float
      Length of outer rectangle in x direction
  Ly : float
      Length of outer rectangle in y direction
  hole_Lx : float
      Length of inner hole in x direction
  hole_Ly : float
      Length of inner hole in y direction
  h : float
      Mesh size
  filename : str
      Output filename without extension
  vtk_out : bool
      If True, also writes a VTK file
  symmetric_mesh : bool
      If True, creates 1/4 mesh and mirrors it. If False, creates full mesh
  """
  # Initialize gmsh
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  if symmetric_mesh:
    # Create 1/4 mesh at origin first, then mirror and translate
    xc_mesh = [0., 0., 0.]

    # Create points for outer rectangle (1/4)
    p1 = gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*Lx, xc_mesh[1], xc_mesh[2], h)  
    p2 = gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*Lx, xc_mesh[1] + 0.5*Ly, xc_mesh[2], h)
    p3 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + 0.5*Ly, xc_mesh[2], h)
    p4 = gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*hole_Lx, xc_mesh[1], xc_mesh[2], h)
    p5 = gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*hole_Lx, xc_mesh[1] + 0.5*hole_Ly, xc_mesh[2], h)
    p6 = gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] + 0.5*hole_Ly, xc_mesh[2], h)

    # Create lines
    l1 = gmsh.model.geo.addLine(p1, p2)
    l2 = gmsh.model.geo.addLine(p2, p3)
    l3 = gmsh.model.geo.addLine(p3, p6)
    l4 = gmsh.model.geo.addLine(p6, p5)
    l5 = gmsh.model.geo.addLine(p5, p4)
    l6 = gmsh.model.geo.addLine(p4, p1)

    # Create curve loops
    cl = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4, l5, l6])

    # Create plane surface with hole
    s = gmsh.model.geo.addPlaneSurface([cl])

    # Synchronize and generate mesh
    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(2)

    # Get mesh data for mirroring
    m = get_gmsh_entities()

    # Mirror the mesh in all quadrants
    gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)  # Mirror across y-axis
    gmsh_transform(m, 2000, 2000000, 2000000, 1, -1, 1)  # Mirror across x-axis
    gmsh_transform(m, 3000, 3000000, 3000000, -1, -1, 1)  # Mirror across origin

    # Remove duplicate nodes
    gmsh.model.mesh.removeDuplicateNodes()

    # Translate to specified center coordinates
    gmsh_translate(xc)

  else:
    # Create points for outer rectangle
    p1 = gmsh.model.geo.addPoint(xc[0] - 0.5*Lx, xc[1] - 0.5*Ly, xc[2], h)  # Bottom left
    p2 = gmsh.model.geo.addPoint(xc[0] + 0.5*Lx, xc[1] - 0.5*Ly, xc[2], h)  # Bottom right
    p3 = gmsh.model.geo.addPoint(xc[0] + 0.5*Lx, xc[1] + 0.5*Ly, xc[2], h)  # Top right
    p4 = gmsh.model.geo.addPoint(xc[0] - 0.5*Lx, xc[1] + 0.5*Ly, xc[2], h)  # Top left

    # Create points for inner hole
    p5 = gmsh.model.geo.addPoint(xc[0] - 0.5*hole_Lx, xc[1] - 0.5*hole_Ly, xc[2], h)  # Bottom left
    p6 = gmsh.model.geo.addPoint(xc[0] + 0.5*hole_Lx, xc[1] - 0.5*hole_Ly, xc[2], h)  # Bottom right
    p7 = gmsh.model.geo.addPoint(xc[0] + 0.5*hole_Lx, xc[1] + 0.5*hole_Ly, xc[2], h)  # Top right
    p8 = gmsh.model.geo.addPoint(xc[0] - 0.5*hole_Lx, xc[1] + 0.5*hole_Ly, xc[2], h)  # Top left

    # Create lines for outer rectangle
    l1 = gmsh.model.geo.addLine(p1, p2)  # Bottom
    l2 = gmsh.model.geo.addLine(p2, p3)  # Right
    l3 = gmsh.model.geo.addLine(p3, p4)  # Top
    l4 = gmsh.model.geo.addLine(p4, p1)  # Left

    # Create lines for inner hole
    l5 = gmsh.model.geo.addLine(p5, p6)  # Bottom
    l6 = gmsh.model.geo.addLine(p6, p7)  # Right
    l7 = gmsh.model.geo.addLine(p7, p8)  # Top
    l8 = gmsh.model.geo.addLine(p8, p5)  # Left

    # Create curve loops
    outer_loop = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
    inner_loop = gmsh.model.geo.addCurveLoop([l5, l6, l7, l8])

    # Create plane surface with hole
    s1 = gmsh.model.geo.addPlaneSurface([outer_loop, inner_loop])

    # Synchronize and generate mesh
    gmsh.model.geo.synchronize()
    gmsh.model.mesh.generate(2)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  # Write mesh files
  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')

  # Finalize gmsh
  gmsh.finalize()

def open_rectangle_mesh(xc = [0., 0., 0.], Lx = 1., Ly = 1., hole_Lx = 0.5, hole_Ly = 0.5, h = 0.1, filename = 'mesh', vtk_out = False):
  """
  Create a rectangular mesh with a rectangular hole in the center (annulus rectangle).
  
  Parameters
  ----------
  xc : list
      Center coordinates [x, y, z]
  Lx : float
      Length of outer rectangle in x direction
  Ly : float
      Length of outer rectangle in y direction
  hole_Lx : float
      Length of inner hole in x direction
  hole_Ly : float
      Length of inner hole in y direction
  h : float
      Mesh size
  filename : str
      Output filename without extension
  vtk_out : bool
      If True, also writes a VTK file
  """
  # Initialize gmsh
  gmsh.initialize()
  gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)

  # Annulus rectangle with top removed
  # xc is the center of the cocentric rectangles
  # we mesh the right half of the annulus rectangle
  xc_mesh = [0., 0., 0.]
  points = []
  points.append(gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*Lx, xc_mesh[1] - 0.5*Ly, xc_mesh[2], h))
  points.append(gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*Lx, xc_mesh[1] + 0.5*Ly, xc_mesh[2], h))
  points.append(gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*hole_Lx, xc_mesh[1] + 0.5*Ly, xc_mesh[2], h))
  points.append(gmsh.model.geo.addPoint(xc_mesh[0] + 0.5*hole_Lx, xc_mesh[1] - 0.5*hole_Ly, xc_mesh[2], h))
  points.append(gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] - 0.5*hole_Ly, xc_mesh[2], h))
  points.append(gmsh.model.geo.addPoint(xc_mesh[0], xc_mesh[1] - 0.5*Ly, xc_mesh[2], h))

  lines = []
  for i in range(len(points) - 1):
    lines.append(gmsh.model.geo.addLine(points[i], points[i + 1]))
  lines.append(gmsh.model.geo.addLine(points[-1], points[0]))
  cl = gmsh.model.geo.addCurveLoop(lines)
  s = gmsh.model.geo.addPlaneSurface([cl])
  gmsh.model.geo.synchronize()
  gmsh.model.mesh.generate(2)

  m = get_gmsh_entities()

  # mirror the mesh across the x-axis
  gmsh_transform(m, 1000, 1000000, 1000000, -1, 1, 1)
  gmsh.model.mesh.removeDuplicateNodes()
  gmsh_translate(xc)

  # Check for hanging nodes before writing
  check_hanging_nodes()

  gmsh.write(filename + '.msh')
  if vtk_out:
    gmsh.write(filename + '.vtk')
  
  gmsh.finalize()

def open_pipe_mesh(xc=[0., 0., 0.], axis=[0., 0., 1.], length=2., outer_radius=1., wall_thickness=0.1, h=0.1, filename='mesh', vtk_out=False):
    """
    Create a 3D pipe mesh with specified axis, closed bottom and open top.
    The pipe is created by:
    1. Creating an outer cylinder
    2. Subtracting an inner cylinder to create walls
    3. Subtracting the top surface to create the opening
    
    Parameters
    ----------
    xc : list
        Center coordinates [x, y, z] of the base center
    axis : list
        Axis vector defining pipe orientation [ax, ay, az]
    length : float
        Length of the pipe along axis
    outer_radius : float
        Outer radius of the pipe
    wall_thickness : float
        Thickness of the pipe wall and bottom
    h : float
        Mesh size
    filename : str
        Output filename without extension
    vtk_out : bool
        If True, also writes a VTK file
    """
    gmsh.initialize()
    gmsh.option.setNumber("Mesh.MshFileVersion", 2.2)
    
    # Normalize axis vector
    axis_norm = np.sqrt(axis[0]**2 + axis[1]**2 + axis[2]**2)
    if axis_norm == 0:
        raise ValueError("Axis vector cannot be zero")
    axis = [x/axis_norm for x in axis]
    
    # Create outer cylinder
    l = length + wall_thickness
    outer_cylinder = gmsh.model.occ.addCylinder(xc[0], xc[1], xc[2],
                                               axis[0]*l, axis[1]*l, axis[2]*l,
                                               outer_radius)
    
    # Create inner cylinder (to be subtracted)
    inner_cylinder = gmsh.model.occ.addCylinder(xc[0]+wall_thickness*axis[0], xc[1]+wall_thickness*axis[1], xc[2]+wall_thickness*axis[2],
                                               axis[0]*l, axis[1]*l, axis[2]*l,
                                               outer_radius - wall_thickness)
    
    # Create a box slightly larger than the cylinder at the top to cut off the top surface
    # First create a box aligned with coordinate axes
    dx = 2 * outer_radius
    box = gmsh.model.occ.addBox(xc[0] - dx, xc[1] - dx, xc[2] + l - wall_thickness,
                               2*dx, 2*dx, 2*wall_thickness)
    
    # If axis is not aligned with z, rotate the box to align with axis
    if not (abs(axis[0]) < 1e-10 and abs(axis[1]) < 1e-10):
        # Calculate rotation angle and axis
        z_axis = [0, 0, 1]
        rotation_axis = np.cross(z_axis, axis)
        rotation_angle = np.arccos(np.dot(z_axis, axis))
        
        # Rotate the box around center point
        gmsh.model.occ.rotate([(3, box)], 
                             xc[0], xc[1], xc[2],
                             rotation_axis[0], rotation_axis[1], rotation_axis[2],
                             rotation_angle)
    
    # First subtract inner cylinder from outer cylinder
    pipe = gmsh.model.occ.cut([(3, outer_cylinder)], [(3, inner_cylinder), (3, box)])
        
    # Synchronize before meshing
    gmsh.model.occ.synchronize()
    
    # Set mesh size
    gmsh.model.mesh.setSize(gmsh.model.getEntities(0), h)
    
    # Generate 3D mesh
    gmsh.model.mesh.generate(3)

    # Check for hanging nodes before writing
    check_hanging_nodes()
    
    # Write mesh files
    gmsh.write(filename + '.msh')
    if vtk_out:
        gmsh.write(filename + '.vtk')
    
    gmsh.finalize()

##-------------------------------------------------------##
##-------------------------------------------------------##
if __name__ == "__main__":
  inp_dir = './'

  test_meshes = ['circle', 'ellipse', 'sphere', 'cuboid', 'ellipsoid', 'rectangle', \
                 'hexagon', 'drum2d', 'triangle', 'polygon', 'cylindrical2d_wall', \
                 'cylinder', 'annulus_circle', 'annulus_rectangle', 'open_rectangle', 'open_pipe']
  test_meshes = ['open_pipe']
  symm_flag = 0 # 0 - both, 1 - symmetric, 2 - non-symmetric
  
  for mesh in test_meshes:
    if mesh == 'circle':
      if symm_flag == 0 or symm_flag == 1:
        circle_mesh_symmetric(xc = [-3., -3., 0.], r = 1., h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True) 
      if symm_flag == 0 or symm_flag == 2:
        circle_mesh_symmetric(xc = [-3., -3., 0.], r = 1., h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'ellipse':
      if symm_flag == 0 or symm_flag == 1:
        ellipse_mesh_symmetric(xc = [-1., -1., 0.], rx = 1., ry = 0.5, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        ellipse_mesh_symmetric(xc = [-1., -1., 0.], rx = 1., ry = 0.5, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'sphere':
      if symm_flag == 0 or symm_flag == 1:
        sphere_mesh_symmetric(xc = [-5., -5., 0.], r = 1., h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        sphere_mesh_symmetric(xc = [-5., -5., 0.], r = 1., h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'rectangle':
      if symm_flag == 0 or symm_flag == 1:
        rectangle_mesh_symmetric(xc = [1., 1., 0.], Lx = 1., Ly = 1., h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)  
      if symm_flag == 0 or symm_flag == 2:
        rectangle_mesh_symmetric(xc = [1., 1., 0.], Lx = 1., Ly = 1., h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)  
    elif mesh == 'hexagon':
      if symm_flag == 0 or symm_flag == 1:
        hexagon_mesh_symmetric(xc = [3., 3., 0.], r = 1., h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)  
      if symm_flag == 0 or symm_flag == 2:
        hexagon_mesh_symmetric(xc = [3., 3., 0.], r = 1., h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'drum2d':
      if symm_flag == 0 or symm_flag == 1:
        drum2d_mesh_symmetric(xc = [5., 5., 0.], r = 1., width = 0.5, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)  
      if symm_flag == 0 or symm_flag == 2:
        drum2d_mesh_symmetric(xc = [5., 5., 0.], r = 1., width = 0.5, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'triangle':
      if symm_flag == 0 or symm_flag == 1:
        triangle_mesh_symmetric(xc = [7., 7., 0.], r = 1., h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        triangle_mesh_symmetric(xc = [7., 7., 0.], r = 1., h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'polygon':
      theta = np.pi/6.
      R, a = 1., 0.25
      v1 = [0., 0., 0.]
      v2 = [R*np.cos(0.5*theta), -R*np.sin(0.5*theta), 0.]
      v4 = [R*np.cos(0.5*theta), R*np.sin(0.5*theta), 0.]
      v3 = [R + a, 0., 0.]
      if symm_flag == 0 or symm_flag == 1:
        polygon_mesh_symmetric(points = [v1, v2, v3, v4], theta = theta, xc = [9., 9., 9.], h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        polygon_mesh_symmetric(points = [v1, v2, v3, v4], theta = theta, xc = [9., 9., 9.], h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'cylindrical2d_wall':
      cylindrical2d_wall_mesh(
      center=[0., 0., 0.],
      outer_radius=1.0,
      inner_radius=0.8,
      bar_width=0.2,
      bar_length=0.3,
      h=0.05,
          filename='./' + mesh + '_sym',
      vtk_out=True
      )
    elif mesh == 'cuboid':
      if symm_flag == 0 or symm_flag == 1:
        cuboid_mesh_symmetric(xc = [0., 0., 0.], Lx = 1., Ly = 0.5, Lz = 0.3, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        cuboid_mesh_symmetric(xc = [0., 0., 0.], Lx = 1., Ly = 0.5, Lz = 0.3, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'ellipsoid':
      if symm_flag == 0 or symm_flag == 1:
        ellipsoid_mesh_symmetric(xc = [0., 0., 0.], rx = 1., ry = 0.5, rz = 0.3, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        ellipsoid_mesh_symmetric(xc = [0., 0., 0.], rx = 1., ry = 0.5, rz = 0.3, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'cylinder':
      if symm_flag == 0 or symm_flag == 1:
        cylinder_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 2., mesh_size = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        cylinder_mesh_symmetric(xc = [0., 0., 0.], r = 1., h = 2., mesh_size = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'annulus_circle':
      if symm_flag == 0 or symm_flag == 1:
        annulus_circle_mesh_symmetric(xc = [0., 0., 0.], r_outer = 1., r_inner = 0.5, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        annulus_circle_mesh_symmetric(xc = [0., 0., 0.], r_outer = 1., r_inner = 0.5, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)  
    elif mesh == 'annulus_rectangle':
      if symm_flag == 0 or symm_flag == 1:
        annulus_rectangle_mesh(xc = [0., 0., 0.], Lx = 1., Ly = 0.8, hole_Lx = 0.2, hole_Ly = 0.4, h = 0.1, filename = './' + mesh + '_sym', vtk_out = True, symmetric_mesh = True)
      if symm_flag == 0 or symm_flag == 2:
        annulus_rectangle_mesh(xc = [0., 0., 0.], Lx = 1., Ly = 0.8, hole_Lx = 0.2, hole_Ly = 0.4, h = 0.1, filename = './' + mesh + '_non_sym', vtk_out = True, symmetric_mesh = False)
    elif mesh == 'open_rectangle':
      open_rectangle_mesh(xc = [0., 0., 0.], Lx = 1.2, Ly = 0.8, hole_Lx = 1., hole_Ly = 0.6, h = 0.1, filename = './' + mesh, vtk_out = True)
    elif mesh == 'open_pipe':
      open_pipe_mesh(xc=[0., 0., 0.], axis=[1., 1., 0.], length=2., outer_radius=0.5, 
                wall_thickness=0.2, h=0.1, filename='./' + mesh, vtk_out=True)
    else:
      print(f"Mesh {mesh} not found")
    