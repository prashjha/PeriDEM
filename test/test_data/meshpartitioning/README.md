# meshpartitioning 

Test partitioning of mesh for nonlocal interaction. 

Domain and the corresponding mesh:

| Domain | Mesh |
| :--- | :---: |
| <img src="../common_data/domain.png" style="width:400px;"> | <img src="../common_data/mesh.png" style="width:400px;"> |

For the nonlocal length scale `horizon = 4h`, `h` being the mesh size, Metis partitioning using two available methods (recursive and k-way) with number of partitions 4 and 8 are as follows:

<img src="../common_data/view_horizon_4h.png" style="width:400px;">

For the nonlocal length scale `horizon = 8h`, `h` being the mesh size, Metis partitioning using two available methods (recursive and k-way) with number of partitions 4 and 8 are as follows:

<img src="../common_data/view_horizon_8h.png" style="width:400px;">


