#!/bin/bash

# usage statement
function usage {
        echo "Usage: ./$(basename $0) [OPTION]...

OPTIONS:
  -d,           Path to the MVSNet TNT data directory.
  -f,           Path to the depth fusion executable.
  -m,           Path to the saved network model.
  -s,           Path to the MVSNet source code directory.
  -z,           Path to the Tanks and Temples evaluation code.
  "
}

# list of arguments expected in the input
optstring="d:f:m:s:z:h"

# collect all passed argument values
while getopts ${optstring} arg; do
  case ${arg} in
    h)
        usage
        exit 0
        ;;
    d)
        MVSNET_DIR=$OPTARG
        echo "Data path for MVSNet TNT set to '${MVSNET_DIR}'..."
        ;;
    f)
        FUSION_EX=$OPTARG
        echo "Depth fusion executable path set to '${FUSION_EX}'..."
        ;;
    m)
        MODEL=$OPTARG
        echo "Network model file set to '${MODEL}'..."
        ;;
    s)
        SRC_DIR=$OPTARG
        echo "Path to the MVSNet source code set to '${SRC_DIR}'..."
        ;;
    z)
        EVAL_CODE_DIR=$OPTARG
        echo "Tanks and Temples evaluation code path set to '${EVAL_CODE_DIR}'..."
        ;;
    ?)
        echo "Invalid option: ${OPTARG}."
        ;;
  esac
done


# set default values if arguments not passed
# set default option -d
if [ -z $MVSNET_DIR ]; then
    MVSNET_DIR=~/Data/MVSNet/tanks_and_temples/training/
    echo "Data path for MVSNet TNT set to default value '${MVSNET_DIR}'..."
fi

# default value for option -f
if [ -z $FUSION_EX ]; then
    FUSION_EX=~/dev/research/confidence-based-fusion/src/build/depth_fusion
    echo "Depth fusion executable path set to default value '${FUSION_EX}'..."
fi

# set default option -m
if [ -z $MODEL ]; then
    MODEL=~/Data/MVSNet/models/3DCNNs/model.ckpt
    echo "Model set to default value '${MODEL}'..."
fi

# set default for option -z
if [ -z $EVAL_CODE_DIR ]; then
    EVAL_CODE_DIR=~/Data/Evaluation/tanks_and_temples/TanksAndTemples/python_toolbox/evaluation/
    echo "Tanks and Temples evaluation code path set to default value '${EVAL_CODE_DIR}'..."
fi

echo -e "\n"

evaluate() {
    cd ${EVAL_CODE_DIR}
	echo "\nRunning Tanks and Temples evaluation..."
    python -u run.py --dataset-dir ${EVAL_CODE_DIR}../../../eval_data/${SCENE}/ --traj-path ${MVSNET_DIR}${SCENE}/cams/camera_pose.log --ply-path ${1}
}


##### Run Tanks and Temples evaluation #####
for SCENE in Barn Ignatius Truck
do
    echo "Working on ${SCENE}..."

	# remove previous point cloud content
    if [ -d  ${MVSNET_DIR}${SCENE}/points_fusion/ ]; then
        rm -rf ${MVSNET_DIR}${SCENE}/points_fusion/*;
	else
		mkdir ${MVSNET_DIR}${SCENE}/points_fusion/
    fi

    # run depth fusion
    $FUSION_EX ${MVSNET_DIR}${SCENE}/ 5 0.1 0.95 0.02

    # merge output point clouds
    python merge_depth_maps.py ${MVSNET_DIR}${SCENE}/points_fusion/
    
	## evaluate point cloud
    evaluate "${MVSNET_DIR}${SCENE}/points_fusion/merged.ply" &
    wait

    echo "Done!"
done
