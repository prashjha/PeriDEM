#!/bin/bash

(

python3 -B problem_setup.py

# nThreads 4 took 2587ms
../../../bin/PeriDEM -i input_0.yaml -nThreads 16

) 2>&1 | tee "$(basename "$0").log"
