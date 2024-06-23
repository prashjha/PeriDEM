#!/bin/bash
MY_PWD=$(pwd)

(
if [[ $# -gt 0 ]]; then n_threads="$1"; else n_threads="2"; fi 

cd "inp" && python3 -B problem_setup.py

peridem="../../../../../bin/PeriDEM"
$peridem -i input_0.yaml -nThreads $n_threads
) 2>&1 |  tee output.log

# check if we have produced 'output_10.vtu' file
cd $MY_PWD
if [[ -f "out/output_10.vtu" ]]; then exit 0; else exit 1; fi
