# Benchmark neighbor search test with 100^3 points

> Note: For large set of points, the tree neighbors had more points than those from brute force method. This is acceptable!

## Multithread (8 threads) run output

### 125 points
brute state: mean = 157.1, std = 92.4716
tree state: mean = 198.8, std = 96.7459

### 1000 points
brute state: mean = 160.65, std = 104.741
tree state: mean = 196.8, std = 103.156

### 40^3 = 64000 points <--- speed up of 100x with tree
brute state: mean = 3.27835e+06, std = 426817
tree state: mean = 30967.7, std = 3487.33

### 100^3 = 1000000 points (million) <--- speed up of 1500x with tree
brute state: mean = 893323900.0, std = 84239428.57884306
tree state: mean = 584161.9, std = 104452.42287898352
