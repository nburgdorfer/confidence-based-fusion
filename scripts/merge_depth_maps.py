import numpy as np
import sys
import os
from plyfile import PlyData, PlyElement

def merge_plys(data_path):

    mesh_list = []
    file_list = os.listdir(data_path)
    file_list.sort()

    print("Loading point clouds...")
    for pc in file_list:
        mesh = PlyData.read(data_path+pc)
        mesh_list.append(mesh.elements[0].data)

    print("Merging point clouds...")
    merged_mesh = mesh_list[0]
    for i in range(1,len(mesh_list)):
        merged_mesh = np.concatenate((merged_mesh, mesh_list[i]))
    
    return merged_mesh

def save_ply(ply, file_path):
    mesh = np.array(  ply,
                        dtype=[ ('x','f4'),
                                ('y','f4'),
                                ('z','f4'),
                                ('red','u1'),
                                ('green','u1'),
                                ('blue','u1')])
    elem = PlyElement.describe(mesh, 'vertex')
    PlyData([elem]).write(file_path)

def main():
    if(len(sys.argv) != 2):
        print("Error: usage python3 {} <data-path>".format(sys.argv[0]))
        sys.exit()


    data_path = sys.argv[1]

    merged_file_path = data_path+"merged.ply"
    if os.path.exists(merged_file_path):
        os.remove(merged_file_path)

    merged_ply = merge_plys(data_path)
    save_ply(merged_ply, merged_file_path)

if __name__=="__main__":
    main()
