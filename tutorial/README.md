# Tutorial

In this tutorial, we will look at in detail two simulations
  - **two-particles test** 
  - **two-particles with wall test** 
  - **compressive test** with circular and hexagon shaped particles.
Objective is to provide a very clear picture of setup and also provide some insights about the design of library. 

We will use python helper script [util.py](util.py) to prepare the `input.yaml` (main input file), `.geo` (input file for gmsh), `.csv` (particle locations file). If one understands how to generate input files for the two tests considered here, it is super easy to create input file for any complex setup. 

## Two-particles test
See jupyter notebook [two-particles test](two_particles.ipynb) where we clearly explain every steps in the simulation.

## Particle and wall test
See [particle_wall.py](particle_wall.py). You can run the example using
```sh
python3 particle_wall.py --peridem_path=<path to PeriDEM executible>
```

## Two-particles with wall test
This is an simple extension of above test and the python script to create the input files are provided in [two-particles with wall test](setup_two_particles_wall.py). In this test, we have two concave particles and a rectangular wall. Top particle is assigned a high downward velocity where as the second particle below the top particle and above the wall is falling freely on wall due to gravity. Wall is fixed to its place using zero displacement boundary condiiton.

## Compressive test
> TBA

