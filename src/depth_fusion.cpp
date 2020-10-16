#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <omp.h>
#include <fstream>

#include "util.h"
#include "depth_fusion.h"

/*
 * @brief Performs depth map fusion using the confidence-based notion of a depth estimate
 *
 * @param depth_maps - The container holding the depth maps to be fused
 * @param conf_maps - The container holding the confidence maps needed for the fusion process
 *
 */
void confidence_fusion(const vector<Mat> &depth_maps, Mat &fused_map, const vector<Mat> &conf_maps, Mat &fused_conf, const vector<Mat> &K, const vector<Mat> &P, const int index, const float scale, const string data_path){
    int depth_map_count = depth_maps.size();
    Size size = depth_maps[0].size();

    cout << "\tRendering depth maps into reference view..." << endl;
    // calculate reference camera center
    Mat R_ref = P[index](Rect(0,0,3,3));
    Mat t_ref = P[index](Rect(3,0,1,3));
    Mat C_ref = -R_ref.t()*t_ref;

    vector<Mat> depth_refs;
    vector<Mat> conf_refs;
    const int rows = size.height;
    const int cols = size.width;

    for (int d=0; d < depth_map_count; ++d) {
        if (d==index) {
            depth_refs.push_back(depth_maps[index]);
            conf_refs.push_back(conf_maps[index]);
            continue;
        }

        Mat depth_ref = Mat::zeros(size, CV_32F);
        Mat conf_ref = Mat::zeros(size, CV_32F);

#pragma omp parallel num_threads(24)
{
        #pragma omp for collapse(2)
        for (int r=0; r<rows; ++r) {
            for (int c=0; c<cols; ++c) {
                float depth = depth_maps[d].at<float>(r,c);
                float conf = conf_maps[d].at<float>(r,c);

                // compute corresponding (u,v) locations
                Mat x_1(4,1,CV_32F);
                x_1.at<float>(0,0) = c;
                x_1.at<float>(1,0) = r;
                x_1.at<float>(2,0) = 1;
                x_1.at<float>(3,0) = 1/depth;

                // find 3D world coord of back projection
                Mat cam_coords = K[d].inv() * x_1;
                Mat X_world = P[d].inv() * cam_coords;
                X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
                X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
                X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
                X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

                // find pixel location in reference image
                Mat x_2 = K[index] * P[index] * X_world;

                x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
                x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
                x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

                int c_p = (int) floor(x_2.at<float>(0,0));
                int r_p = (int) floor(x_2.at<float>(1,0));
                
                if (c_p < 0 || c_p >= size.width || r_p < 0 || r_p >= size.height) {
                    continue;
                }


                Mat diff = Mat::zeros(3,1,CV_32F);
                diff.at<float>(0,0) = X_world.at<float>(0,0) - C_ref.at<float>(0,0);
                diff.at<float>(0,1) = X_world.at<float>(0,1) - C_ref.at<float>(0,1);
                diff.at<float>(0,2) = X_world.at<float>(0,2) - C_ref.at<float>(0,2);
                diff=R_ref*diff;

                float proj_depth = diff.at<float>(0,2);
                //float proj_depth = X_world.at<float>(0,2) - C_ref.at<float>(0,2);
                /*
                if(index==0 && c==100 && r==100){
                    cout << d << ": " << c_p << "," << r_p << endl;
                    cout << diff << endl;
                    cout << X_world << endl;
                    cout << "dp: " << depth << endl;
                    cout << "gt: " << depth_maps[index].at<float>(88,109) << endl;
                    cout << "pd: " << proj_depth << endl;
                    cout << "cf: " << conf << endl << endl;
                }
                */

                if (depth_ref.at<float>(r_p,c_p) > 0) {
                    if(depth_ref.at<float>(r_p,c_p) > proj_depth) {
                        depth_ref.at<float>(r_p,c_p) = proj_depth;
                        conf_ref.at<float>(r_p,c_p) = conf;
                    }
                } else {
                    depth_ref.at<float>(r_p,c_p) = proj_depth;
                    conf_ref.at<float>(r_p,c_p) = conf;
                }
            }
        }
} //omp parallel

        depth_refs.push_back(depth_ref);
        conf_refs.push_back(conf_ref);
    }

    // Fuse depth maps
    float f;
    float initial_f;
    float C;
    float eps = 50;
    float C_thresh = 0.1;

    cout << "\tFusing depth maps..." << endl;
    for (int r=0; r<rows; ++r) {
        for (int c=0; c<cols; ++c) {
            // set initial depth estimate and confidence value
            f = 0.0;
            initial_f = 0.0;
            C = 0.0;
            int initial_d = 0;

            for (int d=0; d < depth_map_count; ++d) {
                if (conf_refs[d].at<float>(r,c) > C) {
                    f = depth_refs[d].at<float>(r,c);
                    C = conf_refs[d].at<float>(r,c);
                    initial_d = d;
                }
            }

            // store the initial depth value
            initial_f = f;

            // for each depth map:
            for (int d=0; d < depth_map_count; ++d) {
                // skip checking for the most confident depth map
                if (d == initial_d) {
                    continue;
                }

                float curr_depth = depth_refs[d].at<float>(r,c);
                float curr_conf = conf_refs[d].at<float>(r,c);

                if (curr_conf < C_thresh) {
                    continue;
                }

                // if depth is close to initial depth:
                if (abs(curr_depth - initial_f) < eps) {
                    if((C + curr_conf) != 0) {
                        f = ((f*C) + (curr_depth*curr_conf)) / (C + curr_conf);
                    }
                    C += curr_conf;
                } 
                // if depth is too close (occlusion):
                else if(curr_depth < initial_f) {
                    C -= curr_conf;
                }
                // if depth is too large (free space violation):
                else if(curr_depth > initial_f) {
                    C -= conf_refs[initial_d].at<float>(r,c);
                }
            }

            if (C <= 0.0) {
                f = -1.0;
                C = -1.0;
            }
            fused_map.at<float>(r,c) = f;
            fused_conf.at<float>(r,c) = C;
        }
    }

    int w = 5;
    int w_offset = (w-1)/2;
    int w_inliers = (w*w)/4;

    int w_s = 3;
    int w_s_offset = (w_s-1)/2;
    int w_s_inliers = (w_s*w_s)/4;

    Mat filled_map = Mat::zeros(size, CV_32F);
    Mat smoothed_map = Mat::zeros(size, CV_32F);

    // Fill in holes (-1 values) in depth map
    for (int r=w_offset; r<rows-w_offset; ++r) {
        for (int c=w_offset; c<cols-w_offset; ++c) {
            if (fused_map.at<float>(r,c) < 0.0){
                filled_map.at<float>(r,c) = med_filt(fused_map(Rect(c-w_offset,r-w_offset,w,w)), w, w_inliers);
            } else {
                filled_map.at<float>(r,c) = fused_map.at<float>(r,c);
            }
        }
    }
    // Smooth out inliers
    for (int r=w_s_offset; r<rows-w_s_offset; ++r) {
        for (int c=w_s_offset; c<cols-w_s_offset; ++c) {
            if (filled_map.at<float>(r,c) != -1){
                smoothed_map.at<float>(r,c) = med_filt(filled_map(Rect(c-w_s_offset,r-w_s_offset,w_s,w_s)), w_s, w_s_inliers);
            }
        }
    }

    fused_map = smoothed_map;
    //fused_map = filled_map;

    //TEST
/*
    // image 28
    if(index == 0) {
        int x = 222;
        int y = 46;
        float depth = fused_map.at<float>(y,x);

        // compute corresponding (x,y) locations
        Mat x_1(4,1,CV_32F);
        x_1.at<float>(0,0) = x*depth;
        x_1.at<float>(1,0) = y*depth;
        x_1.at<float>(2,0) = depth;
        x_1.at<float>(3,0) = 1;

        // find 3D world coord of back projection
        Mat cam_coords = K_fr[index].inv() * x_1;
        Mat X_world = P[index].inv() * cam_coords;
        X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
        X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
        X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
        X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

        cout << "(" << X_world.at<float>(0,0) << "," << X_world.at<float>(0,1) << "," << X_world.at<float>(0,2) << ")" << endl;

        Mat x_w(4,1,CV_32F);
        x_w.at<float>(0,0) = 126.24;
        x_w.at<float>(1,0) = -202.64;
        x_w.at<float>(2,0) = 667.31;
        x_w.at<float>(3,0) = 1;

        // find pixel location in reference image
        Mat x_2 = K_fr[index] * P[index] * x_w;

        x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
        x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
        x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

        int c_p = (int) floor(x_2.at<float>(0,0));
        int r_p = (int) floor(x_2.at<float>(1,0));

        cout << "(" << c_p << "," << r_p << ")" << endl << endl;
        
    }

    // image 29
    if(index == 1) {
        int x = 249;
        int y = 50;
        float depth = fused_map.at<float>(y,x);

        // compute corresponding (x,y) locations
        Mat x_1(4,1,CV_32F);
        x_1.at<float>(0,0) = x*depth;
        x_1.at<float>(1,0) = y*depth;
        x_1.at<float>(2,0) = depth;
        x_1.at<float>(3,0) = 1;

        // find 3D world coord of back projection
        Mat cam_coords = K_fr[index].inv() * x_1;
        Mat X_world = P[index].inv() * cam_coords;
        X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
        X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
        X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
        X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

        cout << "(" << X_world.at<float>(0,0) << "," << X_world.at<float>(0,1) << "," << X_world.at<float>(0,2) << ")" << endl;

        Mat x_w(4,1,CV_32F);
        x_w.at<float>(0,0) = 126.24;
        x_w.at<float>(1,0) = -202.64;
        x_w.at<float>(2,0) = 667.31;
        x_w.at<float>(3,0) = 1;

        // find pixel location in reference image
        Mat x_2 = K_fr[index] * P[index] * x_w;

        x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
        x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
        x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

        int c_p = (int) floor(x_2.at<float>(0,0));
        int r_p = (int) floor(x_2.at<float>(1,0));

        cout << "(" << c_p << "," << r_p << ")" << endl << endl;
        
    }

    // image 30
    if(index == 2) {
        int x = 275;
        int y = 54;
        float depth = fused_map.at<float>(y,x);

        cout << K[index].at<float>(0,2) <<endl; 
        cout << K[index].at<float>(1,2) <<endl; 
        // compute corresponding (x,y) locations
        Mat x_1(4,1,CV_32F);
        x_1.at<float>(0,0) = (depth*x) - K[index].at<float>(0,2);
        x_1.at<float>(1,0) = (depth*y) - K[index].at<float>(1,2);
        x_1.at<float>(2,0) = (depth);
        x_1.at<float>(3,0) = 1;


        // find 3D world coord of back projection
        Mat cam_coords = K_fr[index].inv() * x_1;
        Mat X_world = P[index].inv() * cam_coords;
        X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
        X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
        X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
        X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

        cout << "(" << X_world.at<float>(0,0) << "," << X_world.at<float>(0,1) << "," << X_world.at<float>(0,2) << ")" << endl;

        Mat x_w(4,1,CV_32F);
        x_w.at<float>(0,0) = 126.24;
        x_w.at<float>(1,0) = -202.64;
        x_w.at<float>(2,0) = 667.31;
        x_w.at<float>(3,0) = 1;

        // find pixel location in reference image
        Mat x_2 = K_fr[index] * P[index] * x_w;

        x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
        x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
        x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

        int c_p = (int) floor(x_2.at<float>(0,0));
        int r_p = (int) floor(x_2.at<float>(1,0));

        cout << "(" << c_p << "," << r_p << ")" << endl << endl;
        
    }
*/
    //TEST

    // write ply file
    vector<int> gray = {200, 200, 200};
    vector<int> green = {0, 255, 0};
    if(index<10){
        write_ply(fused_map, K[index], P[index], data_path+"points_mvs/0"+to_string(index)+"_points.ply", gray);
        write_ply(depth_maps[index], K[index], P[index], data_path+"points_unfused/0"+to_string(index)+"_points.ply", green);
    } else {
        write_ply(fused_map, K[index], P[index], data_path+"points_mvs/"+to_string(index)+"_points.ply", gray);
        write_ply(depth_maps[index], K[index], P[index], data_path+"points_unfused/"+to_string(index)+"_points.ply", green);
    }
}


int main(int argc, char **argv) {
    
    if (argc != 3) {
        fprintf(stderr, "Error: usage %s <path-to-depth-maps> <down-sample-scale>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    string data_path = argv[1];
    float scale = atof(argv[2]);
    size_t str_len = data_path.length();

    if (data_path[str_len-1] != '/') {
        data_path += "/";
    }
    
    vector<Mat> depth_maps;
    vector<Mat> conf_maps;
    vector<Mat> K;
    vector<Mat> P;
    Bounds bounds;
    
    // load maps, K's, P's, bounds
    printf("Loading data...\n");
    load_depth_maps(&depth_maps, data_path);
    load_conf_maps(&conf_maps, data_path);
    load_camera_params(&K, &P, &bounds, data_path);

    down_sample_k(&K, scale);

    int depth_map_count = depth_maps.size();
    Size size = depth_maps[0].size();

    cout << "Computing Augmented Intrinsics..." << endl;
    vector<Mat> K_aug;
    // augment intrinsics
    for (int i=0; i<depth_map_count; ++i) {
        Mat K_i;
        K_i.push_back(K[i].t());

        Mat temp2;
        temp2.push_back(Mat::zeros(1,3,CV_32F));
        K_i.push_back(temp2);
        K_i = K_i.t();

        Mat temp3;
        temp3.push_back(Mat::zeros(3,1,CV_32F));
        temp3.push_back(Mat::ones(1,1,CV_32F));
        K_i.push_back(temp3.t());

        K_aug.push_back(K_i);
    }

    Mat fused_map = Mat::zeros(size, CV_32F);
    Mat fused_conf = Mat::zeros(size, CV_32F);

    //stability_fusion(depth_maps, confidence_maps);
    for (int i=0; i<depth_map_count; ++i) {
        printf("Running confidence-based fusion for depth map %d/%d...\n",(i+1),depth_map_count);

        confidence_fusion(depth_maps, fused_map, conf_maps, fused_conf, K_aug, P, i, scale, data_path);

        if(i<10){
            write_map(fused_map, data_path + "fused_maps/0" + to_string(i) + "_depth_fused.csv");
            write_map(fused_conf, data_path + "fused_confs/0" + to_string(i) + "_conf_fused.csv");
        } else {
            write_map(fused_map, data_path + "fused_maps/" + to_string(i) + "_depth_fused.csv");
            write_map(fused_conf, data_path + "fused_confs/" + to_string(i) + "_conf_fused.csv");
        }

        // display visual for fused depths and confs
        /*
        if(i<10){
            display_map(fused_map, "disp_depth_fused_0" + to_string(i) + ".png");
            display_map(fused_conf, "disp_conf_fused_0" + to_string(i) + ".png");
        } else {
            display_map(fused_map, "disp_depth_fused_" + to_string(i) + ".png");
            display_map(fused_conf, "disp_conf_fused_" + to_string(i) + ".png");
        }
        */
    }

    return EXIT_SUCCESS;
}
