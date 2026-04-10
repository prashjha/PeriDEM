/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "mshReader.h"
#include "util/io.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "util/feElementDefs.h"

namespace {

void trim_inplace(std::string &s) {
  while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
    s.pop_back();
  size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
    ++i;
  if (i > 0)
    s.erase(0, i);
}

} // namespace

rw::reader::MshReader::MshReader(const std::string &filename)
    : d_filename(filename){};

void rw::reader::MshReader::readMesh(size_t dim,
                                     std::vector<util::Point> *nodes,
                                     size_t &element_type, size_t &num_elems,
                                     std::vector<size_t> *enc,
                                     std::vector<std::vector<size_t>> *nec,
                                     std::vector<double> *volumes, bool is_fd) {

  (void)is_fd;

  if (util::io::isFileEmpty(d_filename)) {
    std::cerr << "Error: Filename = " << d_filename <<
                 " in MshReader is either nonexistent or empty.\n";
    exit(EXIT_FAILURE);
  }

  // open file
  if (d_file) d_file.close();

  d_file.open(d_filename);

  if (!d_file) {
    std::cerr << "Error: Can not open file = " << d_filename + ".msh"
              << ".\n";
    exit(1);
  }

  int format = 0;
  int size = 0;
  double version = 1.0;

  // clear data
  nodes->clear();
  enc->clear();
  nec->clear();
  volumes->clear();

  // specify type of element to read
  if (dim != 2 and dim != 3) {
    std::cerr << "Error: MshReader currently only supports reading of "
                 "triangle/quadrangle elements in dimension 2 and tetragonal "
                 "elements in 3.\n";
    exit(1);
  }

  bool read_nodes = false;
  bool read_elements = false;

  std::string line;
  while (std::getline(d_file, line)) {
    trim_inplace(line);
    if (line.empty())
      continue;

    if (line == "$MeshFormat") {
      if (!std::getline(d_file, line)) {
        std::cerr << "Error: Unexpected end of file in MshReader (after $MeshFormat).\n";
        exit(EXIT_FAILURE);
      }
      trim_inplace(line);
      {
        std::istringstream iss(line);
        iss >> version >> format >> size;
      }
      if ((version != 2.0) && (version != 2.1) && (version != 2.2)) {
        std::cerr << "Error: Unknown .msh file version " << version << "\n";
        exit(1);
      }
      if (format) {
        std::cerr << "Error: Format of .msh is possibly binary which is not"
                     " supported currently.\n ";
        exit(1);
      }
    } else if (line == "$Nodes" || line == "$NOD" || line == "$NOE") {
      read_nodes = true;
      if (!std::getline(d_file, line)) {
        std::cerr << "Error: Unexpected end of file in MshReader (node count).\n";
        exit(EXIT_FAILURE);
      }
      trim_inplace(line);
      unsigned int num_nodes = 0;
      {
        std::istringstream ns(line);
        ns >> num_nodes;
      }
      nodes->resize(num_nodes);
      nec->resize(num_nodes);

      for (unsigned int i = 0; i < num_nodes; ++i) {
        if (!std::getline(d_file, line)) {
          std::cerr << "Error: Unexpected end of file in MshReader (reading nodes).\n";
          exit(EXIT_FAILURE);
        }
        trim_inplace(line);
        std::istringstream ls(line);
        unsigned int id;
        double x, y, z;
        ls >> id >> x >> y >> z;
        if (id < 1 || id > num_nodes) {
          std::cerr << "Error: MshReader: node id out of range in file " << d_filename << "\n";
          exit(EXIT_FAILURE);
        }
        // 2D: planar meshes use xy; some .msh exports leave z as denormal/garbage — ignore file z.
        if (dim == 2)
          (*nodes)[id - 1] = util::Point(x, y, 0.);
        else
          (*nodes)[id - 1] = util::Point(x, y, z);
      }
    } else if (line == "$Elements" || line == "$ELM") {
      read_elements = true;
      if (!std::getline(d_file, line)) {
        std::cerr << "Error: Unexpected end of file in MshReader (element count).\n";
        exit(EXIT_FAILURE);
      }
      trim_inplace(line);
      unsigned int num_elem = 0;
      {
        std::istringstream es(line);
        es >> num_elem;
      }

      size_t elem_counter = 0;
      bool found_tri = false;
      bool found_quad = false;

      for (unsigned int iel = 0; iel < num_elem; ++iel) {
        if (!std::getline(d_file, line)) {
          std::cerr << "Error: Unexpected end of file in MshReader (reading elements).\n";
          exit(EXIT_FAILURE);
        }
        trim_inplace(line);
        std::istringstream ls(line);
        unsigned int id;
        unsigned int type;
        unsigned int ntags;
        ls >> id >> type >> ntags;
        int tag = 0;
        for (unsigned int j = 0; j < ntags; j++)
          ls >> tag;

        bool read_this_element = false;
        unsigned int num_nodes_con = 0;

        if (type == util::msh_type_triangle and dim == 2) {
          read_this_element = true;
          found_tri = true;
          element_type = util::vtk_type_triangle;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_triangle];
        } else if (type == util::msh_type_quadrangle and dim == 2) {
          read_this_element = true;
          found_quad = true;
          element_type = util::vtk_type_quad;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_quadrangle];
        } else if (type == util::msh_type_tetrahedron and dim == 3) {
          read_this_element = true;
          element_type = util::vtk_type_tetra;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_tetrahedron];
        }

        if (read_this_element) {
          for (unsigned int i = 0; i < num_nodes_con; i++) {
            unsigned int node_id;
            ls >> node_id;
            enc->push_back(node_id - 1);
            (*nec)[node_id - 1].push_back(elem_counter);
          }
          elem_counter++;
        } else {
          unsigned int n_skip = 0;
          if (type < 16)
            n_skip = static_cast<unsigned int>(util::msh_map_element_to_num_nodes[type]);
          if (n_skip > 0) {
            unsigned int dummy;
            for (unsigned int i = 0; i < n_skip; i++)
              ls >> dummy;
          } else {
            unsigned int dummy;
            while (ls >> dummy) {
            }
          }
        }
      }

      if (found_quad and found_tri) {
        std::cerr << "Error: Check mesh file. It appears to have both "
                     "quadrangle elements and triangle elements. "
                     "Currently we only support one kind of elements.\n";
        exit(1);
      }

      num_elems = elem_counter;
      break;
    }
  }

  if (!read_nodes || !read_elements) {
    std::cerr << "Error: MshReader: incomplete .msh file (need $Nodes and $Elements): "
              << d_filename << "\n";
    exit(EXIT_FAILURE);
  }

  // close file
  d_file.close();
}

void rw::reader::MshReader::readNodes(std::vector<util::Point> *nodes) {

  if (util::io::isFileEmpty(d_filename)) {
    std::cerr << "Error: Filename = " << d_filename <<
              " in MshReader is either nonexistent or empty.\n";
    exit(EXIT_FAILURE);
  }

  // open file
  if (d_file) d_file.close();

  d_file.open(d_filename);

  if (!d_file) {
    std::cerr << "Error: Can not open file = " << d_filename + ".msh.\n";
    exit(1);
  }

  std::string line;
  int format = 0;
  int size = 0;
  double version = 1.0;

  // clear data
  nodes->clear();
  bool read_nodes = false;

  while (true) {
    std::getline(d_file, line);
    if (d_file) {
      // // read $MeshFormat block
      if (line.find("$MeshFormat") == static_cast<std::string::size_type>(0)) {
        d_file >> version >> format >> size;
        if ((version != 2.0) && (version != 2.1) && (version != 2.2)) {
          std::cerr << "Error: Unknown .msh file version " << version << "\n";
          exit(1);
        }

        // we only support reading of ascii format, so issue error if this
        // condition is not met
        if (format) {
          std::cerr << "Error: Format of .msh is possibly binary which is not"
                       " supported currently.\n ";
          exit(1);
        }
      }
      // read $Nodes block
      if (line.find("$NOD") == static_cast<std::string::size_type>(0) ||
          line.find("$NOE") == static_cast<std::string::size_type>(0) ||
          line.find("$Nodes") == static_cast<std::string::size_type>(0)) {
        read_nodes = true;
        unsigned int num_nodes = 0;
        d_file >> num_nodes;

        // allocate space
        nodes->resize(num_nodes);

        // read in the nodal coordinates and form points.
        double x, y, z;
        unsigned int id;

        // add the nodal coordinates to the d_file
        for (unsigned int i = 0; i < num_nodes; ++i) {
          d_file >> id >> x >> y >> z;
          (*nodes)[id - 1] = util::Point(x, y, z);
        }
        // read the $ENDNOD delimiter
        std::getline(d_file, line);
      }  // end of reading nodes
    }    // if d_file

    // If !d_file, check to see if EOF was set.  If so, break out
    // of while loop.
    if (d_file.eof()) break;

    if (read_nodes) break;

    // If !d_file and !d_file.eof(), stream is in a bad state!
    // std::cerr<<"Error: Stream is bad! Perhaps the file does not exist?\n";
    // exit(1);
  }  // while true

  // close file
  d_file.close();
}

void rw::reader::MshReader::readCells(size_t dim, size_t &element_type,
        size_t &num_elems, std::vector<size_t> *enc,
        std::vector<std::vector<size_t>> *nec) {

  if (util::io::isFileEmpty(d_filename)) {
    std::cerr << "Error: Filename = " << d_filename <<
              " in MshReader is either nonexistent or empty.\n";
    exit(EXIT_FAILURE);
  }

  // open file
  if (d_file) d_file.close();

  d_file.open(d_filename);

  if (!d_file) {
    std::cerr << "Error: Can not open file = " << d_filename + ".msh"
              << ".\n";
    exit(1);
  }

  // specify type of element to read
  if (dim != 2 and dim != 3) {
    std::cerr << "Error: MshReader currently only supports reading of "
                 "triangle/quadrangle elements in dimension 2 and tetragonal "
                 "elements in 3.\n";
    exit(1);
  }

  enc->clear();
  nec->clear();

  bool have_nodes = false;
  bool read_elements = false;

  std::string line;
  while (std::getline(d_file, line)) {
    trim_inplace(line);
    if (line.empty())
      continue;

    if (!have_nodes && (line == "$Nodes" || line == "$NOD" || line == "$NOE")) {
      if (!std::getline(d_file, line)) {
        std::cerr << "Error: Unexpected end of file in MshReader (readCells node count).\n";
        exit(EXIT_FAILURE);
      }
      trim_inplace(line);
      unsigned int num_nodes = 0;
      {
        std::istringstream ns(line);
        ns >> num_nodes;
      }
      for (unsigned int i = 0; i < num_nodes; ++i) {
        if (!std::getline(d_file, line)) {
          std::cerr << "Error: Unexpected end of file in MshReader (readCells skip nodes).\n";
          exit(EXIT_FAILURE);
        }
      }
      nec->assign(num_nodes, {});
      have_nodes = true;
      continue;
    }

    if (have_nodes && (line == "$Elements" || line == "$ELM")) {
      read_elements = true;
      if (!std::getline(d_file, line)) {
        std::cerr << "Error: Unexpected end of file in MshReader (readCells element count).\n";
        exit(EXIT_FAILURE);
      }
      trim_inplace(line);
      unsigned int num_elem = 0;
      {
        std::istringstream es(line);
        es >> num_elem;
      }

      size_t elem_counter = 0;
      bool found_tri = false;
      bool found_quad = false;

      for (unsigned int iel = 0; iel < num_elem; ++iel) {
        if (!std::getline(d_file, line)) {
          std::cerr << "Error: Unexpected end of file in MshReader (readCells elements).\n";
          exit(EXIT_FAILURE);
        }
        trim_inplace(line);
        std::istringstream ls(line);
        unsigned int id;
        unsigned int type;
        unsigned int ntags;
        ls >> id >> type >> ntags;
        int tag = 0;
        for (unsigned int j = 0; j < ntags; j++)
          ls >> tag;

        bool read_this_element = false;
        unsigned int num_nodes_con = 0;

        if (type == util::msh_type_triangle and dim == 2) {
          read_this_element = true;
          found_tri = true;
          element_type = util::vtk_type_triangle;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_triangle];
        } else if (type == util::msh_type_quadrangle and dim == 2) {
          read_this_element = true;
          found_quad = true;
          element_type = util::vtk_type_quad;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_quadrangle];
        } else if (type == util::msh_type_tetrahedron and dim == 3) {
          read_this_element = true;
          element_type = util::vtk_type_tetra;
          num_nodes_con =
              util::msh_map_element_to_num_nodes[util::msh_type_tetrahedron];
        }

        if (read_this_element) {
          for (unsigned int i = 0; i < num_nodes_con; i++) {
            unsigned int node_id;
            ls >> node_id;
            enc->push_back(node_id - 1);
            (*nec)[node_id - 1].push_back(elem_counter);
          }
          elem_counter++;
        } else {
          unsigned int n_skip = 0;
          if (type < 16)
            n_skip = static_cast<unsigned int>(util::msh_map_element_to_num_nodes[type]);
          if (n_skip > 0) {
            unsigned int dummy;
            for (unsigned int i = 0; i < n_skip; i++)
              ls >> dummy;
          } else {
            unsigned int dummy;
            while (ls >> dummy) {
            }
          }
        }
      }

      if (found_quad and found_tri) {
        std::cerr << "Error: Check mesh file. It appears to have both "
                     "quadrangle elements and triangle elements. "
                     "Currently we only support one kind of elements.\n";
        exit(1);
      }

      num_elems = elem_counter;
      break;
    }
  }

  if (!have_nodes || !read_elements) {
    std::cerr << "Error: MshReader::readCells: incomplete .msh file: " << d_filename << "\n";
    exit(EXIT_FAILURE);
  }

  // close file
  d_file.close();
}

bool rw::reader::MshReader::readPointData(const std::string &name,
                                          std::vector<util::Point> *data) {
  // open file
  if (!d_file) d_file = std::ifstream(d_filename);

  if (!d_file)
    if (!d_file) {
      std::cerr << "Error: Can not open file = " << d_filename + ".msh"
                << ".\n";
      exit(1);
    }

  bool found_data = false;
  std::string line;
  while (true) {
    std::getline(d_file, line);
    if (d_file) {
      // read $Nodes block
      if (line.find("$NodeData") == static_cast<std::string::size_type>(0)) {
        // get name of data
        int num_tags = 0;
        d_file >> num_tags;
        std::string tag[num_tags];
        for (size_t i = 0; i < num_tags; i++) d_file >> tag[i];

        // read dummy data
        d_file >> num_tags;
        double real_tag = 0.;
        d_file >> real_tag;

        int tag_number = 0;
        int field_type = 0;
        int num_data = 0;
        d_file >> tag_number >> field_type >> num_data;

        // check we found the data
        if (tag[0] == name) {
          // check if data is of desired field type
          if (field_type != 3) {
            std::cerr << "Error: Data " << tag[0] << " is of type "
                      << field_type << " but we expect it to be of type " << 3
                      << ".\n";
            exit(1);
          }

          found_data = true;
          data->resize(num_data);
        }

        // we read through the data irrespective of we found it or not
        for (size_t i = 0; i < num_data; i++) {
          double d[field_type];
          for (size_t j = 0; j < field_type; j++) d_file >> d[j];

          if (found_data) (*data)[i] = util::Point(d[0], d[1], d[2]);
        }
        // read the end of data block
        std::getline(d_file, line);
      }  // end of reading nodes
    }    // if d_file

    if (found_data) break;

    // If !d_file, check to see if EOF was set.  If so, break out
    // of while loop.
    if (d_file.eof()) break;
  }  // while true

  d_file.close();
  return found_data;
}

bool rw::reader::MshReader::readPointData(const std::string &name,
                                          std::vector<double> *data) {
  // open file
  if (!d_file) d_file = std::ifstream(d_filename);

  if (!d_file)
    if (!d_file) {
      std::cerr << "Error: Can not open file = " << d_filename + ".msh"
                << ".\n";
      exit(1);
    }

  bool found_data = false;
  std::string line;
  while (true) {
    std::getline(d_file, line);
    if (d_file) {
      // read $Nodes block
      if (line.find("$NodeData") == static_cast<std::string::size_type>(0)) {
        // get name of data
        int num_tags = 0;
        d_file >> num_tags;
        std::string tag[num_tags];
        for (size_t i = 0; i < num_tags; i++) d_file >> tag[i];

        // read dummy data
        d_file >> num_tags;
        double real_tag = 0.;
        d_file >> real_tag;

        int tag_number = 0;
        int field_type = 0;
        int num_data = 0;
        d_file >> tag_number >> field_type >> num_data;

        // check we found the data
        if (tag[0] == name) {
          // check if data is of desired field type
          if (field_type != 1) {
            std::cerr << "Error: Data " << tag[0] << " is of type "
                      << field_type << " but we expect it to be of type " << 1
                      << ".\n";
            exit(1);
          }

          found_data = true;
          data->resize(num_data);
        }

        // we read through the data irrespective of we found it or not
        for (size_t i = 0; i < num_data; i++) {
          double d[field_type];
          for (size_t j = 0; j < field_type; j++) d_file >> d[j];

          if (found_data) (*data)[i] = d[0];
        }
        // read the end of data block
        std::getline(d_file, line);
      }  // end of reading nodes
    }    // if d_file

    if (found_data) break;

    // If !d_file, check to see if EOF was set.  If so, break out
    // of while loop.
    if (d_file.eof()) break;
  }  // while true

  d_file.close();
  return found_data;
}

void rw::reader::MshReader::close() {}
