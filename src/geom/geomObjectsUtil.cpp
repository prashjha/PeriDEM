////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "geomObjects.h"
#include "complexGeomObjects.h"
#include "openRectChannel2D.h"
#include "geomObjectsUtil.h"
#include <iostream>
#include "geomUtilFunctions.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/io.h"
#include "util/json.h"
#include <set>
#include <stdexcept>
#include <vector>

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

namespace geom {
    std::vector<size_t> getNumParamsRequired(std::string geom_type) {

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
      else if (geom_type == "sphere")
        return {1, 4};
      else if (geom_type == "ellipse")
        return {6};
      else if (geom_type == "ellipsoid")
        return {6, 10};
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
      else if (geom_type == "circle_minus_circle")
        return {5};
      else if (geom_type == "sphere_minus_sphere")
        return {5};
      else if (geom_type == "open_rect_channel_2d")
        return {6};
      else {
        std::cerr << "Error: Invalid geometry type: " << geom_type << std::endl;
        exit(1);
      }
    }

    bool
    isNumberOfParamForGeometryValid(size_t n, std::string geom_type) {

      return util::methods::isInList(n, getNumParamsRequired(geom_type));
    }

    bool isNumberOfParamForComplexGeometryValid(size_t n,
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
    checkParamForGeometry(size_t n, std::string geom_type) {

      return !isNumberOfParamForGeometryValid(n, geom_type);
    }

    bool checkParamForComplexGeometry(size_t n,
                                                      std::string geom_type,
                                                      std::vector<std::string> vec_type) {

      return !isNumberOfParamForComplexGeometryValid(n, geom_type, vec_type);
    }



    void createGeomObjectOld(const std::string &type,
                                          const std::vector<double> &params,
                                          const std::vector<std::string> &vec_type,
                                          const std::vector<std::string> &vec_flag,
                                          std::shared_ptr<geom::GeomObject> &obj,
                                          bool perform_check) {

      // for any of the objects below, issue error if number of parameters not
      // sufficient regardless of perform_check value
      std::vector<std::string> no_default_obj = {"cylinder", "complex",
                                                 "rectangle_minus_rectangle",
                                                 "cuboid_minus_cuboid",
                                                 "circle_minus_circle",
                                                 "sphere_minus_sphere",
                                                 "open_rect_channel_2d"};

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
          obj = std::make_shared<geom::Circle>(
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
          obj = std::make_shared<geom::Circle>(params[0],
                                                         util::Point());
        } // if else check_failed
      } // if circle
      else if (type == "rectangle") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Rectangle>(
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
            obj = std::make_shared<geom::Rectangle>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
          else if (params.size() == 5)
            obj = std::make_shared<geom::Rectangle>(
                    params[0], params[1],
                    util::Point(params[2], params[3], params[4]));
        } // if else check_failed
      } // if rectangle
      else if (type == "square") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Square>(
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
          obj = std::make_shared<geom::Square>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
        } // if else check_failed
      } // if square
      else if (type == "triangle") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Triangle>(
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
          obj = std::make_shared<geom::Triangle>(
                  params[0], util::Point(params[1], params[2], params[3]));
        }// if else check_failed
      }// if triangle
      else if (type == "hexagon") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Hexagon>(
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
          obj = std::make_shared<geom::Hexagon>(
                  params[0], util::Point(params[1], params[2], params[3]));
        }// if else check_failed
      }// if hexagon
      else if (type == "drum2d") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Drum2D>(
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
          obj = std::make_shared<geom::Drum2D>(
                  params[0], params[1],
                  util::Point(params[2], params[3], params[4]));
        }// if else check_failed
      }// if drum2d
      else if (type == "sphere") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Sphere>(
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
          obj = std::make_shared<geom::Sphere>(params[0],
                                                         util::Point());
        }// if else check_failed
      }// if sphere
      else if (type == "ellipse") {

        if (check_passed) {
          obj = std::make_shared<geom::Ellipse>(
                  params[0], params[1], params[2],
                  util::Point(params[3], params[4], params[5]));
        } else {
          std::cerr << "Error: need at least " << 6
                    << " parameters for creating ellipse (a, b, theta, cx, cy, cz). "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }
      } // ellipse
      else if (type == "ellipsoid") {

        if (check_passed) {
          if (params.size() == 6) {
            obj = std::make_shared<geom::Ellipsoid>(
                    params[0], params[1], params[2], 
                    util::Point(params[4], params[5], params[6]));
          } else {
            obj = std::make_shared<geom::Ellipsoid>(
                    params[0], params[1], params[2], params[3],
                    util::Point(params[4], params[5], params[6]),
                    util::Point(params[7], params[8], params[9]));
          }
        } else {
          std::cerr << "Error: need 6 parameters (cx, cy, cz, r1, r2, r3) or 10 parameters "
                       "(cx, cy, cz, r1, r2, r3, ax, ay, az, theta) for creating ellipsoid. "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }
      } // ellipsoid
      else if (type == "cuboid") {

        if (check_passed) {
          // if check is passed
          obj = std::make_shared<geom::Cuboid>(
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
          obj = std::make_shared<geom::Cube>(
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
          obj = std::make_shared<geom::Cube>(
                  util::Point(params[0], params[1], params[2]),
                  util::Point(params[3], params[4], params[5]));
        } // if else check_failed
      } // if cube
      else if (type == "cylinder") {

        if (check_passed) {
          // Seven params: r, center of beginning cross-section, vector from beginning to end of cylinder
          obj = std::make_shared<geom::Cylinder>(
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
          auto rin = new geom::Rectangle(
                  util::Point(params[0], params[1],
                              params[2]),
                  util::Point(params[3], params[4], params[5]));
          auto rout = new geom::Rectangle(
                  util::Point(params[6], params[7],
                              params[8]),
                  util::Point(params[9], params[10],
                              params[11]));

          obj = std::make_shared<geom::AnnulusGeomObject>
                  (rin, rout);
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
          auto rin = new geom::Cuboid(
                  util::Point(params[0], params[1],
                              params[2]),
                  util::Point(params[3], params[4],
                              params[5]));
          auto rout = new geom::Cuboid(
                  util::Point(params[6], params[7],
                              params[8]),
                  util::Point(params[9], params[10],
                              params[11]));

          obj = std::make_shared<geom::AnnulusGeomObject>
                  (rin, rout);
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
      else if (type == "circle_minus_circle") {

        if (check_passed) {
          const double cx = params[0];
          const double cy = params[1];
          const double cz = params[2];
          const double r_outer = params[3];
          const double r_inner = params[4];
          if (r_inner <= 0. || r_outer <= r_inner)
            throw std::runtime_error(
                    "circle_minus_circle: require 0 < r_inner < r_outer (params: cx,cy,cz,r_outer,r_inner).");
          auto *cin = new geom::Circle(r_inner, util::Point(cx, cy, cz));
          auto *cout = new geom::Circle(r_outer, util::Point(cx, cy, cz));
          obj = std::make_shared<geom::AnnulusGeomObject>(cin, cout);
        } else {
          std::cerr << "Error: need " << 5
                    << " parameters for creating circle_minus_circle (cx, cy, cz, r_outer, r_inner). "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }
      } // circle_minus_circle
      else if (type == "sphere_minus_sphere") {

        if (check_passed) {
          const double cx = params[0];
          const double cy = params[1];
          const double cz = params[2];
          const double r_outer = params[3];
          const double r_inner = params[4];
          if (r_inner <= 0. || r_outer <= r_inner)
            throw std::runtime_error(
                    "sphere_minus_sphere: require 0 < r_inner < r_outer (params: cx,cy,cz,r_outer,r_inner).");
          auto *cin = new geom::Sphere(r_inner, util::Point(cx, cy, cz));
          auto *cout = new geom::Sphere(r_outer, util::Point(cx, cy, cz));
          obj = std::make_shared<geom::AnnulusGeomObject>(cin, cout);
        } else {
          std::cerr << "Error: need " << 5
                    << " parameters for creating sphere_minus_sphere (cx, cy, cz, r_outer, r_inner). "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }
      } // sphere_minus_sphere
      else if (type == "open_rect_channel_2d") {

        if (check_passed) {
          obj = std::make_shared<geom::OpenRectChannel2D>(
                  params[0], params[1], params[2], params[3], params[4], params[5]);
        } else {
          std::cerr << "Error: need " << 6
                    << " parameters for open_rect_channel_2d (x0,y0,x1,y1,t,z). "
                       "Number of params provided = "
                    << params.size()
                    << ", params = "
                    << util::io::printStr(params) << " \n";
          exit(1);
        }
      } // open_rect_channel_2d
      else if (type == "complex") {

        if (check_passed) {
          // if check is passed
          std::vector<std::shared_ptr<geom::GeomObject>> vec_obj(
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
                             std::vector<std::string>(), vec_obj[i]);

            param_start += num_params;
          }

          // create complex geom object
          ///std::cout << "creating complex object\n";
          obj = std::make_shared<geom::ComplexGeomObject>(vec_obj,
                                                                    vec_flag);
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

    void createGeomObject(const std::string &geom_type,
                                          const std::vector<double> &params,
                                          const std::vector<std::string> &vec_type,
                                          const std::vector<std::string> &vec_flag,
                                          std::shared_ptr<geom::GeomObject> &obj,
                                          bool perform_check) {

      std::vector<size_t> num_params_needed;

      if (geom_type == "line") {

        num_params_needed = {1, 4, 6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<geom::Line>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Line>(params[0],
                                                           util::Point(
                                                                   params[1],
                                                                   params[2],
                                                                   params[3]));

              return;
            } else if (n == 6) {
              obj = std::make_shared<geom::Line>(
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
              obj = std::make_shared<geom::Triangle>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Triangle>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));

              return;
            } else if (n == 7) {
              obj = std::make_shared<geom::Triangle>(
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
              obj = std::make_shared<geom::Square>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Square>(params[0],
                                                             util::Point(
                                                                     params[1],
                                                                     params[2],
                                                                     params[3]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<geom::Square>(
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
              obj = std::make_shared<geom::Rectangle>(params[0],
                                                                params[1]);
              return;
            } else if (n == 5) {
              obj = std::make_shared<geom::Rectangle>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<geom::Rectangle>(
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
              obj = std::make_shared<geom::Hexagon>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Hexagon>(
                      params[0], util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 7) {
              obj = std::make_shared<geom::Hexagon>(
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
              obj = std::make_shared<geom::Drum2D>(
                      params[0], params[1]);
              return;
            } else if (n == 5) {
              obj = std::make_shared<geom::Drum2D>(
                      params[0], params[1],
                      util::Point(params[2], params[3], params[4]));
              return;
            } else if (n == 8) {
              obj = std::make_shared<geom::Drum2D>(
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
              obj = std::make_shared<geom::Cube>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Cube>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            } else if (n == 6) {
              obj = std::make_shared<geom::Cube>(
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
              obj = std::make_shared<geom::Cuboid>(
                      params[0], params[1], params[2]);
              return;
            } else if (n == 6) {
              obj = std::make_shared<geom::Cuboid>(
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
              obj = std::make_shared<geom::Circle>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Circle>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Circle
      else if (geom_type == "sphere") {

        num_params_needed = {1, 4};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 1) {
              obj = std::make_shared<geom::Sphere>(params[0]);
              return;
            } else if (n == 4) {
              obj = std::make_shared<geom::Sphere>(
                      params[0],
                      util::Point(params[1], params[2], params[3]));
              return;
            }
          } // if params.size() == n
        } // loop over n
      } // Sphere
      else if (geom_type == "ellipse") {

        num_params_needed = {6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 6) {
              obj = std::make_shared<geom::Ellipse>(
                      params[0], params[1], params[2],
                      util::Point(params[3], params[4], params[5]));
              return;
            }
          }
        }
      } // Ellipse
      else if (geom_type == "ellipsoid") {

        num_params_needed = {6, 10};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 6) {
              obj = std::make_shared<geom::Ellipsoid>(
                      params[0], params[1], params[2],
                      util::Point(params[3], params[4], params[5]));
              return;
            }
            if (n == 10) {
              obj = std::make_shared<geom::Ellipsoid>(
                      params[0], params[1], params[2], params[3],
                      util::Point(params[4], params[5], params[6]),
                      util::Point(params[7], params[8], params[9]));
              return;
            }
          }
        }
      } // Ellipsoid
      else if (geom_type == "cylinder") {

        num_params_needed = {7, 8};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 7) {
              // r, center begin, axis vector (no normalization as length of axis is length of cylinder)
              obj = std::make_shared<geom::Cylinder>(
                      params[0],
                      util::Point(params[1], params[2], params[3]),
                      util::Point(params[4], params[5], params[6]));
              return;
            } else if (n == 8) {
              // r, length, center begin, unit axis vector
              obj = std::make_shared<geom::Cylinder>(
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
              obj = std::make_shared<geom::Rectangle>(
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
              obj = std::make_shared<geom::Cuboid>(
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

              auto rin = new geom::Rectangle(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              auto rout = new geom::Rectangle(
                      util::Point(params[6], params[7], params[8]),
                      util::Point(params[9], params[10], params[11]));

              obj = std::make_shared<geom::AnnulusGeomObject>
                      (rin, rout);

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

              auto rin = new geom::Cuboid(
                      util::Point(params[0], params[1], params[2]),
                      util::Point(params[3], params[4], params[5]));
              auto rout = new geom::Cuboid(
                      util::Point(params[6], params[7], params[8]),
                      util::Point(params[9], params[10], params[11]));

              obj = std::make_shared<geom::AnnulusGeomObject>
                      (rin, rout);

              return;
            }
          } // if params.size() == n
        } // loop over n
      } // cuboid_minus_cuboid
      else if (geom_type == "circle_minus_circle") {

        num_params_needed = {5};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 5) {
              const double cx = params[0];
              const double cy = params[1];
              const double cz = params[2];
              const double r_outer = params[3];
              const double r_inner = params[4];
              if (r_inner <= 0. || r_outer <= r_inner)
                throw std::runtime_error(
                        "circle_minus_circle: require 0 < r_inner < r_outer (cx,cy,cz,r_outer,r_inner).");
              auto *cin = new geom::Circle(r_inner, util::Point(cx, cy, cz));
              auto *cout = new geom::Circle(r_outer, util::Point(cx, cy, cz));
              obj = std::make_shared<geom::AnnulusGeomObject>(cin, cout);
              return;
            }
          }
        }
      } // circle_minus_circle
      else if (geom_type == "sphere_minus_sphere") {

        num_params_needed = {5};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 5) {
              const double cx = params[0];
              const double cy = params[1];
              const double cz = params[2];
              const double r_outer = params[3];
              const double r_inner = params[4];
              if (r_inner <= 0. || r_outer <= r_inner)
                throw std::runtime_error(
                        "sphere_minus_sphere: require 0 < r_inner < r_outer (cx,cy,cz,r_outer,r_inner).");
              auto *cin = new geom::Sphere(r_inner, util::Point(cx, cy, cz));
              auto *cout = new geom::Sphere(r_outer, util::Point(cx, cy, cz));
              obj = std::make_shared<geom::AnnulusGeomObject>(cin, cout);
              return;
            }
          }
        }
      } // sphere_minus_sphere
      else if (geom_type == "open_rect_channel_2d") {

        num_params_needed = {6};

        for (auto n: num_params_needed) {
          if (params.size() == n) {
            if (n == 6) {
              obj = std::make_shared<geom::OpenRectChannel2D>(
                      params[0], params[1], params[2], params[3], params[4], params[5]);
              return;
            }
          }
        }
      } // open_rect_channel_2d
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

        std::vector<std::shared_ptr<geom::GeomObject>> objs(vec_type.size());
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
            geom::createGeomObject(
                    geom_type_temp, params_temp, vec_type_temp, vec_flag_temp,
                    objs[i], perform_check);

            param_start += params_level[i];
          } // loop over objects

          // now create a composite object
          obj = std::make_shared<geom::ComplexGeomObject>(objs, obj_flags);

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

    void createGeomObject(GeomData &geomData,
                                          bool perform_check) {

      createGeomObject(geomData.d_geomName, geomData.d_geomParams,
                       geomData.d_geomComplexInfo.first,
                       geomData.d_geomComplexInfo.second,
                       geomData.d_geom_p, perform_check);
    }

    void geom::GeomData::copyGeometry(geom::GeomData &z) {
      z.d_geomName = d_geomName;
      z.d_geomParams = d_geomParams;
      z.d_geomComplexInfo = d_geomComplexInfo;

      if (d_geom_p->d_name == "null") {
        z.d_geom_p =
                std::make_shared<geom::NullGeomObject>(
                        d_geom_p->d_description);
      } else if (d_geom_p->d_name.empty()) {
        z.d_geom_p =
                std::make_shared<geom::GeomObject>(
                        d_geom_p->d_name, d_geom_p->d_description);
      } else {
        geom::createGeomObject(z);
      }
    }

    void geom::GeomData::copyGeometry(std::string &name,
                                                std::vector<double> &params,
                                                std::pair<std::vector<std::string>, std::vector<std::string>> &complexInfo,
                                                std::shared_ptr<geom::GeomObject> &geom) {
      name = d_geomName;
      params = d_geomParams;
      complexInfo = d_geomComplexInfo;

      if (d_geom_p->d_name == "null") {
        geom =
                std::make_shared<geom::NullGeomObject>(
                        d_geom_p->d_description);
      } else if (d_geom_p->d_name.empty()) {
        geom =
                std::make_shared<geom::GeomObject>(
                        d_geom_p->d_name, d_geom_p->d_description);
      } else {
        geom::createGeomObject(name,
                                         params,
                                         complexInfo.first,
                                         complexInfo.second,
                                         geom);
      }
    }

}// Utility functions

// read and write geometry from json object
namespace geom {

  void writeGeometry(json &j, const geom::GeomData &geomData) {
    j["Type"] = geomData.d_geomName;

    if (geomData.d_geomName == "complex") {
      j["Vec_type"] = geomData.d_geomComplexInfo.first;
      j["Vec_flag"] = geomData.d_geomComplexInfo.second;
    }

    j["Parameters"] = geomData.d_geomParams;
  }

  void readGeometry(const json &j, geom::GeomData &geomData) {

    if (j.find("Type") == j.end()) {
      std::cerr << "Error: Geometry type not found in json file.\n";
      exit(1);
    }
    geomData.d_geomName = j.at("Type");

    if (geomData.d_geomName == "complex") {
      if ((j.find("Vec_type") == j.end()) or (j.find("Vec_Flag") == j.end())) {
        std::cerr << "Error: Geometry type and/or flag not found in json file.\n";
        exit(1);
      }
      geomData.d_geomComplexInfo.first = j.at("Vec_type");
      geomData.d_geomComplexInfo.second = j.at("Vec_flag");
    }

    if (j.find("Parameters") == j.end()) {
      std::cerr << "Error: Geometry parameters not found in json file.\n";
      exit(1);
    }

    for (auto a: j.at("Parameters"))
      geomData.d_geomParams.push_back(a);
  }

  GeomObject* createGeomDeepCopy(GeomObject* obj) {
    if (!obj)
      return nullptr;

    const std::string &type = obj->d_name;
    if (type == "null")
      return new NullGeomObject(*dynamic_cast<const NullGeomObject *>(obj));
    if (type == "line")
      return new Line(*dynamic_cast<const Line *>(obj));
    if (type == "triangle")
      return new Triangle(*dynamic_cast<const Triangle *>(obj));
    if (type == "square")
      return new Square(*dynamic_cast<const Square *>(obj));
    if (type == "rectangle")
      return new Rectangle(*dynamic_cast<const Rectangle *>(obj));
    if (type == "hexagon")
      return new Hexagon(*dynamic_cast<const Hexagon *>(obj));
    if (type == "drum2d")
      return new Drum2D(*dynamic_cast<const Drum2D *>(obj));
    if (type == "cube")
      return new Cube(*dynamic_cast<const Cube *>(obj));
    if (type == "cuboid")
      return new Cuboid(*dynamic_cast<const Cuboid *>(obj));
    if (type == "circle")
      return new Circle(*dynamic_cast<const Circle *>(obj));
    if (type == "sphere")
      return new Sphere(*dynamic_cast<const Sphere *>(obj));
    if (type == "cylinder")
      return new Cylinder(*dynamic_cast<const Cylinder *>(obj));
    if (type == "ellipse")
      return new Ellipse(*dynamic_cast<const Ellipse *>(obj));
    if (type == "ellipsoid")
      return new Ellipsoid(*dynamic_cast<const Ellipsoid *>(obj));
    if (type == "annulus_object")
      return new AnnulusGeomObject(*dynamic_cast<const AnnulusGeomObject *>(obj));
    if (type == "open_rect_channel_2d")
      return new OpenRectChannel2D(*dynamic_cast<const OpenRectChannel2D *>(obj));

    std::cerr << "Error: Unsupported object type '" << type << "' in createGeomDeepCopy\n";
    exit(1);
  }

}