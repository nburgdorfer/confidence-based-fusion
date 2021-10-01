#!/bin/bash

# usage statement
function usage {
        echo "Usage: ./$(basename $0) [OPTION]...

OPTIONS:
  -f,           Path to the depth fusion executable.
  -d,           Path to the data directory for the scan being fused.
  -m,           Path to the MVSNet data directory for the scan being fused.
  -p,           Path to the DTU MVS Points directory for evaluation.
  -q,           Path to the DTU Fusion Points directory for evaluation.
  -r,           Path to the DTU MVS Results directory for evalutaion.
  -c,           Path to the DTU MVS Matlab evaluation code."
}

# list of arguments expected in the input
optstring="f:d:m:p:q:r:c:h"

# collect all passed argument values
while getopts ${optstring} arg; do
  case ${arg} in
    h)
        usage
        exit 0
        ;;
    f)
        FUSION_EX=$OPTARG
        echo "Depth fusion executable path set to '${FUSION_EX}'..."
        ;;
    d)
        DATA_DIR=$OPTARG
        echo "Data path for scan set to '${DATA_DIR}'..."
        ;;
    m)
        MVSNET_DIR=$OPTARG
        echo "Data path for MVSNet scan set to '${MVSNET_DIR}'..."
        ;;
    p)
        EVAL_PC_DIR_MVSNET=$OPTARG
        echo "DTU MVS Points path set to '${EVAL_PC_DIR_MVSNET}'..."
        ;;
    q)
        EVAL_PC_DIR_FUSION=$OPTARG
        echo "DTU Fusion Points path set to '${EVAL_PC_DIR_FUSION}'..."
        ;;
    r)
        EVAL_RESULTS_DIR=$OPTARG
        echo "DTU MVS Results path set to '${EVAL_RESULTS_DIR}'..."
        ;;
    c)
        EVAL_CODE_DIR=$OPTARG
        echo "DTU MVS Matlab evaluation code path set to '${EVAL_CODE_DIR}'..."
        ;;
    ?)
        echo "Invalid option: ${OPTARG}."
        ;;
  esac
done

# set default values if arguments not passed
if [ -z $FUSION_EX ]; then
    FUSION_EX=~/dev/research/confidence-based-fusion/src/build/depth_fusion
    echo "Depth fusion executable path set to default value '${FUSION_EX}'..."
fi
if [ -z $DATA_DIR ]; then
    DATA_DIR=~/Data/fusion/dtu/
    echo "Data path for scan set to default value '${DATA_DIR}'..."
fi
if [ -z $MVSNET_DIR ]; then
    MVSNET_DIR=~/Data/MVSNet/testing/
    echo "Data path for MVSNet scan set to default value '${MVSNET_DIR}'..."
fi
if [ -z $EVAL_PC_DIR_MVSNET ]; then
    EVAL_PC_DIR_MVSNET=~/Data/dtu_data_eval/mvs_data/Points/mvsnet/
    echo "DTU MVS Points path set to default value '${EVAL_PC_DIR_MVSNET}'..."
fi
if [ -z $EVAL_PC_DIR_FUSION ]; then
    EVAL_PC_DIR_FUSION=~/Data/dtu_data_eval/mvs_data/Points/fusion/
    echo "DTU Fusion Points path set to default value '${EVAL_PC_DIR_FUSION}'..."
fi
if [ -z $EVAL_RESULTS_DIR ]; then
    EVAL_RESULTS_DIR=~/Data/dtu_data_eval/mvs_data/Results/
    echo "DTU MVS Results path set to default value '${EVAL_RESULTS_DIR}'..."
fi
if [ -z $EVAL_CODE_DIR ]; then
    EVAL_CODE_DIR=~/Data/dtu_data_eval/matlab_code/
    echo "DTU MVS Matlab evaluation code path set to default value '${EVAL_CODE_DIR}'..."
fi


# grab fusion point cloud folder name
pc_tail_fusion=${EVAL_PC_DIR_FUSION#*Points/}
pc_dir_name_fusion=${pc_tail_fusion%/*}

# grab mvsnet point cloud folder name
pc_tail_mvsnet=${EVAL_PC_DIR_MVSNET#*Points/}
pc_dir_name_mvsnet=${pc_tail_mvsnet%/*}

for SCAN in 1 4 9 10 11 12 13 15 23 24 29 32 33 34 48 49 62 75 77 110 114 118
#for SCAN in 1 6 9 
#for SCAN in 9 
do
    # run netowrk for each scan
    echo "Running DMFNet on scan ${SCAN}"

    # pad scan number
    printf -v curr_scan_num "%03d" $SCAN
    
    mvs_scan_dir=scan${curr_scan_num}_test/
    fusion_scan_dir=scan${SCAN}/

    # create point cloud file name
    pc_file_name_fusion=${pc_dir_name_fusion}${curr_scan_num}_l3.ply
    echo ${pc_file_name_fusion}

    # create point cloud file name
    pc_file_name_mvsnet=${pc_dir_name_mvsnet}${curr_scan_num}_l3.ply
    echo ${pc_file_name_mvsnet}

    ##### run operations #####
    # migrate maps to fusion path
    python3 migrate_maps.py ${MVSNET_DIR}${mvs_scan_dir}depths_mvsnet/ ${DATA_DIR}${fusion_scan_dir}

    # run depth fusion
    $FUSION_EX ${DATA_DIR}${fusion_scan_dir} 5 0.1 0.8 0.01

    # merge output point clouds
    python3 merge_depth_maps.py ${DATA_DIR}${fusion_scan_dir}post_fusion_points/

    # move merged point cloud to evaluation path
    cp ${DATA_DIR}${fusion_scan_dir}post_fusion_points/merged.ply ${EVAL_PC_DIR_FUSION}${pc_file_name_fusion}

    #### MVSNet original maps eval ####
    # merge output point clouds
    python3 merge_depth_maps.py ${DATA_DIR}${fusion_scan_dir}pre_fusion_points/

    # move merged point cloud to evaluation path
    cp ${DATA_DIR}${fusion_scan_dir}pre_fusion_points/merged.ply ${EVAL_PC_DIR_MVSNET}${pc_file_name_mvsnet}
done

# delete previous results if 'Results' directory is not empty
if [ "$(ls -A $EVAL_RESULTS_DIR)" ]; then
    rm -r $EVAL_RESULTS_DIR*
fi

# run matlab evaluation on merged fusion point cloud
matlab -nodisplay -nosplash -nodesktop -r "clear all; close all; format compact; arg_method='${pc_dir_name_fusion}'; run('${EVAL_CODE_DIR}BaseEvalMain_web.m'); clear all; close all; format compact; arg_method='${pc_dir_name_fusion}'; run('${EVAL_CODE_DIR}ComputeStat_web.m'); exit;" | tail -n +10

# don't need to run base mvsnet depth map eval
exit 0

# delete previous results if 'Results' directory is not empty
if [ "$(ls -A $EVAL_RESULTS_DIR)" ]; then
    rm -r $EVAL_RESULTS_DIR*
fi

# run matlab evaluation on merged mvsnet point cloud
matlab -nodisplay -nosplash -nodesktop -r "clear all; close all; format compact; arg_method='${pc_dir_name_mvsnet}'; run('${EVAL_CODE_DIR}BaseEvalMain_web.m'); clear all; close all; format compact; arg_method='${pc_dir_name_mvsnet}'; run('${EVAL_CODE_DIR}ComputeStat_web.m'); exit;" | tail -n +10
