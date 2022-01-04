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

DATA_DIR=/media/nate/Data/MVSNet/dtu/testing/
EVAL_DIR=/media/nate/Data/Evaluation/dtu/

./dtu_eval.sh -c ${EVAL_DIR}matlab_code/ -d ${DATA_DIR} -p ${EVAL_DIR}mvs_data/Points/ -r ${EVAL_DIR}mvs_data/Results
