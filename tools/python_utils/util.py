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
  a = prefix + '[' + print_list(arg, '%d') + ']\n'
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