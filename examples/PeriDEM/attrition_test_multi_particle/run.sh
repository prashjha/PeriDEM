#!/bin/bash

(

pd_exec="/home/prashant/work/peridem/PeriDEM/build/Release/bin/PeriDEM"

python3 -B problem_setup.py

${pd_exec} -i input_0.yaml -nThreads 16

) 2>&1 | tee "$(basename "$0").log"
