#!/bin/bash

DATA_DIR=/media/nate/Data/MVSNet/dtu/testing/
EVAL_DIR=/media/nate/Data/Evaluation/dtu/

./dtu_eval.sh -c ${EVAL_DIR}matlab_code/ -d ${DATA_DIR} -p ${EVAL_DIR}mvs_data/Points/fusion/ -r ${EVAL_DIR}mvs_data/Results
