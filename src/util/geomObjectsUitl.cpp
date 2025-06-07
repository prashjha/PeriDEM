////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "geomObjectsUtil.h"
#include "function.h"
#include "geom.h"
#include "methods.h"
#include <vector>
#include <set>

namespace {
  std::string printErrMsg(const std::string &geom_type,
                          const std::vector<double> &params,
                          const std::vector<size_t> &num_params_needed) {

    std::ostringstream oss;

    oss <<  "Error: Number of parameters needed to create geometry = "
        << geom_type << " are "
        << util::io::printStr(num_params_needed, 0)
        << ". But the number of parameters provided are "
        << params.size()
        << " and the parameters are "
        << util::io::printStr(params, 0)
        << ". Exiting.\n";

    return oss.str();
  }
};

namespace util {
    std::vector<size_t> util::geometry::getNumParamsRequired(std::string geom_type) {

      if (geom_type == "line")
        return {1, 4, 6};
      else if (geom_type == "triangle")
        return {1, 4, 7};
      else if (geom_type == "square")
        return {1, 4, 6};
      else if (geom_type == "rectangle")
        return {2, 5, 6};
      else if (geom_type == "hexagon")
        return {1, 4, 7};
      else if (geom_type == "drum2d")
        return {2, 5, 8};
      else if (geom_type == "cube")
        return {1, 4, 6};
      else if (geom_type == "cuboid")
        return {3, 6};
      else if (geom_type == "circle")
        return {1, 4};
      else if (geom_type == "ellipse")
        return {2, 5};
      else if (geom_type == "sphere")
        return {1, 4};
      else if (geom_type == "cylinder")
        return {7, 8};
      else if (geom_type == "angled_rectangle")
        return {6};
      else if (geom_type == "angled_cuboid")
        return {6};
      else if (geom_type == "rectangle_minus_rectangle")
        return {12};
      else if (geom_type == "cuboid_minus_cuboid")
        return {12};
      else {
        std::cerr << "Error: Invalid geometry type: " << geom_type << std::endl;
        exit(1);
      }
    }

    bool
    util::geometry::isNumberOfParamForGeometryValid(size_t n, std::string geom_type) {

      return util::methods::isInList(n, getNumParamsRequired(geom_type));
    }

    bool util::geometry::isNumberOfParamForComplexGeometryValid(size_t n,
                                                                std::string geom_type,
                                                                std::vector<std::string> vec_type) {

      int num_params = 0;
      for (const auto &s: vec_type) {
        // only consider the biggest parameter set from the list
        auto nps = getNumParamsRequired(s);
        if (nps.size() > 0)
          num_params += nps[nps.size() - 1];
        else {
          std::cerr << "Error: Geometry type = " << s
                    << " has zero number of parameters required. \n";
          exit(EXIT_FAILURE);
        }
      }
      return n == num_params;
    }

    bool
    util::geometry::checkParamForGeometry(size_t n, std::string geom_type) {

      return !util::geometry::isNumberOfParamForGeometryValid(n, geom_type);
    }

    bool util::geometry::checkParamForComplexGeometry(size_t n,
                                                      std::string geom_type,
                                                      std::vector<std::string> vec_type) {

      return !util::geometry::checkParamForComplexGeometry(n, geom_type, vec_type);
    }



    void util::geometry::createGeomObjectOld(const std::string &type,
                                          const std::vector<double> &params,
                                          const std::vector<std::string> &vec_type,
                                          const std::vector<std::string> &vec_flag,
                                          std::shared_ptr<util::geometry::GeomObject> &obj,
                                          const size_t &dim,
                                          bool perform_check) {

      // for any of the objects below, issue error if number of parameters not
      // sufficient regardless of perform_check value
      std::vector<std::string> no_default_obj = {"cylinder", "complex",
                                                 "rectangle_minus_rectangle",
                                                 "cuboid_minus_cuboid"};

      bool check_passed; // true means check passed
      if (type != "complex")
        check_passed = isNumberOfParamForGeometryValid(params.size(), type);
      else
        check_passed = isNumberOfParamForComplexGeometryValid(params.size(), type,
                                                    vec_type);

      std::ostringstream oss;
      if (!check_passed) {
        oss << "Error: Data maybe invalid. Can not create geometrical object: "
            << type << " with params: " << util::io::printStr(params)
            << ", vec type: " << util::io::printStr(vec_type)
            << ", vec flag: " << util::io::printStr(vec_flag) << std::endl;
      }

      // issue error
      if (!check_passed) {
        if (perform_check || util::methods::isTagInList(type, no_default_obj)) {
          std::cerr << oss.str();
          exit(1);
        }
      }

      // create object
      if (type == "circle") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Circle>(
                  params[0], util::Point(params[1], params[2], params[3]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() < 1) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 1
                      << " parameters for creating circle. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(EXIT_FAILURE);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Circle>(params[0],
                                                         util::Point());
        } // if else check_failed
      } // if circle
      else if (type == "ellipse") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Ellipse>(params[0], params[1],
                                                         util::Point());
        } // if else check_failed
        else {
          // if check is failed check if we can use other constructor
          if (params.size() < 2) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 2
                      << " parameters for creating ellipse. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(EXIT_FAILURE);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Ellipse>(params[0], params[1],
                                                         util::Point());
        } // if else check_failed
      } // if ellipse
      else if (type == "rectangle") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Rectangle>(
                  params[0], params[1],
                  util::Point(params[2], params[3], params[4]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() != 6 or params.size() != 5) {
            // if params are not adequate
            std::cerr << "Error: need either 5 or 6"
                      << " parameters for creating Rectangle. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(EXIT_FAILURE);
          }

          // reached here it means we have adequate parameters
          if (params.size() == 6)
            obj = std::make_shared<util::geometry::Rectangle>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
          else if (params.size() == 5)
            obj = std::make_shared<util::geometry::Rectangle>(
                    params[0], params[1],
                    util::Point(params[2], params[3], params[4]));
        } // if else check_failed
      } // if rectangle
      else if (type == "square") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Square>(
                  params[0],
                  util::Point(params[2], params[3], params[4]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() != 6) {
            // if params are not adequate
            std::cerr << "Error: need " << 6
                      << " parameters for creating Square. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(EXIT_FAILURE);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Square>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
        } // if else check_failed
      } // if square
      else if (type == "triangle") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Triangle>(
                  params[0], util::Point(params[1], params[2], params[3]),
                  util::Point(params[4], params[5], params[6]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() != 4) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 4
                      << " parameters for creating triangle. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(1);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Triangle>(
                  params[0], util::Point(params[1], params[2], params[3]));
        }// if else check_failed
      }// if triangle
      else if (type == "hexagon") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Hexagon>(
                  params[0], util::Point(params[1], params[2], params[3]),
                  util::Point(params[4], params[5], params[6]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() != 4) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 4
                      << " parameters for creating hexagon. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(1);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Hexagon>(
                  params[0], util::Point(params[1], params[2], params[3]));
        }// if else check_failed
      }// if hexagon
      else if (type == "drum2d") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Drum2D>(
                  params[0], params[1],
                  util::Point(params[2], params[3], params[4]),
                  util::Point(params[5], params[6], params[7]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() < 5) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 5
                      << " parameters for creating drum2d. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(1);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Drum2D>(
                  params[0], params[1],
                  util::Point(params[2], params[3], params[4]));
        }// if else check_failed
      }// if drum2d
      else if (type == "sphere") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Sphere>(
                  params[0], util::Point(params[1], params[2], params[3]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() < 1) {
            // if params are not adequate
            std::cerr << "Error: need at least " << 1
                      << " parameters for creating sphere. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(1);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Sphere>(params[0],
                                                         util::Point());
        }// if else check_failed
      }// if sphere
      else if (type == "cuboid") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Cuboid>(
                  params[0], params[1], params[2],
                  util::Point(params[3], params[4], params[5]));
        } else {
          std::cerr << "Error: need at least " << 6
                    << " parameters for creating cuboid. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }// if else check_failed
      }// if cuboid
      else if (type == "cube") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Cube>(
                  params[0],
                  util::Point(params[2], params[3], params[4]));
        } else {
          // if check is failed check if we can use other constructor
          if (params.size() < 6) {
            // if params are not adequate
            std::cerr << "Error: need " << 6
                      << " parameters for creating Cube. "
                         "Number of params provided = "
                      << params.size()
                      << ", params = "
                      << util::io::printStr(params) << " \n";
            exit(EXIT_FAILURE);
          }

          // reached here it means we have adequate parameters
          obj = std::make_shared<util::geometry::Cube>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
        } // if else check_failed
      } // if cube
      else if (type == "cylinder") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<util::geometry::Cylinder>(
                  params[0], util::Point(params[1], params[2], params[3]),
                  util::Point(params[4], params[5], params[6]));
        } else {
          std::cerr << "Error: need at least " << 7
                    << " parameters for creating Cylinder. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }// if else check_failed
      }// if cylinder
      else if (type == "rectangle_minus_rectangle") {

        if (check_passed) {
          // if check is passed
          auto rin = new util::geometry::Rectangle(
                  util::Point(params[0], params[1],
                              params[2]),
                  util::Point(params[3], params[4], params[5]));
          auto rout = new util::geometry::Rectangle(
                  util::Point(params[6], params[7],
                              params[8]),
                  util::Point(params[9], params[10],
                              params[11]));

          obj = std::make_shared<util::geometry::AnnulusGeomObject>
                  (rin, rout, 2);
        } else {
          std::cerr << "Error: need at least " << 12
                    << " parameters for creating rectangle_minus_rectangle. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }// if else check_failed
      }// if rectangle_minus_rectangle
      else if (type == "cuboid_minus_cuboid") {

        if (check_passed) {
          // if check is passed
          auto rin = new util::geometry::Cuboid(
                  util::Point(params[0], params[1],
                              params[2]),
                  util::Point(params[3], params[4],
                              params[5]));
          auto rout = new util::geometry::Cuboid(
                  util::Point(params[6], params[7],
                              params[8]),
                  util::Point(params[9], params[10],
                              params[11]));

          obj = std::make_shared<util::geometry::AnnulusGeomObject>
                  (rin, rout, 3);
        } else {
          std::cerr << "Error: need at least " << 12
                    << " parameters for creating cuboid_minus_cuboid. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }// if else check_failed
      }// if cuboid_minus_cuboid
      else if (type == "complex") {

        if (check_passed) {
          // if check is passed
          std::vector<std::shared_ptr<util::geometry::GeomObject>> vec_obj(
                  vec_type.size());

          size_t param_start = 0;
          for (size_t i = 0; i < vec_type.size(); i++) {
            auto geom_type = vec_type[i];
            auto geom_flag = vec_flag[i];
            auto num_params = getNumParamsRequired(geom_type)[0];

            // get slice of full param vector
            auto p1 = params.begin() + param_start;
            auto p2 = params.begin() + param_start + num_params;
            auto geom_param = std::vector<double>(p1, p2);

            // create geom object
            createGeomObject(geom_type, geom_param, std::vector<std::string>(),
                             std::vector<std::string>(), vec_obj[i], dim);

            param_start += num_params;
          }

          // create complex geom object
          ///std::cout << "creating complex object\n";
          obj = std::make_shared<util::geometry::ComplexGeomObject>(vec_obj,
                                                                    vec_flag,
                                                                    dim);
          //obj->print();
        } else {
          std::cerr << "Error: Not enough parameters for creating complex. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }// if else check_failed
      }// if complex
    }

    void util::geometry::createGeomObject(const std::string &geom_type,
                                          const std::vector<double> &params,
                                          const std::vector<std::string> &vec_type,
                                          const std::vector<std::string> &vec_flag,
                                          std::shared_ptr<util::geometry::GeomObject> &obj,
                                          const size_t &dim,
                                          bool perform_check) {

      std::vector<size_t> num_params_needed;

      if (geom_type == "line") {

        num_params_needed = {1, 4, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Line>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Line>(params[0],
                                                           util::Point(
                                                                   params[1],
                                                                   params[2],
                                                                   params[3]));

              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Line>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Line
      else if (geom_type == "triangle") {

        num_params_needed = {1, 4, 7};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Triangle>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Triangle>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));

              return;
            } else if (n == 7) {
              obj = std::make_shared<util::geometry::Triangle>(
                      params[0],
                      util::Point(params[1],params[2],params[3]),
                      util::Point(params[4],params[5],params[6]));

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Triangle
      else if (geom_type == "square") {

        num_params_needed = {1, 4, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Square>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Square>(params[0],
                                                             util::Point(
                                                                     params[1],
                                                                     params[2],
                                                                     params[3]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Square>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Square
      else if (geom_type == "rectangle") {

        num_params_needed = {2, 5, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 2) {
              obj = std::make_shared<util::geometry::Rectangle>(params[0],
                                                                params[1]);
              return;
            } else if (n == 5) {
              obj = std::make_shared<util::geometry::Rectangle>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Rectangle>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Rectangle
      else if (geom_type == "hexagon") {

        num_params_needed = {1, 4, 7};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Hexagon>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Hexagon>(
                      params[0], util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 7) {
              obj = std::make_shared<util::geometry::Hexagon>(
                      params[0],
                      util::Point(params[1], params[2], params[3]),
                      util::Point(params[4], params[5], params[6]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Hexagon
      else if (geom_type == "drum2d") {

        num_params_needed = {2, 5, 8};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 2) {
              obj = std::make_shared<util::geometry::Drum2D>(
                      params[0], params[1]);
              return;
            } else if (n == 5) {
              obj = std::make_shared<util::geometry::Drum2D>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 8) {
              obj = std::make_shared<util::geometry::Drum2D>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]),
                      util::Point(params[5], params[6], params[7]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Drum2D
      else if (geom_type == "cube") {

        num_params_needed = {1, 4, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Cube>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Cube>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Cube>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Cube
      else if (geom_type == "cuboid") {

        num_params_needed = {3, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 3) {
              obj = std::make_shared<util::geometry::Cuboid>(
                      params[0], params[1], params[2]);
              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Cuboid>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Cuboid
      else if (geom_type == "circle") {

        num_params_needed = {1, 4};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Circle>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Circle>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Circle
      else if (geom_type == "ellipse") {

        num_params_needed = {2, 5, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 2) {
              obj = std::make_shared<util::geometry::Ellipse>(params[0], params[1]);
              return;
            } else if (n == 5) {
              obj = std::make_shared<util::geometry::Ellipse>(params[0], params[1],
                                                             util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<util::geometry::Ellipse>(params[0], params[1],
                                                             util::Point(params[2], params[3], params[4]),
                                                             params[5]);
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Ellipse
      else if (geom_type == "sphere") {

        num_params_needed = {1, 4};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<util::geometry::Sphere>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<util::geometry::Sphere>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Sphere
      else if (geom_type == "cylinder") {

        num_params_needed = {7, 8};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 7) {
              obj = std::make_shared<util::geometry::Cylinder>(
                      params[0],
                      util::Point(params[1], params[2], params[3]),
                      util::Point(params[4], params[5], params[6]));
              return;
            } else if (n == 8) {
              obj = std::make_shared<util::geometry::Cylinder>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]),
                      util::Point(params[5], params[6], params[7]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Cylinder
      else if (geom_type == "angled_rectangle") {

        num_params_needed = {6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 6) {
              obj = std::make_shared<util::geometry::Rectangle>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // angled_rectangle
      else if (geom_type == "angled_cuboid") {

        num_params_needed = {6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 6) {
              obj = std::make_shared<util::geometry::Cuboid>(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // angled_cuboid
      else if (geom_type == "rectangle_minus_rectangle") {

        num_params_needed = {12};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 12) {

              auto rin = new util::geometry::Rectangle(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              auto rout = new util::geometry::Rectangle(
                      util::Point(params[6], params[7], params[8]),
                      util::Point(params[9], params[10], params[11]));

              obj = std::make_shared<util::geometry::AnnulusGeomObject>
                      (rin, rout, 2);

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // rectangle_minus_rectangle
      else if (geom_type == "cuboid_minus_cuboid") {

        num_params_needed = {12};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 12) {

              auto rin = new util::geometry::Cuboid(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              auto rout = new util::geometry::Cuboid(
                      util::Point(params[6], params[7], params[8]),
                      util::Point(params[9], params[10], params[11]));

              obj = std::make_shared<util::geometry::AnnulusGeomObject>
                      (rin, rout, 3);

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // rectangle_minus_rectangle
      else if (geom_type == "complex") {

        /*
        std::cout << "vec_type = " << util::io::printStr(vec_type, 0)
                  << ", vec_flag = " << util::io::printStr(vec_flag, 0)
                  << "\n";
        */

        num_params_needed = {0};
        std::vector<size_t> params_level(vec_type.size());
        for (size_t i = 0; i < vec_type.size(); i++) {

          // only consider the biggest parameter set from the list
          auto nps = getNumParamsRequired(vec_type[i]);
          if (nps.size() > 0)
            params_level[i] = nps[nps.size() - 1];
          else {
            std::cerr << "Error: Geometry type = " << vec_type[i]
                      << " has zero number of parameters required. \n";
            exit(EXIT_FAILURE);
          }

          //std::cout << "Geom type = " << vec_type[i]
          //          << ", params required = " << params_level[i] << "\n";

          num_params_needed[0] += params_level[i];
        }

        std::vector<std::shared_ptr<util::geometry::GeomObject>> objs(vec_type.size());
        std::vector<std::string> obj_flags(vec_type.size());

        if (params.size() == num_params_needed[0]) {

          // loop over objects and create
          size_t param_start = 0;
          for (size_t i = 0; i < vec_type.size(); i++) {

            auto geom_type_temp = vec_type[i];
            auto geom_flag_temp = vec_flag[i];

            std::vector<std::string> vec_type_temp;
            std::vector<std::string> vec_flag_temp;

            // find what range of parameters we need to provide
            std::vector<double> params_temp;
            for (size_t j=0; j<params_level[i]; j++)
              params_temp.push_back(params[j + param_start]);

            // call this function recursively
            obj_flags[i] = vec_flag[i];
            util::geometry::createGeomObject(
                    geom_type_temp, params_temp, vec_type_temp, vec_flag_temp,
                    objs[i], dim, perform_check);

            param_start += params_level[i];
          } // loop over objects

          // now create a composite object
          obj = std::make_shared<util::geometry::ComplexGeomObject>(objs, obj_flags, dim);

          return;
        } // if params.size() == n
      }  // complex
      else {
        std::cerr << "Error: Invalid geometry type: " << geom_type << std::endl;
        exit(1);
      }


      std::cerr << printErrMsg(geom_type, params, num_params_needed);
      exit(1);
    }

    void util::geometry::createGeomObject(util::geometry::GeomData &geomData,
                                          const size_t &dim,
                                          bool perform_check) {

      createGeomObject(geomData.d_geomName, geomData.d_geomParams,
                       geomData.d_geomComplexInfo.first,
                       geomData.d_geomComplexInfo.second,
                       geomData.d_geom_p, dim, perform_check);
    }

    void util::geometry::GeomData::copyGeometry(util::geometry::GeomData &z, size_t dim) {
      z.d_geomName = d_geomName;
      z.d_geomParams = d_geomParams;
      z.d_geomComplexInfo = d_geomComplexInfo;

      if (d_geom_p->d_name == "null") {
        z.d_geom_p =
                std::make_shared<util::geometry::NullGeomObject>(
                        d_geom_p->d_description);
      } else if (d_geom_p->d_name.empty()) {
        z.d_geom_p =
                std::make_shared<util::geometry::GeomObject>(
                        d_geom_p->d_name, d_geom_p->d_description);
      } else {
        util::geometry::createGeomObject(z, dim);
      }
    }

    void util::geometry::GeomData::copyGeometry(std::string &name,
                                                std::vector<double> &params,
                                                std::pair<std::vector<std::string>, std::vector<std::string>> &complexInfo,
                                                std::shared_ptr<util::geometry::GeomObject> &geom,
                                                size_t dim) {
      name = d_geomName;
      params = d_geomParams;
      complexInfo = d_geomComplexInfo;

      if (d_geom_p->d_name == "null") {
        geom =
                std::make_shared<util::geometry::NullGeomObject>(
                        d_geom_p->d_description);
      } else if (d_geom_p->d_name.empty()) {
        geom =
                std::make_shared<util::geometry::GeomObject>(
                        d_geom_p->d_name, d_geom_p->d_description);
      } else {
        util::geometry::createGeomObject(name,
                                         params,
                                         complexInfo.first,
                                         complexInfo.second,
                                         geom, dim);
      }
    }

}// Utility functions