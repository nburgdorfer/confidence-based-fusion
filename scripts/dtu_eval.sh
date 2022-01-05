#!/bin/bash

# usage statement
function usage {
        echo "Usage: ./$(basename $0) [OPTION]...

OPTIONS:
  -c,           Path to the DTU MVS Matlab evaluation code.
  -d,           Path to the data directory for the scan being fused.
  -f,           Path to the depth fusion executable.
  -p,           Path to the DTU MVS Points directory for evaluation.
  -r,           Path to the DTU MVS Results directory for evalutaion.
"
}

# list of arguments expected in the input
optstring="c:d:f:p:r:h"

# collect all passed argument values
while getopts ${optstring} arg; do
  case ${arg} in
    h)
        usage
        exit 0
        ;;
    c)
        EVAL_CODE_DIR=$OPTARG
        echo "DTU MVS Matlab evaluation code path set to '${EVAL_CODE_DIR}'..."
        ;;
    d)
        DATA_DIR=$OPTARG
        echo "Data path for scan set to '${DATA_DIR}'..."
        ;;
    f)
        FUSION_EX=$OPTARG
        echo "Depth fusion executable path set to '${FUSION_EX}'..."
        ;;
    p)
        EVAL_PC_DIR=$OPTARG
        echo "DTU MVS Points path set to '${EVAL_PC_DIR}'..."
        ;;
    r)
        EVAL_RESULTS_DIR=$OPTARG
        echo "DTU MVS Results path set to '${EVAL_RESULTS_DIR}'..."
        ;;
    ?)
        echo "Invalid option: ${OPTARG}."
        ;;
  esac
done

# set default values if arguments not passed
# default value for option c)
if [ -z $EVAL_CODE_DIR ]; then
    EVAL_CODE_DIR=~/Data/dtu_data_eval/matlab_code/
    echo "DTU MVS Matlab evaluation code path set to default value '${EVAL_CODE_DIR}'..."
fi

# default value for option d)
if [ -z $DATA_DIR ]; then
    DATA_DIR=~/Data/MVSNet/dtu/testing/
    echo "Data path for scan set to default value '${DATA_DIR}'..."
fi

# default value for option f)
if [ -z $FUSION_EX ]; then
    FUSION_EX=~/dev/research/confidence-based-fusion/src/build/depth_fusion
    echo "Depth fusion executable path set to default value '${FUSION_EX}'..."
fi

# default value for option p)
if [ -z $EVAL_PC_DIR ]; then
    EVAL_PC_DIR=~/Data/dtu_data_eval/mvs_data/Points/fusion/
    echo "DTU MVS Points path set to default value '${EVAL_PC_DIR}'..."
fi

# default value for option r)
if [ -z $EVAL_RESULTS_DIR ]; then
    EVAL_RESULTS_DIR=~/Data/dtu_data_eval/mvs_data/Results/
    echo "DTU MVS Results path set to default value '${EVAL_RESULTS_DIR}'..."
fi

echo -e "\n"

SCANS=(1 4 9 10 11 12 13 15 23 24 29 32 33 34 48 49 62 75 77 110 114 118)

for SCAN in ${SCANS[@]}
do
    # run netowrk for each scan
    echo "Running DMFNet on scan ${SCAN}"

    # pad scan number
    printf -v curr_scan_num "%03d" $SCAN
    
    scan_dir=scan${curr_scan_num}_test/

    # create point cloud file name
    pc_file_name=fusion${curr_scan_num}_l3.ply
    echo ${pc_file_name}

    ##### run operations #####
	# remove previous point cloud content
    if [ -d  ${DATA_DIR}${scan_dir}points_fusion/ ]; then
        rm -rf ${DATA_DIR}${scan_dir}points_fusion/*;
	else
		mkdir ${DATA_DIR}${scan_dir}points_fusion/
    fi

    # run depth fusion
    $FUSION_EX ${DATA_DIR}${scan_dir} 5 0.1 0.95 0.01

    # merge output point clouds
    python3 merge_depth_maps.py ${DATA_DIR}${scan_dir}points_fusion/

    # move merged point cloud to evaluation path
    cp ${DATA_DIR}${scan_dir}points_fusion/merged.ply ${EVAL_PC_DIR}${pc_file_name}
done

# delete previous results if 'Results' directory is not empty
if [ "$(ls -A $EVAL_RESULTS_DIR)" ]; then
    rm -r $EVAL_RESULTS_DIR*
fi

USED_SETS="[${SCANS[@]}]"

# run matlab evaluation on merged fusion point cloud
matlab -nodisplay -nosplash -nodesktop -r "clear all; close all; format compact; arg_method='fusion'; UsedSets=${USED_SETS}; run('${EVAL_CODE_DIR}BaseEvalMain_web.m'); clear all; close all; format compact; arg_method='fusion'; UsedSets=${USED_SETS}; run('${EVAL_CODE_DIR}ComputeStat_web.m'); exit;" | tail -n +10
