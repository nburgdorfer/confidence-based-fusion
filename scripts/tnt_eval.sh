#!/bin/bash

# params
TNT_DIR=/media/nate/Data/confidence_fusion/tnt/
COLMAP_DIR=/media/nate/Data/confidence_fusion/tnt/colmap/
EVAL_CODE_DIR=/media/nate/Data/Evaluation/tnt/TanksAndTemples/python_toolbox/evaluation/
FUSION_EX=~/dev/research/confidence-based-fusion/src/build/depth_fusion
ALIGN=alignment.txt
COLMAP_ALIGN=alignment.txt

# display parameters
echo "Data path for Tanks and Temples set to '${TNT_DIR}'..."
echo "Data path for Colmap set to '${COLMAP_DIR}'..."
echo "Depth fusion executable path set to default value '${FUSION_EX}'..."
echo "Tanks and Temples evaluation code path set to '${EVAL_CODE_DIR}'..."
echo "Alignment transformation file set to '${ALIGN}'..."
echo "Colmap alignment transformation file set to '${COLMAP_ALIGN}'..."
echo -e "\n"

evaluate() {
    cd ${EVAL_CODE_DIR}
	echo "\nRunning Tanks and Temples evaluation..."
    python -u run.py --dataset-dir ${EVAL_CODE_DIR}../../../eval_data/${SCENE}/ --traj-path ${TNT_DIR}${SCENE}/cams/camera_pose.log --ply-path ${1}
}


##### Run Tanks and Temples evaluation #####
for SCENE in Ignatius
do
    echo "Working on ${SCENE}..."

	## compute the alignment for mvsnet -> GT
    echo -e "\e[1;33mComputing scene alignmment...\e[0;37m"
	python -u ../tools/alignment/compute_alignment.py \
			-a ${TNT_DIR}${SCENE}/cams/ \
			-b ${COLMAP_DIR}Cameras/${SCENE}/ \
			-t ${COLMAP_DIR}Cameras/${SCENE}/${COLMAP_ALIGN} \
			-o ${TNT_DIR}${SCENE}/cams/${ALIGN}

	## convert mvsnet cameras into .log format (w/ alignment)
    echo -e "\e[1;33mCreating tnt camera file...\e[0;37m"
	python -u ../tools/conversion/convert_to_log.py \
			-d ${TNT_DIR}${SCENE}/cams/ \
			-f mvsnet \
			-a ${TNT_DIR}${SCENE}/cams/${ALIGN} \
			-o ${TNT_DIR}${SCENE}/cams/camera_pose.log

	# remove previous point cloud content
    if [ -d  ${TNT_DIR}${SCENE}/post_fusion_points/ ]; then
        rm -rf ${TNT_DIR}${SCENE}/post_fusion_points/*;
	else
		mkdir ${TNT_DIR}${SCENE}/post_fusion_points/
    fi

    # run depth fusion
    $FUSION_EX ${TNT_DIR}${SCENE}/ 5 0.1 0.95 0.006

    # merge output point clouds
    python merge_depth_maps.py ${TNT_DIR}${SCENE}/post_fusion_points/ ${TNT_DIR}${SCENE}/cams/${ALIGN}
    
	## evaluate point cloud
    evaluate "${TNT_DIR}${SCENE}/post_fusion_points/merged.ply" &
    wait

    echo "Done!"
done
