"""
Migrates the depth and confidence maps for a given scan form MVSNet to the data dir for fusion
"""
import numpy as np
import sys
import os
import shutil
import re
import cv2

def move_depth_files(old_data_path, new_data_path):
    new_data_path += "depth_maps/"

    old_file_ext = "init.pfm"
    new_file_ext = "depth.pfm"

    # remove new path content
    for f in os.listdir(new_data_path):
        file_path = os.path.join(new_data_path, f)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print('Failed to delete %s. Reason: %s' % (file_path, e))

    # migrate files to new path
    old_ext_len = len(old_file_ext)
    files = os.listdir(old_data_path)
    for f in files:
        if (f[-(old_ext_len):] == old_file_ext):
            old_file = os.path.join(old_data_path, f)
            new_file = os.path.join(new_data_path, (f[:-(old_ext_len)] + new_file_ext))
            shutil.copyfile(old_file, new_file)

def move_prob_files(old_data_path, new_data_path):
    new_data_path += "conf_maps/"

    old_file_ext = "prob.pfm"
    new_file_ext = "conf.pfm"

    # remove new path content
    for f in os.listdir(new_data_path):
        file_path = os.path.join(new_data_path, f)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print('Failed to delete %s. Reason: %s' % (file_path, e))

    # migrate files to new path
    old_ext_len = len(old_file_ext)
    files = os.listdir(old_data_path)
    for f in files:
        if (f[-(old_ext_len):] == old_file_ext):
            old_file = os.path.join(old_data_path, f)
            new_file = os.path.join(new_data_path, (f[:-(old_ext_len)] + new_file_ext))
            shutil.copyfile(old_file, new_file)

def move_cam_files(old_data_path, new_data_path):
    new_data_path += "cams/"

    old_file_ext = ".txt"
    new_file_ext = "_cam.txt"

    # remove new path content
    for f in os.listdir(new_data_path):
        file_path = os.path.join(new_data_path, f)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print('Failed to delete %s. Reason: %s' % (file_path, e))

    # migrate files to new path
    old_ext_len = len(old_file_ext)
    files = os.listdir(old_data_path)
    for f in files:
        if (f[-(old_ext_len):] == old_file_ext):
            old_file = os.path.join(old_data_path, f)
            new_file = os.path.join(new_data_path, (f[:-(old_ext_len)] + new_file_ext))
            shutil.copyfile(old_file, new_file)

def main():
    if(len(sys.argv) != 3):
        print("Error: usage python {} <old-data-path> <new-data-path>".format(sys.argv[0]))
        sys.exit()
        
    old_data_path = sys.argv[1]
    new_data_path = sys.argv[2]

    move_depth_files(old_data_path, new_data_path)
    move_prob_files(old_data_path, new_data_path)
    move_cam_files(old_data_path, new_data_path)

if __name__=="__main__":
    main()
