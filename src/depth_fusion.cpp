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
 * @param depth_maps        - The container holding the depth maps to be fused.
 * @param fused_map		    - The reference to the output fused depth map.
 * @param conf_maps 	    - The container holding the confidence maps needed for the fusion process.
 * @param fused_conf	    - The reference to the output fused confidence map.
 * @param K			        - The container holding the intrinsics for each camera view.
 * @param P			        - The container holding the extrinsics for each cmaera view.
 * @param views			    - The container holding the supporting views for the current view.
 * 				                This is a 2D vector with 'total_views' rows and 'num_views' columns.
 * 				                For example: if we are fusing view #5, 
 * 				                then views[5] is a list of the best supporting views to fuse for view #5.
 * @param index			    - The current view we are fusing.
 * @param data_path		    - The path to the root folder for our data. Used to store the point clouds after fusion.
 * @param conf_pre_filt     - The pre-fusion confidence filter.
 * 				                Pixels with confidence less than this value will not be considered for fusion consensus.
 * @param conf_post_filt    - The post-fusion confidence filter.
 * 				                Pixels with confidence less than this value will become 'holes' in the output fusion map.
 * @param support_ratio		    - The support ratio used to assess whether a depth supports the initial depth estimate.
 * 				                This value is a decimal value indicating the ratio between the support region and the current depth estimate.
 * 				                For example: DTU depth values range from about [450mm-950mm], so a value of 0.01 would produce support regions [4.5mm-9.5mm].
 *
 */
void confidence_fusion(
		const vector<Mat> &depth_maps,
		Mat &fused_map,
		const vector<Mat> &conf_maps,
		Mat &fused_conf,
		const vector<Mat> &images,
		const vector<Mat> &K,
		const vector<Mat> &P,
		const vector<vector<int>> &views,
		const int index,
		const string data_path,
		const float conf_pre_filt,
		const float conf_post_filt,
		const float support_ratio)
{
    int num_views = views[index].size();
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

    // for each supporting view of the current index (reference view)
    for (auto d : views[index]) {
		if(d == index) {
			// push the current view
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

                if(conf < conf_pre_filt) {
                    continue;
                }

                // compute 3D world coord of back projection
                Mat x_1(4,1,CV_32F);
                x_1.at<float>(0,0) = depth * c;
                x_1.at<float>(1,0) = depth * r;
                x_1.at<float>(2,0) = depth;
                x_1.at<float>(3,0) = 1;

                Mat cam_coords = K[d].inv() * x_1;
                Mat X_world = P[d].inv() * cam_coords;
                X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
                X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
                X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
                X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

                // calculate pixel location in reference image
                Mat x_2 = K[index] * P[index] * X_world;

                x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
                x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
                x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

                // take the floor to get the row and column pixel locations
                int c_p = (int) floor(x_2.at<float>(0,0));
                int r_p = (int) floor(x_2.at<float>(1,0));
                
                // ignore if pixel projection falls outside the image
                if (c_p < 0 || c_p >= size.width || r_p < 0 || r_p >= size.height) {
                    continue;
                }

                // calculate the projection depth from reference image plane
                Mat diff = Mat::zeros(3,1,CV_32F);
                diff.at<float>(0,0) = X_world.at<float>(0,0) - C_ref.at<float>(0,0);
                diff.at<float>(0,1) = X_world.at<float>(0,1) - C_ref.at<float>(0,1);
                diff.at<float>(0,2) = X_world.at<float>(0,2) - C_ref.at<float>(0,2);
                diff=R_ref*diff;

                float proj_depth = diff.at<float>(0,2);

                /* 
                 * Keep the closer (smaller) projection depth.
                 * A previous projection could have already populated the current pixel.
                 * If it is 0, no previous projection to this pixel was seen.
                 * Otherwise, we need to overwrite only if the current estimate is closer (smaller value).
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
    int initial_d;

    cout << "\tFusing depth maps..." << endl;

    for (int r=0; r<rows; ++r) {
        for (int c=0; c<cols; ++c) {
            f = 0.0;
            initial_f = 0.0;
            C = 0.0;
            initial_d = 0;

            // take most confident pixel as initial depth estimate
			for (int d=0; d<num_views; ++d) {
                if (conf_refs[d].at<float>(r,c) > C) {
                    f = depth_refs[d].at<float>(r,c);
                    C = conf_refs[d].at<float>(r,c);
                    initial_f = f;
                    initial_d = d;
                }
            }

			// get the appropriate absolute index from the views vector
			int abs_initial_d = views[index][initial_d];

            // Set support region as fraction of initial depth estimate
            float epsilon = support_ratio * initial_f;

			for (int d=0; d<num_views; ++d) {
                // skip computation if this iteration is the initial depth map
                if (d == initial_d) {
                    continue;
                }

				// get the appropriate absolute index from the views vector
				int abs_d = views[index][d];

                // grab current depth and confidence values
                float curr_depth = depth_refs[d].at<float>(r,c);
                float curr_conf = conf_refs[d].at<float>(r,c);

                // if confidence is below the threshold, do not have this pixel contribute
                // TODO: run tests to see how this affects estimation
                //if (curr_conf < 0.1) {
                //    continue;
                //}

                // if depth is within the support region of the initial depth
                if (abs(curr_depth - initial_f) < epsilon) {
                    if((C + curr_conf) != 0) {
                        f = ((f*C) + (curr_depth*curr_conf)) / (C + curr_conf);
                    }
                    C += curr_conf;
                } 
                // if depth is closer than initial estimate (occlusion)
                else if(curr_depth < initial_f) {
                    C -= curr_conf;
                }
                // if depth is farther than initial estimate (free-space violation)
                else if(curr_depth > initial_f) {
                    // compute 3D world coord of back projection
                    Mat x_1(4,1,CV_32F);
                    x_1.at<float>(0,0) = initial_f * c;
                    x_1.at<float>(1,0) = initial_f * r;
                    x_1.at<float>(2,0) = initial_f;
                    x_1.at<float>(3,0) = 1;

                    Mat cam_coords = K[abs_initial_d].inv() * x_1;
                    Mat X_world = P[abs_initial_d].inv() * cam_coords;
                    X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
                    X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
                    X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
                    X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

                    // calculate pixel location in reference image
                    Mat x_2 = K[abs_d] * P[abs_d] * X_world;

                    x_2.at<float>(0,0) = x_2.at<float>(0,0)/x_2.at<float>(2,0);
                    x_2.at<float>(1,0) = x_2.at<float>(1,0)/x_2.at<float>(2,0);
                    x_2.at<float>(2,0) = x_2.at<float>(2,0)/x_2.at<float>(2,0);

                    // take the floor to get the row and column pixel locations
                    int c_p = (int) floor(x_2.at<float>(0,0));
                    int r_p = (int) floor(x_2.at<float>(1,0));
                    
                    // ignore if pixel projection falls outside the image
                    if (c_p >= 0 && c_p < size.width && r_p >= 0 && r_p < size.height) {
                        C -= conf_maps[abs_d].at<float>(r_p,c_p);
                    }
                }
            }

            // bound confidence to interval (0-1)
            // 5 views could all have max contributions 1.0/-1.0, making the max value of C = 5.0/-5.0 for the given pixel
            // TODO: could be a better way to squeeze the confidence value to avoid the effect of outliers... Sigmoid?
            C += num_views;
            C /= (2*num_views);

            // drop any estimates that do not meet the minimum confidence value
            if (C <= conf_post_filt) {
                f = -1.0;
                C = -1.0;
            }

            // set the values for the confidence and depth estimates at the current pixel
            fused_map.at<float>(r,c) = f;
            fused_conf.at<float>(r,c) = C;
        }
    }

    // hole-filling parameters
    int w = 5;
    int w_offset = (w-1)/2;
    int w_inliers = (w*w)/3;

    // smoothing parameters
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

	// pad the index string for filenames
	std::string index_str = to_string(index);
	pad(index_str, 4, '0');

	// write ply files
	write_ply(fused_map, K[index], P[index], data_path+"points_fusion/" + index_str + "_points.ply", images[index]);
	//write_ply(depth_maps[index], K[index], P[index], data_path+"pre_fusion_points/" + index_str + "_points.ply", images[index]);
}

int main(int argc, char **argv) {
    // check for proper command-line usage
    if (argc != 6) {
        fprintf(stderr, "Error: usage %s <path-to-depth-maps> <num-views> <conf-pre-filt> <conf-post-filt> <epsilon>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // read in command-line args
    string data_path = argv[1];
    int num_views = atoi(argv[2]);
    float conf_pre_filt = atof(argv[3]);
    float conf_post_filt = atof(argv[4]);
    float support_ratio = atof(argv[5]);

    size_t str_len = data_path.length();

    // string formatting to add '/' to data_path if it is missing from the input
    if (data_path[str_len-1] != '/') {
        data_path += "/";
    }
    
    vector<Mat> depth_maps;
    vector<Mat> conf_maps;
    vector<Mat> images;
    vector<Mat> K;
    vector<Mat> P;
    Bounds bounds;
    vector<vector<int>> views;
    
    // load maps, views, K's, P's, bounds
    printf("Loading data...\n");
    load_depth_maps(&depth_maps, data_path);
    load_conf_maps(&conf_maps, data_path);
	load_images(&images, data_path);
    load_views(&views, num_views, data_path);
    load_camera_params(&K, &P, &bounds, data_path);

    int depth_map_count = depth_maps.size();
    Size size = depth_maps[0].size();

    /*
     * Augment the intrinsics matrices.
     * This is needed for the depth map rendering process.
     * It is a relatively straight-forward transformation.
     * The process can be seen below:
     *
     *		    | k11  k12  k13 |
     *	K = 	| k21  k22  k23 |
     *		    | k31  k32  k33 |
     *
     *		    | k11  k12  k13  0 |
     *	K_aug = | k21  k22  k23  0 |
     *		    | k31  k32  k33  0 |
     *		    | 0    0    0    1 |
     */
    cout << "Computing Augmented Intrinsics..." << endl;
    vector<Mat> K_aug;

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

    // create containers to be populated with fusion output
    Mat fused_map = Mat::zeros(size, CV_32F);
    Mat fused_conf = Mat::zeros(size, CV_32F);

    // starting and ending index used to select which views to produce fused maps for (default is all views).
    int start_ind = 0;
    int end_ind = depth_map_count;

    for (int i=start_ind; i<end_ind; ++i) {
        printf("Running confidence-based fusion for depth map %d/%d...\n",(i+1)-start_ind,end_ind-start_ind);

        confidence_fusion(
                depth_maps,
		        fused_map,
        		conf_maps,
		        fused_conf,
				images,
	            K_aug,
	            P,
	            views,
	            i,
	            data_path,
	            conf_pre_filt,
	            conf_post_filt,
	            support_ratio);

		/*
        // pad the index string for filenames
        std::string index_str = to_string(i);
        pad(index_str, 4, '0');

	    // save the depth and confidence map outputs in .pfm format
        save_pfm(fused_map, data_path + "output/" + index_str + "_depth_fused.pfm");
        save_pfm(fused_conf, data_path + "output/" + index_str + "_conf_fused.pfm");

        // save the depth and confidence map outputs in .png output for visual display
        display_depth(fused_map, data_path + "output/" + index_str + "_disp_depth_fused.png");
        display_conf(fused_conf, data_path + "output/" + index_str + "_disp_conf_fused.png");
		*/
    }

    return EXIT_SUCCESS;
}
