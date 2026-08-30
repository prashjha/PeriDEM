/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "neighborPolicy.h"

#include "model/modelData.h"
#include "model/modelLog.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/parallelUtil.h"

#include <format>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void nsearch::updatePeridynamicNeighborlist(model::ModelData &data) {


  data.d_neighPd.resize(data.d_x.size());
  // data.d_neighPdSqdDist.resize(data.d_x.size());
  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, data.d_x.size(), (std::size_t) 1, [&data](std::size_t i) {
      const auto &pi = data.d_ptId[i];
      double search_r = data.d_particlesListTypeAll[pi]->d_material_p->getHorizon();

      std::vector<size_t> neighs;
      std::vector<double> sqr_dist;
      if (data.d_nsearch_p->radiusSearchIncludeTag(data.d_x[i],
                                                    search_r,
                                                    neighs,
                                                    sqr_dist,
                                                    data.d_ptId[i],
                                                    data.d_ptId) > 0) {
        for (std::size_t j = 0; j < neighs.size(); ++j)
          if (neighs[j] != i && data.d_ptId[neighs[j]] == pi) {
            data.d_neighPd[i].push_back(size_t(neighs[j]));
            // data.d_neighPdSqdDist[i].push_back(sqr_dist[j]);
          }
      }
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  model::log(data, std::format("{}: Peridynamics neighbor update time = {}\n",
                  data.d_name, util::methods::timeDiff(t1, t2)), 2);

}
