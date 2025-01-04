# ACES

This repository implements the proposed spatial accelerator design in the paper [**ACES: Accelerating Sparse Matrix Multiplication with Adaptive Execution Flow and Concurrency-Aware Cache Optimizations (ASPLOS 2024)**](https://dl.acm.org/doi/10.1145/3620666.3651381).

**Please note that:** The simulator we have implemented is **cycle accurate** and comes with MSHR cache (with fixed cache row size and accurate memory package for every element). Therefore, the operation time is generally longer than that of event-driven simulators.

# Build
Run the following code to build ACES

    git clone git@github.com:Boyu-Long/ACES.git
    mkdir build
    cd build
    cmake ..
    make

a executable file named **aces** will be generated in the same directory.

# Get a matrix
Running **get_mat.py** and **process_mat.py** in ACES/mat/get_mat will generate matrix files for use by aces.

# Run
Aces accepts the following parameters:

1. matrix file path (necessary): the absolute path of matrix file.
2. matrix name (necessary): the name of the matrix which should also be the name of the file.
3. MSHR Table size (optional, default 16): the mshr entry size of aces
4. MSHR Load/Store Table size (optional, default 64): the load/store table size of mshr table
5. Condense type (optional, default -1): 
    1. 0 for outer
    2. 1 for fully-condense
    3. 2 for mid-condense
    4. -1 for automatic sampling and selecting the optimal condense type based on performance

The results will be saved in "matrix file path"/"matrix name"_ACES_performance.txt


Please cite our paper if you find ACES useful for your research:
```
@inproceedings{lu2024aces,
  title={ACES: Accelerating Sparse Matrix Multiplication with Adaptive Execution Flow and Concurrency-Aware Cache Optimizations},
  author={Lu, Xiaoyang and Long, Boyu and Chen, Xiaoming and Han, Yinhe and Sun, Xian-He},
  booktitle={Proceedings of the 29th ACM International Conference on Architectural Support for Programming Languages and Operating Systems, Volume 3},
  pages={71--85},
  year={2024}
}
```

