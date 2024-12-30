import ssgetpy
import os

mat_id_list = [["2cubes_sphere", 1919], ["amazon0312", 2305], ["ca-CondMat", 2296],
               ["cage12", 913], ["cit-Patents", 2294], ["cop20k_A", 2378],
               ["email-Enron", 2290], ["filter3D", 1431],
               ["m133-b3", 2056], ["mario002", 1241], ["offshore", 2283],
               ["p2p-Gnutella31", 2316], ["patents_main", 1513], ["poisson3Da", 927],
               ["roadNet-CA", 2317], ["scircuit", 544], ["webbase-1M", 2379],
               ["web-Google", 2301], ["wiki-Vote", 2288]]
root_path = os.getcwd()
for mat_id in mat_id_list:
    path = root_path+"/../test_mat/"
    ssgetpy.fetch(mat_id[1], format='MM', location=path)





