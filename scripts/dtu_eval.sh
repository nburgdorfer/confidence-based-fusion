#!/bin/bash

compile() {
	echo -e "\e[1;33mCompiling code...\e[0;37m"
	SCRIPT_PATH=$(pwd)

	cd ${BUILD_DIR}
	cmake .. # > /dev/null 2>&1
	make

	cd ${SCRIPT_PATH}
	echo -e ""
}


run() {
	if [ ! -d  ${OUTPUT_DIR} ]; then
		mkdir -p ${OUTPUT_DIR};
	fi


	SCANS=(1 4 9 10 11 12 13 15 23 24 29 32 33 34 48 49 62 75 77 110 114 118)

	for SCAN in ${SCANS[@]}
	do
		# run netowrk for each scan
		echo "Running conventional fusion on scan ${SCAN}"
		printf -v curr_scan_num "%03d" $SCAN
		SCAN_DIR=scan${curr_scan_num}/

		if [ ! -d  ${OUTPUT_DIR}${SCAN_DIR} ]; then
			mkdir -p ${OUTPUT_DIR}${SCAN_DIR};
		fi
		if [ ! -d  "${OUTPUT_DIR}${SCAN_DIR}depths/" ]; then
			mkdir -p "${OUTPUT_DIR}${SCAN_DIR}depths/";
		fi
		if [ ! -d  "${OUTPUT_DIR}${SCAN_DIR}confs/" ]; then
			mkdir -p "${OUTPUT_DIR}${SCAN_DIR}confs/";
		fi

		# run depth fusion
		$FUSION_EXE ${DATA_DIR} ${OUTPUT_DIR}${SCAN_DIR} ${SCAN_DIR} 5 0.0 0.0 0.005
	done
}



BUILD_DIR=../src/build/
FUSION_EXE=${BUILD_DIR}depth_fusion
compile


NETWORK=mvsnet
DATA_DIR=/media/nate/Data/Fusion/dtu/${NETWORK}/
OUTPUT_DIR=/media/nate/Data/Results/C-FUSE/Output_${NETWORK}/
run


NETWORK=ucsnet
DATA_DIR=/media/nate/Data/Fusion/dtu/${NETWORK}/
OUTPUT_DIR=/media/nate/Data/Results/C-FUSE/Output_${NETWORK}/
run


NETWORK=npcvp_dtu
DATA_DIR=/media/nate/Data/Fusion/dtu/${NETWORK}/
OUTPUT_DIR=/media/nate/Data/Results/C-FUSE/Output_${NETWORK}/
run


NETWORK=gbinet
DATA_DIR=/media/nate/Data/Fusion/dtu/${NETWORK}/
OUTPUT_DIR=/media/nate/Data/Results/C-FUSE/Output_${NETWORK}/
run
