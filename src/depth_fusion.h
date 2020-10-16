#ifndef _DEPTH_FUSION_H_
#define _DEPTH_FUSION_H_

#include <vector>

using namespace std;
using namespace cv;

void confidence_fusion(const vector<Mat> &depth_maps, const vector<Mat> &conf_maps, const vector<Mat> &K, const vector<Mat> &R, const vector<Mat> &t, const vector<Mat> &bounds, const int index, const int depth_count, const Size shape, const bool dtu, int scale, const string data_path);

#endif
