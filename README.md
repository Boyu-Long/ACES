# ACES
Code related to ACES (ASPLOS 2024)

# Build
Run the following code to build ACES

`git clone git@github.com:Boyu-Long/ACES.git
`mkdir build
`cd build
`cmake ..
`make

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
