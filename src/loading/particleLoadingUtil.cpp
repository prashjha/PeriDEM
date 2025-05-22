/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleLoadingUtil.h"
#include "util/methods.h"

bool loading::needToProcessParticle(size_t id, const inp::BCBaseDeck &bc) {

  // if there is a list, and if particle is not in the list, skip
  bool skip_condition1 = (bc.d_selectionType == "particle"
                          || bc.d_selectionType == "region_with_include_list")
                         && !util::methods::isInList(id, bc.d_pList);
  // if there is an exclusion list, and if particle is in the list, skip
  bool skip_condition2 = (bc.d_selectionType == "region_with_exclude_list")
                         && util::methods::isInList(id, bc.d_pNotList);
  // if there is a inclusion and an exclusion list,
  // and if particle is either in the exclusion list or not in the inclusion list, skip
  bool skip_condition3 = (bc.d_selectionType == "region_with_include_list_with_exclude_list")
                         && (util::methods::isInList(id, bc.d_pNotList) ||
                             !util::methods::isInList(id, bc.d_pList));

  bool skip = skip_condition1 or skip_condition2 or skip_condition3;
  return !skip;
}

bool loading::needToComputeDof(const util::Point &x,
                                                 size_t id,
                                                 const inp::BCBaseDeck &bc) {

  if (!bc.d_isRegionActive) {
    if (bc.d_selectionType == "particle" &&
        util::methods::isInList(id, bc.d_pList))
      return true;
  }
  else {
    if (bc.d_selectionType == "region" && bc.d_regionGeomData.d_geom_p->isInside(x))
      return true;
    else if (bc.d_selectionType == "region_with_include_list" &&
             bc.d_regionGeomData.d_geom_p->isInside(x) &&
        util::methods::isInList(id, bc.d_pList))
      return true;
    else if (bc.d_selectionType == "region_with_exclude_list" &&
             bc.d_regionGeomData.d_geom_p->isInside(x) &&
             !util::methods::isInList(id, bc.d_pNotList))
      return true;
    else if (bc.d_selectionType == "region_with_include_list_with_exclude_list" &&
             bc.d_regionGeomData.d_geom_p->isInside(x) &&
        util::methods::isInList(id, bc.d_pList) &&
             !util::methods::isInList(id, bc.d_pNotList))
      return true;
  }

  return false;
}
