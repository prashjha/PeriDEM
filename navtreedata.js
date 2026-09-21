/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "PeriDEM", "index.html", [
    [ "PeriDEM - High-fidelity modeling of granular media consisting of deformable complex-shaped particles", "index.html", "index" ],
    [ "PeriDEM driver", "md__2home_2user_2project_2PeriDEM_2README.html", null ],
    [ "Attrition examples (main GIF suite)", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2README.html", null ],
    [ "Attrition sim1 — rotating cylinder + protrusion", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim1__rotating__cylinder_2README.html", null ],
    [ "Attrition input defaults audit (modular vs main sim2)", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2INPUT__DEFAULTS.html", [
      [ "Trap defaults (will bite if omitted)", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2INPUT__DEFAULTS.html#autotoc_md4", null ],
      [ "Explicit physics (must match main sim2 numbers)", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2INPUT__DEFAULTS.html#autotoc_md5", null ],
      [ "Wall DOFs — already controlled by input (do not confuse with main’s flag)", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2INPUT__DEFAULTS.html#autotoc_md6", null ],
      [ "Not exposed, or optional", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2INPUT__DEFAULTS.html#autotoc_md7", null ]
    ] ],
    [ "Attrition sim2 — thin container + offset rotation", "md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2README.html", null ],
    [ "Compressive n12 (small pack)", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n12_2README.html", [
      [ "Run (from a <strong>copy under build/</strong>)", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n12_2README.html#autotoc_md10", null ],
      [ "Python (<tt>-DEnable_Python=ON</tt>)", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n12_2README.html#autotoc_md11", null ]
    ] ],
    [ "Compressive N≈502 (Jha JMPS 2021 §4.4)", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n500_2README.html", [
      [ "Paper parameters", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n500_2README.html#autotoc_md13", null ],
      [ "Stages", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n500_2README.html#autotoc_md14", null ],
      [ "Run", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n500_2README.html#autotoc_md15", null ],
      [ "Restart IC", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2n500_2README.html#autotoc_md16", null ]
    ] ],
    [ "Compressive examples", "md__2home_2user_2project_2examples_2PeriDEM_2compressive_2README.html", null ],
    [ "Ellipse × short tip", "md__2home_2user_2project_2examples_2PeriDEM_2ellipse__triangle_2README.html", null ],
    [ "PeriDEM examples", "md__2home_2user_2project_2examples_2PeriDEM_2README.html", [
      [ "Python", "md__2home_2user_2project_2examples_2PeriDEM_2README.html#autotoc_md20", null ],
      [ "JSON", "md__2home_2user_2project_2examples_2PeriDEM_2README.html#autotoc_md21", null ],
      [ "What is checked in", "md__2home_2user_2project_2examples_2PeriDEM_2README.html#autotoc_md22", null ]
    ] ],
    [ "Silling 2003 Kalthoff–Winkler (KW)", "md__2home_2user_2project_2examples_2PeriDEM_2silling__kw_2README.html", [
      [ "Python", "md__2home_2user_2project_2examples_2PeriDEM_2silling__kw_2README.html#autotoc_md24", null ]
    ] ],
    [ "Circle — single-particle Peridynamics", "md__2home_2user_2project_2examples_2Peridynamics_2circle_2README.html", null ],
    [ "Peridynamics examples (single particle)", "md__2home_2user_2project_2examples_2Peridynamics_2README.html", null ],
    [ "Rectangle — single-particle Peridynamics", "md__2home_2user_2project_2examples_2Peridynamics_2rectangle_2README.html", null ],
    [ "Examples", "md__2home_2user_2project_2examples_2README.html", [
      [ "What each folder holds", "md__2home_2user_2project_2examples_2README.html#autotoc_md29", null ],
      [ "The examples", "md__2home_2user_2project_2examples_2README.html#autotoc_md30", null ],
      [ "Running them", "md__2home_2user_2project_2examples_2README.html#autotoc_md31", null ],
      [ "Meshes", "md__2home_2user_2project_2examples_2README.html#autotoc_md32", null ],
      [ "Comparing the Python and C++ decks", "md__2home_2user_2project_2examples_2README.html#autotoc_md33", null ]
    ] ],
    [ "----------------------------------------—", "md__2home_2user_2project_2src_2contact_2README.html", [
      [ "Copyright (c) 2021 - 2026 Prashant K. Jha", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md35", null ],
      [ "----------------------------------------—", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md36", null ],
      [ "PeriDEM https://github.com/prashjha/PeriDEM", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md37", null ],
      [ "Distributed under the Boost Software License, Version 1.0. (See accompanying", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md38", null ],
      [ "file LICENSE)", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md39", null ],
      [ "Contact / wall / self-contact policy selection (input deck).", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md40", null ],
      [ "Defaults (Jha-compatible):", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md41", null ],
      [ "Contact.Damping_Law  = com_and_node", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md42", null ],
      [ "Contact.Friction_Law = coulomb_simple", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md43", null ],
      [ "Contact.Correct_Volume = true   # partial Vj near Rc (modular default)", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md44", null ],
      [ "Model.Self_Contact   = broken_bond_kn", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md45", null ],
      [ "Model.Wall_Contact   = meshed", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md46", null ],
      [ "Model.Bond_Break     = tension", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md47", null ],
      [ "Alternates:", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md48", null ],
      [ "Damping_Law:  com | node | off", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md49", null ],
      [ "Friction_Law: stick_slip", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md50", null ],
      [ "Correct_Volume: false  # full Vj — matches main-branch DEM contact spring", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md51", null ],
      [ "Self_Contact: reference_gap | none", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md52", null ],
      [ "Wall_Contact: analytical_plane", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md53", null ],
      [ "Bond_Break:   absolute_stretch", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md54", null ],
      [ "Normal contact spring is force density ∝ Vj (corrected when Correct_Volume", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md55", null ],
      [ "is true). The velocity update is v += (dt/rho)*f.", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md56", null ],
      [ "Set policies in JSON under Contact / Model, or when building a deck in C++", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md57", null ],
      [ "(see test/test_data/peridem/*_inbuilt drivers). Factories live in", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md58", null ],
      [ "contact/policy.cpp, pd/selfContact.cpp, contact/wallContact.cpp.", "md__2home_2user_2project_2src_2contact_2README.html#autotoc_md59", null ]
    ] ],
    [ "Literature tests", "md__2home_2user_2project_2test_2literature.html", [
      [ "Jha et al., JMPS 2021, 151:104376", "md__2home_2user_2project_2test_2literature.html#autotoc_md61", null ],
      [ "Bhattacharya and Lipton, SISC 2023", "md__2home_2user_2project_2test_2literature.html#autotoc_md62", null ],
      [ "Bhattacharya, Damircheli, Lipton, 2025 (arXiv:2506.05362)", "md__2home_2user_2project_2test_2literature.html#autotoc_md63", null ],
      [ "Bhattacharya and Lipton, 2025b (vehicle mobility)", "md__2home_2user_2project_2test_2literature.html#autotoc_md64", null ]
    ] ],
    [ "meshpartitioning", "md__2home_2user_2project_2test_2test__data_2meshpartitioning_2README.html", null ],
    [ "parallelcomp", "md__2home_2user_2project_2test_2test__data_2parallelcomp_2README.html", null ],
    [ "Topics", "topics.html", "topics" ],
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
        [ "Related Symbols", "functions_rela.html", null ]
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
"PeriDEM_2attrition_2sim1__rotating__cylinder_2problem_8py.html",
"classPeriDEMModel.html#a3b0380a06df9424497c82e824bfd67c5",
"classdata_1_1ModelData.html#a8eaa3aa1c543491a6f92bd0a49096b03",
"classgeom_1_1AnnulusGeomObject.html#aa74ad5c8916557a83880924e85e84417",
"classgeom_1_1Cuboid.html#af16d6c84ecd7ee834bf9535f2cc51b56",
"classgeom_1_1GeomObject.html#a508111509336368f7b167d91dadf704f",
"classgeom_1_1OpenRectChannel2D.html#a400c5318bdfd82e873251cbad0b677e5",
"classgeom_1_1Square.html#a6feb9bd77cb5eaa54a448d5c149e5df0",
"classmaterial_1_1Material.html#aee6d2ee329eadf310db0606badd26c4a",
"classmesh_1_1Mesh.html#a63b02c0b2efec0cca7288cbb910f8a9f",
"classparticle_1_1BaseParticle.html#a18cf721d036b02feca3335040a9939dc",
"classparticle_1_1BaseParticle.html#af6fc9bcbcf2995c65502b25cef7768eb",
"classrw_1_1writer_1_1VtkParticleWriter.html#a1613ce911d7b73ad801499ce3af5e486",
"dir_7322a20eee69e371dd3023174b7846a0.html",
"geomUtilFunctions_8h.html#a2779a35d5ce741a89d57c69366e42523",
"md__2home_2user_2project_2examples_2PeriDEM_2attrition_2sim2__thin__container_2README.html",
"namespaceanonymous__namespace_02testFeLib_8cpp_03.html#afd6fd93513908247402cbf5f5eb17c79",
"namespacemembers_func_h.html",
"namespacerw_1_1writer.html",
"particleULoading_8h.html",
"structanonymous__namespace_02main_8cpp_03_1_1Probe.html",
"structinp_1_1Field.html#a41dec8f94723d1a3b55e23c77f036a7a",
"structinp_1_1RestartDeck.html#af19d9cd3ef7b5f55998fb1ce674c845f",
"testDeckField_8cpp.html#a255c6d6a4b5ebaca5b740dbdbf669a2d",
"vecMethods_8h.html#aab17a5d1088344c57c7efd4c06815fe4"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';