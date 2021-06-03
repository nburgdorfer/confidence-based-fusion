#ifndef _DEPTH_FUSION_H_
#define _DEPTH_FUSION_H_

#include <vector>

using namespace std;
using namespace cv;

void confidence_fusion(const vector<Mat> &depth_maps, Mat &fused_map, const vector<Mat> &conf_maps, Mat &fused_conf, const vector<Mat> &K, const vector<Mat> &P, const vector<vector<int>> &views, const int index, const string data_path);

#endif
