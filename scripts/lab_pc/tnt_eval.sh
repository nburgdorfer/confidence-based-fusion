#!/bin/bash

DATA_DIR=/media/nate/Data/MVSNet/tanks_and_temples/training/
EVAL_CODE_DIR=/media/nate/Data/Evaluation/tanks_and_temples/TanksAndTemples/python_toolbox/evaluation/

./tnt_eval.sh -d ${DATA_DIR} -z ${EVAL_CODE_DIR}
