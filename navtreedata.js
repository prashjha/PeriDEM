/*
@licstart  The following is the entire license notice for the
JavaScript code in this file.

Copyright (C) 1997-2019 by Dimitri van Heesch

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

@licend  The above is the entire license notice
for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "PeriDEM", "index.html", [
    [ "PeriDEM - Peridynamics-based discrete element model of granular systems", "index.html", [
      [ "Table of contents", "index.html#autotoc_md37", null ],
      [ "Introduction", "index.html#autotoc_md38", null ],
      [ "<a href=\"https://prashjha.github.io/PeriDEM/\">Documentation</a>", "index.html#autotoc_md39", null ],
      [ "Tutorial", "index.html#autotoc_md40", null ],
      [ "@ref ./examples/README.md \"Examples\"", "index.html#autotoc_md41", [
        [ "Two-particle tests", "index.html#autotoc_md42", null ],
        [ "Two-particle with wall test", "index.html#autotoc_md43", null ],
        [ "Compressive tests", "index.html#autotoc_md44", null ],
        [ "Attrition tests - Particles in a rotating container", "index.html#autotoc_md45", null ]
      ] ],
      [ "Brief implementation details", "index.html#autotoc_md46", [
        [ "DEMModel::run()", "index.html#autotoc_md47", null ],
        [ "DEMModel::integrate()", "index.html#autotoc_md48", null ],
        [ "DEMModel::computeForces()", "index.html#autotoc_md49", null ],
        [ "Further reading", "index.html#autotoc_md50", null ]
      ] ],
      [ "@ref ./tools/README.md \"Installation\"", "index.html#autotoc_md51", [
        [ "Dependencies", "index.html#autotoc_md52", null ],
        [ "Building the code", "index.html#autotoc_md53", null ],
        [ "Future plans", "index.html#autotoc_md54", null ],
        [ "Ask for help", "index.html#autotoc_md55", null ]
      ] ],
      [ "Running simulations", "index.html#autotoc_md56", [
        [ "Two-particle with wall", "index.html#autotoc_md57", null ],
        [ "Compressive test", "index.html#autotoc_md58", null ]
      ] ],
      [ "Visualizing results", "index.html#autotoc_md59", null ],
      [ "Citations", "index.html#autotoc_md60", null ],
      [ "Developers", "index.html#autotoc_md61", null ]
    ] ],
    [ "PeriDEM: Examples", "md__home_user_project_examples_README.html", [
      [ "Running simulations", "md__home_user_project_examples_README.html#autotoc_md5", [
        [ "Two-particle tests", "md__home_user_project_examples_README.html#autotoc_md1", null ],
        [ "Two-particle with wall test", "md__home_user_project_examples_README.html#autotoc_md2", null ],
        [ "Compressive tests", "md__home_user_project_examples_README.html#autotoc_md3", null ],
        [ "Attrition tests - Particles in a rotating container", "md__home_user_project_examples_README.html#autotoc_md4", null ],
        [ "Two-particle with wall", "md__home_user_project_examples_README.html#autotoc_md6", [
          [ "Important remark on modifying input.yaml file", "md__home_user_project_examples_README.html#autotoc_md7", null ]
        ] ],
        [ "Compressive test", "md__home_user_project_examples_README.html#autotoc_md8", null ],
        [ "Compute times for various examples (From old version of the code!)", "md__home_user_project_examples_README.html#autotoc_md9", null ]
      ] ],
      [ "Visualizing results", "md__home_user_project_examples_README.html#autotoc_md10", null ]
    ] ],
    [ "meshpartitioning", "md__home_user_project_test_test_data_meshpartitioning_README.html", null ],
    [ "testparallelcomp", "md__home_user_project_test_test_data_testparallelcomp_README.html", null ],
    [ "dockerimages", "md__home_user_project_tools_docker_README.html", [
      [ "building docker image ready for PeriDEM library", "md__home_user_project_tools_docker_README.html#autotoc_md14", null ],
      [ "building and testing PeriDEM using docker", "md__home_user_project_tools_docker_README.html#autotoc_md15", null ],
      [ "clion remote development", "md__home_user_project_tools_docker_README.html#autotoc_md16", null ]
    ] ],
    [ "PeriDEM: Installation", "md__home_user_project_tools_README.html", [
      [ "Dependencies", "md__home_user_project_tools_README.html#autotoc_md18", [
        [ "Building the code", "md__home_user_project_tools_README.html#autotoc_md19", null ]
      ] ],
      [ "Installing dependencies", "md__home_user_project_tools_README.html#autotoc_md20", [
        [ "Mac", "md__home_user_project_tools_README.html#autotoc_md21", null ],
        [ "Ubuntu", "md__home_user_project_tools_README.html#autotoc_md22", null ]
      ] ],
      [ "for metis, create symlink", "md__home_user_project_tools_README.html#autotoc_md23", null ],
      [ "python libs", "md__home_user_project_tools_README.html#autotoc_md24", null ],
      [ "NOTE: add '–break-system-packages' at the end if installing in Ubuntu 24.04 (noble)", "md__home_user_project_tools_README.html#autotoc_md25", null ],
      [ "get codename of ubuntu", "md__home_user_project_tools_README.html#autotoc_md26", null ],
      [ "add repo", "md__home_user_project_tools_README.html#autotoc_md27", null ],
      [ "now you can install recent cmake from apt-get", "md__home_user_project_tools_README.html#autotoc_md28", null ]
    ] ],
    [ "Tutorial", "md__home_user_project_tutorial_README.html", [
      [ "Two-particles test", "md__home_user_project_tutorial_README.html#autotoc_md32", null ],
      [ "Particle and wall test", "md__home_user_project_tutorial_README.html#autotoc_md33", null ],
      [ "Two-particles with wall test", "md__home_user_project_tutorial_README.html#autotoc_md34", null ],
      [ "Compressive test", "md__home_user_project_tutorial_README.html#autotoc_md35", null ]
    ] ],
    [ "Modules", "modules.html", "modules" ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", "namespacemembers_func" ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ]
      ] ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Related Functions", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"PeriDEMConfig_8h.html",
"classmaterial_1_1Material.html#a98692ea03fbc7e5c42e3266b1519aa6b",
"classnsearch_1_1BaseNSearch.html#a653b263977553ad5f6b942a02e390335",
"classrw_1_1reader_1_1VtkParticleReader.html#a6a08e0ad1486f9cb1e1f5d6f9224f019",
"classutil_1_1geometry_1_1Cuboid.html#a23e713ac9a77180cf746a4eb76c0245e",
"classutil_1_1geometry_1_1Triangle.html#abcbed3a1e604e5c6154ea0e25667b70f",
"examples_2PeriDEM_2attrition__tests_2sim2__multi__particle__circ__tri__drum__hex__with__rotating55a3791f74235677eb61ddf2ba3e367e.html#a55fb909469dce4448cbab6267c05d783",
"feElementDefs_8h.html#ga1a164a7b8ac23ab4ab769db408eea56f",
"io_8h.html#a6588c46813adf0cb86b795880f1a2ec7",
"nflannSetup_8h.html#a4b7eed2a5de253d24f7718a9a01c274c",
"structinp_1_1MeshDeck.html#a707e2499289b05aeec114430c8cb7df3",
"structutil_1_1geometry_1_1BoxPartition.html#a68d1523bf8dd81296da63489e576e951",
"test_2test__data_2peridem_2compression__small__set__restart_2inp_2problem__setup_8py.html#a1afd4fae0c6853fdedafa9abd449653e",
"vtkWriter_8h_source.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';