#ifndef _UTIL_H_
#define _UTIL_H_

#include <vector>

using namespace std;
using namespace cv;

// string comparison function
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

// string padding function
inline void pad(std::string &str, const int width, const char c) {
    str.insert(str.begin(), width-str.length(), c);
}

// structure to hold depth bounds info
struct Bounds {
    float min_dist;
    float increment;
};

// filtering functions
float med_filt(const Mat &patch, int filter_width, int num_inliers);
float mean_filt(const Mat &patch, int filter_width, int num_inliers);

// loading functions
void load_conf_maps(vector<Mat> *conf_maps, string data_path);
void load_depth_maps(vector<Mat> *depth_maps, string data_path);
void load_images(vector<Mat> *images, string data_path);
void load_views(vector<vector<int>> *views, const int num_views, string data_path);
void load_camera_params(vector<Mat> *K, vector<Mat> *P, Bounds *bounds, string data_path);
Mat load_pfm(const string filePath);

// storage functions
void write_ply(const Mat &depth_map, const Mat &K, const Mat &P, const string filename, const Mat &image);
void display_depth(const Mat map, string filename);
void display_conf(const Mat map, string filename);
bool save_pfm(const cv::Mat image, const std::string filePath);

#endif
