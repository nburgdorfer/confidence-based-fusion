#ifndef _UTIL_H_
#define _UTIL_H_

#include <vector>

using namespace std;
using namespace cv;

// String comparison function
inline bool comp(char *a, char *b) {
    int res = strcmp(a,b);
    bool ret_val;

    if (res < 0) {
        ret_val = true;
    } else {
        ret_val = false;
    }

    return ret_val;
}

struct Bounds {
    float min_dist;
    float increment;
};

void load_conf_maps(vector<Mat> *conf_maps, string data_path);
void load_depth_maps(vector<Mat> *depth_maps, string data_path);
void load_images(vector<Mat> *images, string data_path);
void load_camera_params(vector<Mat> *K, vector<Mat> *P, Bounds *bounds, string data_path);
void load_bounds(vector<Mat> *bounds, string data_path);
float med_filt(const Mat &patch, int filter_width, int num_inliers);
float mean_filt(const Mat &patch, int filter_width, int num_inliers);
void write_ply(const Mat &depth_map, const Mat &K, const Mat &P, const string filename, const vector<int> color);
void down_sample(vector<Mat> *images, vector<Mat> *intrinsics, const float scale);
void down_sample_k(vector<Mat> *intrinsics, const float scale);
Mat up_sample(const Mat *image, const int scale);
void display_map(const Mat map, string filename);
void write_map(const Mat map, string filename);
Mat read_csv(const string file_path);

#endif
