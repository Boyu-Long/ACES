import os
import scipy.io as sio
import get_mat
import tarfile


def merge(row, col, data):
    merger = []
    for i in range(len(row)):
        merger.append([row[i], col[i], data[i]])
    return merger


def write_mat(mat, shape, index, name):
    f = open(name, mode="w")
    f.write(str(shape[0]) + " " + str(shape[1]) + "\n")
    f.write(str(len(mat)) + "\n")
    for p in mat:
        s = ""
        for i in index:
            s = s + str(p[i]) + " "
        f.write(s + "\n")
    f.close()


mat_id_list = get_mat.mat_id_list
root_path = get_mat.root_path
for mat_id in mat_id_list:
    path = root_path + "/../test_mat/"
    # untar
    if os.path.isfile(path + mat_id[0] + "tar.gz"):
        tarfile.open(path + mat_id[0] + "tar.gz").extractall(path)
    # re-format
    path = root_path + "/../test_mat/" + mat_id[0] + "/"
    if not os.path.isfile(path + mat_id[0] + ".mtx"):
        print("error in process mat: " + path + mat_id[0] + ".mtx")
    else:
        mtx_mat = sio.mmread(path + mat_id[0] + ".mtx")
        mat = merge(mtx_mat.row, mtx_mat.col, mtx_mat.data)
        write_mat(mat, mtx_mat.shape, [1, 0, 2], path + mat_id[0] + "_c.txt")
        mat = sorted(mat, key=lambda x: (x[0], x[1], x[2]))
        write_mat(mat, mtx_mat.shape, [0, 1, 2], path + mat_id[0] + "_r.txt")
