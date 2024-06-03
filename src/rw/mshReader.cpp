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

#include <fstream>
#include <iostream>

#include "util/feElementDefs.h"

rw::reader::MshReader::MshReader(const std::string &filename)
    : d_filename(filename){};

void rw::reader::MshReader::readMesh(size_t dim,
                                     std::vector<util::Point> *nodes,
                                     size_t &element_type, size_t &num_elems,
                                     std::vector<size_t> *enc,
                                     std::vector<std::vector<size_t>> *nec,
                                     std::vector<double> *volumes, bool is_fd) {
  std::ifstream filein(d_filename);

  // open file
  if (!d_file) d_file.open(d_filename);

  if (!d_file) {
    std::cerr << "Error: Can not open file = " << d_filename + ".msh"
              << ".\n";
    exit(1);
  }

  std::string line;
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

  unsigned int num_nodes_con = 0;
  bool read_nodes = false;
  bool read_elements = false;

  while (true) {
    std::getline(filein, line);
    if (filein) {
      // // read $MeshFormat block
      if (line.find("$MeshFormat") == static_cast<std::string::size_type>(0)) {
        filein >> version >> format >> size;
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
        filein >> num_nodes;

        // allocate space
        nodes->resize(num_nodes);
        nec->resize(num_nodes);

        // read in the nodal coordinates and form points.
        double x, y, z;
        unsigned int id;

        // add the nodal coordinates to the filein
        for (unsigned int i = 0; i < num_nodes; ++i) {
          filein >> id >> x >> y >> z;
          (*nodes)[id - 1] = util::Point(x, y, z);
        }
        // read the $ENDNOD delimiter
        std::getline(filein, line);
      }  // end of reading nodes
        // Read the element block
      else if (line.find("$ELM") == static_cast<std::string::size_type>(0) ||
               line.find("$Elements") ==
               static_cast<std::string::size_type>(0)) {
        read_elements = true;

        // For reading the number of elements and the node ids from the stream
        unsigned int num_elem = 0;
        unsigned int node_id = 0;

        // read how many elements are there
        // this includes point element, line element also
        filein >> num_elem;

        // As of version 2.2, the format for each element line is:
        // elm-number elm-type number-of-tags < tag > ... node-number-list

        // read the elements
        size_t elem_counter = 0;
        bool found_tri = false;
        bool found_quad = false;
        bool found_tet = false;
        for (unsigned int iel = 0; iel < num_elem; ++iel) {
          unsigned int id;
          unsigned int type;
          unsigned int ntags;
          int tag;
          filein >> id >> type >> ntags;
          for (unsigned int j = 0; j < ntags; j++) filein >> tag;

          // we will read only those elements which we support
          bool read_this_element = false;

          // read element type we desire and for other element type
          // perform dummy read
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
            found_tet = true;
            element_type = util::vtk_type_tetra;
            num_nodes_con =
                util::msh_map_element_to_num_nodes[util::msh_type_tetrahedron];
          }

          // std::vector<size_t> e_nodes;
          if (read_this_element) {
            // read vertex of this element
            for (unsigned int i = 0; i < num_nodes_con; i++) {
              filein >> node_id;
              // add to the element-node connectivity
              // substract 1 to correct the numbering convention
              enc->push_back(node_id - 1);

              // e_nodes.push_back(node_id - 1);

              // fill the node-element connectivity table
              (*nec)[node_id - 1].push_back(elem_counter);
            }

            // debug
            //            std::cout << "(" << id << ", " << type << ", " << elem_counter
            //                      << ") = ";
            //            for (auto enode: e_nodes)
            //              std::cout << enode << ";";
            //            std::cout << "\n";

            // increment the element counter
            elem_counter++;
          } else {
            // these are the type of elements we need to ignore.
            size_t n = util::msh_map_element_to_num_nodes[type];
            // dummy read
            for (unsigned int i = 0; i < n; i++) filein >> node_id;
          }
        }  // element loop

        // check if mesh contains both triangle and quadrangle elements
        if (found_quad and found_tri) {
          std::cerr << "Error: Check mesh file. It appears to have both "
                       "quadrangle elements and triangle elements. "
                       "Currently we only support one kind of elements.\n";
          exit(1);
        }

        // write the number of elements
        num_elems = elem_counter;

        // read the $ENDELM delimiter
        std::getline(filein, line);
      }  // end of reading elements
    }    // if filein

    // If !filein, check to see if EOF was set.  If so, break out
    // of while loop.
    if (filein.eof()) break;

    if (read_nodes and read_elements) break;

    // If !filein and !filein.eof(), stream is in a bad state!
    // std::cerr<<"Error: Stream is bad! Perhaps the file does not exist?\n";
    // exit(1);
  }  // while true

  // close file
  filein.close();
}

void rw::reader::MshReader::readNodes(std::vector<util::Point> *nodes) {
  // open file
  if (!d_file) d_file = std::ifstream(d_filename);

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
