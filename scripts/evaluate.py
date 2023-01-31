import numpy as np
import sys
import os
import re
import cv2
import matplotlib.pyplot as plt


def main():
    #method="mvsnet"
    #method="ucsnet"
    #method="npcvp_dtu"
    method="gbinet"

    data_path = "/media/Data2/nate/Results/V-FUSE/dtu/Output_{}_BF/".format(method)
    gt_depth_path = "/media/Data/nate/Fusion/dtu/{}/GT_Depths/".format(method)
    scans=[1, 4, 9, 10, 11, 12, 13, 15, 23, 24, 29, 32, 33, 34, 48, 49, 62, 75, 77, 110, 114, 118]
    num_scans = len(scans)

    avg_mae = 0.0
    avg_auc = 0.0
    avg_pe = np.zeros((4,))
    for scan in scans:
        print("\nEvaluating scan {}".format(scan))

        scan_dir = "scan{:03d}/".format(scan)
        depth_path = os.path.join(data_path,scan_dir,"depths")
        conf_path = os.path.join(data_path,scan_dir,"confs")
        gt_path = os.path.join(gt_depth_path,scan_dir)

        depth_files = os.listdir(depth_path)
        depth_files.sort()
        depth_files = [d for d in depth_files if d[-3:]=="pfm"]

        conf_files = os.listdir(conf_path)
        conf_files.sort()
        conf_files = [c for c in conf_files if c[-3:]=="pfm"]

        gt_depth_files = os.listdir(gt_path)
        gt_depth_files.sort()
        gt_depth_files = [g for g in gt_depth_files if g[-3:]=="pfm"]

        num_views = len(depth_files)
        mae = 0.0
        view_num = 0
        total_auc = 0.0
        total_gt = 0.0 
        total_percs = np.zeros((4,))

        for (d,c,g) in zip(depth_files,conf_files,gt_depth_files):
            depth = load_pfm(os.path.join(depth_path, d))
            conf = load_pfm(os.path.join(conf_path, c))
            gt_depth = load_pfm(os.path.join(gt_path, g))
            gt_depth = mask_depth_image(gt_depth, 427, 935)
            mean_abs_error, pe, num_gt = error_stats(depth,gt_depth,view_num)
            auc = compute_auc(depth, conf, gt_depth, view_num)

            mae += mean_abs_error
            total_percs += pe
            total_gt += num_gt
            total_auc += auc
            view_num += 1

        mae = mae/num_views
        total_percs = total_percs/total_gt
        total_auc = total_auc/num_views

        avg_mae += mae
        avg_pe += total_percs
        avg_auc += total_auc

        print("MAE: {}".format(mae))
        print("THs: {}".format(100*total_percs))
        print("AUC: {}".format(total_auc))


    avg_mae /= num_scans
    avg_pe /= num_scans
    avg_auc /= num_scans
    print("\n----AVERAGES----")
    print("MAE: {}".format(avg_mae))
    print("THs: {}".format(100*avg_pe))
    print("AUC: {}".format(avg_auc))


def error_stats(fused_depth, gt_depth, view_num):
    height, width = gt_depth.shape
    th = 0.125

    fused_depth = fused_depth.astype(float)
    gt_depth = gt_depth.astype(float)

    # compute gt mask and number of valid pixels
    gt_mask = np.not_equal(gt_depth, 0.0).astype(float)
    gt_mask_flat = np.copy(gt_mask).flatten()
    num_gt = np.sum(gt_mask) + 1e-7

    ### Error ###
    # output
    signed_error = fused_depth - gt_depth
    abs_error = np.abs(signed_error)

    # measure error percentages
    ae = np.copy(abs_error).flatten()
    ae_flat = np.array( [x for x,g in zip(ae,gt_mask_flat) if g != 0] )
    e_0_125 = np.sum(np.where(ae_flat < th, 1,0))
    e_0_25 = np.sum(np.where(ae_flat < 2*th, 1,0))
    e_0_5 = np.sum(np.where(ae_flat < 4*th, 1,0))
    e_1_0 = np.sum(np.where(ae_flat < 8*th, 1,0))
    pe = np.asarray([e_0_125,e_0_25,e_0_5,e_1_0])


    ### Apply GT Mask ###
    # output
    signed_error = gt_mask * signed_error
    abs_error = gt_mask * abs_error
    mean_abs_error = np.mean(abs_error)

    #   ### Plots ###
    #   # output absolute error
    #   ad = abs_error[:,:]
    #   ad = np.clip(ad, 0, 1)

    #   fig = plt.figure()
    #   ax = fig.add_subplot(1,1,1, aspect='equal')
    #   shw = plt.imshow(ad)
    #   plt.hot()
    #   plt.axis('off')
    #   #divider = make_axes_locatable(ax)
    #   #cax1 = divider.append_axes("right", size="5%", pad=0.05)
    #   #bar = plt.colorbar(shw, cax=cax1)
    #   #bar.set_label("error (mm)")
    #   plt.savefig(os.path.join("errors/{:04d}_abs_error.png".format(view_num)), dpi=300, bbox_inches='tight')
    #   plt.close()

    return mean_abs_error, pe, num_gt

def load_pfm(pfm_file):
    with open(pfm_file,'rb') as pfm_file:
        color = None
        width = None
        height = None
        scale = None
        data_type = None
        header = pfm_file.readline().decode('iso8859_15').rstrip()

        if header == 'PF':
            color = True
        elif header == 'Pf':
            color = False
        else:
            raise Exception('Not a PFM file.')
        dim_match = re.match(r'^(\d+)\s(\d+)\s$', pfm_file.readline().decode('iso8859_15'))
        if dim_match:
            width, height = map(int, dim_match.groups())
        else:
            raise Exception('Malformed PFM header.')
        # scale = float(file.readline().rstrip())
        scale = float((pfm_file.readline()).decode('iso8859_15').rstrip())
        if scale < 0: # little-endian
            data_type = '<f'
        else:
            data_type = '>f' # big-endian
        data_string = pfm_file.read()
        data = np.frombuffer(data_string, data_type)
        shape = (height, width, 3) if color else (height, width)
        data = np.reshape(data, shape)
        data = cv2.flip(data, 0)
    return data


def compute_auc(fused_depth, fused_conf, gt_depth, view_num):
    height, width = fused_depth.shape

    gt_mask = np.not_equal(gt_depth, 0.0)
    gt_count = int(np.sum(gt_mask)+1e-7)

    # flatten to 1D tensor
    fused_depth = fused_depth.flatten()
    fused_conf = fused_conf.flatten()
    gt_depth = gt_depth.flatten()

    ##### OUTPUT #####
    # sort all tensors by confidence value
    indices = np.argsort(fused_conf)
    indices = indices[::-1]
    fused_depth = np.take(fused_depth, indices=indices, axis=0)
    output_gt_depth = np.take(gt_depth, indices=indices, axis=0)
    output_gt_indices = np.nonzero(output_gt_depth)[0]
    fused_depth = np.take(fused_depth, indices=output_gt_indices, axis=0)
    output_gt_depth = np.take(output_gt_depth, indices=output_gt_indices, axis=0)
    output_error = np.abs(fused_depth - output_gt_depth)

    # sort orcale curves by error
    indices = np.argsort(output_error)
    output_oracle = np.take(output_error, indices=indices, axis=0)
    output_oracle_gt = np.take(output_gt_depth, indices=indices, axis=0)
    output_oracle_indices = np.nonzero(output_oracle_gt)[0]
    output_oracle_error = np.take(output_oracle, indices=output_oracle_indices, axis=0)


    # build density vector
    num_gt_points = gt_depth.shape[0]
    perc = np.array(list(range(5,105,5)))
    density = np.array((perc/100) * (num_gt_points), dtype=np.int32)

    output_roc = np.zeros(density.shape)
    output_oracle = np.zeros(density.shape)

    for i,k in enumerate(density):
        # compute input absolute error chunk
        oe = output_error[0:k]
        ooe = output_oracle_error[0:k]
        if (oe.shape[0] == 0):
            output_roc[i] = 0.0
        else :
            output_roc[i] = np.mean(oe)

        if (ooe.shape[0] == 0):
            output_oracle[i] = 0.0
        else:
            output_oracle[i] = np.mean(ooe)

    # comput AUC
    output_auc = np.trapz(output_roc, dx=5)

    #   # plot ROC density errors
    #   plt.plot(perc, output_roc, label="output")
    #   plt.plot(perc, output_oracle, label="output_oracle")
    #   plt.title("ROC Error")
    #   plt.xlabel("density")
    #   plt.ylabel("absolte error")
    #   plt.legend()
    #   plt.savefig(os.path.join("{:04d}_roc_error.png".format(view_num)))
    #   plt.close()

    return output_auc

def mask_depth_image(depth_image, min_depth, max_depth):
    """ mask out-of-range pixel to zero """
    # print ('mask min max', min_depth, max_depth)
    ret, depth_image = cv2.threshold(depth_image, min_depth, 100000, cv2.THRESH_TOZERO)
    ret, depth_image = cv2.threshold(depth_image, max_depth, 100000, cv2.THRESH_TOZERO_INV)
    depth_image = np.expand_dims(depth_image, 2)
    return np.squeeze(depth_image, axis=2)

if __name__=="__main__":
    main()
