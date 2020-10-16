#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "util.h"


void load_conf_maps(vector<Mat> *conf_maps, string data_path) {
    cout << "Loading confidence maps..." << endl;
    DIR *dir;
    struct dirent *ent;
    char conf_path[256];
    vector<char*> conf_files;

    // load images
    strcpy(conf_path,data_path.c_str());
    strcat(conf_path,"conf_maps/");

    if((dir = opendir(conf_path)) == NULL) {
        fprintf(stderr,"Error: Cannot open directory %s.\n",conf_path);
        exit(EXIT_FAILURE);
    }

    while((ent = readdir(dir)) != NULL) {
        if ((ent->d_name[0] != '.') && (ent->d_type != DT_DIR)) {
            char *conf_filename = (char*) malloc(sizeof(char) * 256);

            strcpy(conf_filename,conf_path);
            strcat(conf_filename,ent->d_name);

            conf_files.push_back(conf_filename);
        }
    }

    // sort files by name
    sort(conf_files.begin(), conf_files.end(), comp);

    int conf_count = conf_files.size();

    for (int i=0; i<conf_count; ++i) {
        /*
        cv::FileStorage fs(conf_files[i], cv::FileStorage::READ);
        Mat map;
        fs["map"] >> map;

        map.convertTo(map,CV_32F);
        */
        Mat map = read_csv(conf_files[i]);
        conf_maps->push_back(map);

        //fs.release();
    }

}

void load_depth_maps(vector<Mat> *depth_maps, string data_path) {
    cout << "Loading depth maps..." << endl;
    DIR *dir;
    struct dirent *ent;
    char depth_path[256];
    vector<char*> depth_files;

    // load images
    strcpy(depth_path,data_path.c_str());
    strcat(depth_path,"depth_maps/");

    if((dir = opendir(depth_path)) == NULL) {
        fprintf(stderr,"Error: Cannot open directory %s.\n",depth_path);
        exit(EXIT_FAILURE);
    }

    while((ent = readdir(dir)) != NULL) {
        if ((ent->d_name[0] != '.') && (ent->d_type != DT_DIR)) {
            char *depth_filename = (char*) malloc(sizeof(char) * 256);

            strcpy(depth_filename,depth_path);
            strcat(depth_filename,ent->d_name);

            depth_files.push_back(depth_filename);
        }
    }

    // sort files by name
    sort(depth_files.begin(), depth_files.end(), comp);

    int depth_count = depth_files.size();

    for (int i=0; i<depth_count; ++i) {
        /*
        cv::FileStorage fs(depth_files[i], cv::FileStorage::READ);
        Mat map;
        fs["map"] >> map;

        map.convertTo(map,CV_32F);
        */
        Mat map = read_csv(depth_files[i]);
        depth_maps->push_back(map);

        //fs.release();
    }
}

/*
 * @brief Loads all files found under 'data_path' into the 'images' container
 *
 * @param images - The container to be populated with the loaded image objects
 * @param data_path - The relative path to the base directory for the data
 *
 */
void load_images(vector<Mat> *images, string data_path) {
    cout << "Loading images..." << endl;
    DIR *dir;
    struct dirent *ent;
    char img_path[256];
    vector<char*> img_files;

    // load images
    strcpy(img_path,data_path.c_str());
    strcat(img_path,"images/");

    if((dir = opendir(img_path)) == NULL) {
        fprintf(stderr,"Error: Cannot open directory %s.\n",img_path);
        exit(EXIT_FAILURE);
    }

    while((ent = readdir(dir)) != NULL) {
        if ((ent->d_name[0] != '.') && (ent->d_type != DT_DIR)) {
            char *img_filename = (char*) malloc(sizeof(char) * 256);

            strcpy(img_filename,img_path);
            strcat(img_filename,ent->d_name);

            img_files.push_back(img_filename);
        }
    }

    // sort files by name
    sort(img_files.begin(), img_files.end(), comp);

    int img_count = img_files.size();

    for (int i=0; i<img_count; ++i) {
        Mat img = imread(img_files[i]);
        img.convertTo(img,CV_32F);
        images->push_back(img);
    }
}

/*
 * @brief Loads in the camera intrinsics and extrinsics
 *
 * @param intrinsics - The container to be populated with the intrinsic matrices for the images
 * @param rotations - The container to be populated with the rotation matrices for the images
 * @param translations - The container to be populated with the translation vectors for the images
 * @param data_path - The relative path to the base directory for the data
 *
 */
void load_camera_params(vector<Mat> *K, vector<Mat> *P, Bounds *bounds, string data_path) {
    cout << "Loading camera parameters..." << endl;
    DIR *dir;
    struct dirent *ent;
    char camera_path[256];
    vector<char*> camera_files;

    FILE *fp;
    char *line=NULL;
    size_t n = 128;
    ssize_t bytes_read;
    char *ptr = NULL;

    // load intrinsics
    strcpy(camera_path,data_path.c_str());
    strcat(camera_path,"cams/");

    if((dir = opendir(camera_path)) == NULL) {
        fprintf(stderr,"Error: Cannot open directory %s.\n",camera_path);
        exit(EXIT_FAILURE);
    }

    while((ent = readdir(dir)) != NULL) {
        if ((ent->d_name[0] != '.') && (ent->d_type != DT_DIR)) {
            char *camera_filename = (char*) malloc(sizeof(char) * 256);

            strcpy(camera_filename,camera_path);
            strcat(camera_filename,ent->d_name);

            camera_files.push_back(camera_filename);
        }
    }

    // sort files by name
    sort(camera_files.begin(), camera_files.end(), comp);

    int camera_count = camera_files.size();

    for (int i=0; i<camera_count; ++i) {
        if ((fp = fopen(camera_files[i],"r")) == NULL) {
            fprintf(stderr,"Error: could not open file %s.\n", camera_files[i]);
            exit(EXIT_FAILURE);
        }

        // throw away 'extrinsic' tag...
        bytes_read = getline(&line, &n, fp);
        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);

        // load P matrix
        Mat P_i(4,4,CV_32F);
        for (int j=0; j<4; ++j) {
            if ((bytes_read = getline(&line, &n, fp)) == -1) {
                fprintf(stderr, "Error: could not read line from %s.\n",camera_files[i]);
            }

            ptr = strstr(line,"\n");
            strncpy(ptr,"\0",1);
            
            char *token = strtok(line," ");
            int ind = 0;
            while (token != NULL) {
                P_i.at<float>(j,ind) = atof(token);

                token = strtok(NULL," ");
                ind++;
            }
        }

        P->push_back(P_i);

        // throw away empty line and 'intrinsic' tag...
        bytes_read = getline(&line, &n, fp);
        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);
        bytes_read = getline(&line, &n, fp);
        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);

        // load K matrix
        Mat K_i(3,3,CV_32F);
        for (int j=0; j<3; ++j) {
            if ((bytes_read = getline(&line, &n, fp)) == -1) {
                fprintf(stderr, "Error: could not read line from %s.\n",camera_files[i]);
            }

            ptr = strstr(line,"\n");
            strncpy(ptr,"\0",1);
            
            char *token = strtok(line," ");
            int ind = 0;
            while (token != NULL) {
                K_i.at<float>(j,ind) = atof(token);

                token = strtok(NULL," ");
                ind++;
            }
        }

        K->push_back(K_i);

        // throw away empty line...
        bytes_read = getline(&line, &n, fp);
        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);

        // load Bounds
        if ((bytes_read = getline(&line, &n, fp)) == -1) {
            fprintf(stderr, "Error: could not read line from %s.\n",camera_files[i]);
        }

        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);
        
        char *token = strtok(line," ");
        int ind = 0;

        bounds->min_dist = atof(token);
        token = strtok(NULL," ");
        bounds->increment = atof(token);

        fclose(fp);
    }
}

/*
 * @brief Loads in the boundary information based on the DTU dataset
 *
 * @param bounds - The container to be populated with the bounds information for the images
 * @param data_path - The relative path to the base directory for the data
 *
 */
void load_bounds(vector<Mat> *bounds, string data_path) {
    cout << "Loading bounds..." << endl;
    DIR *dir;
    struct dirent *ent;
    char bounds_path[256];
    vector<char*> bounds_files;

    FILE *fp;
    char *line=NULL;
    size_t n = 128;
    ssize_t bytes_read;
    char *ptr = NULL;

    // load bounds
    strcpy(bounds_path,data_path.c_str());
    strcat(bounds_path,"bounding/");

    if((dir = opendir(bounds_path)) == NULL) {
        fprintf(stderr,"Error: Cannot open directory %s.\n",bounds_path);
        exit(EXIT_FAILURE);
    }

    while((ent = readdir(dir)) != NULL) {
        if ((ent->d_name[0] != '.') && (ent->d_type != DT_DIR)) {
            char *bounds_filename = (char*) malloc(sizeof(char) * 256);

            strcpy(bounds_filename,bounds_path);
            strcat(bounds_filename,ent->d_name);

            bounds_files.push_back(bounds_filename);
        }
    }

    // sort files by name
    sort(bounds_files.begin(), bounds_files.end(), comp);

    int bounds_count = bounds_files.size();

    for (int i=0; i<bounds_count; ++i) {
        if ((fp = fopen(bounds_files[i],"r")) == NULL) {
            fprintf(stderr,"Error: could not open file %s.\n", bounds_files[i]);
            exit(EXIT_FAILURE);
        }

        // load bounds vectors
        Mat bound(2,1,CV_32F);

        // get min distance
        if ((bytes_read = getline(&line, &n, fp)) == -1) {
            fprintf(stderr, "Error: could not read line from %s.\n",bounds_files[i]);
        }

        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);
        bound.at<float>(0,0) = atof(line);

        // get max dist
        if ((bytes_read = getline(&line, &n, fp)) == -1) {
            fprintf(stderr, "Error: could not read line from %s.\n",bounds_files[i]);
        }

        ptr = strstr(line,"\n");
        strncpy(ptr,"\0",1);
        bound.at<float>(1,0) = atof(line);

        bounds->push_back(bound);

        fclose(fp);
    }
}

float med_filt(const Mat &patch, int filter_width, int num_inliers) {
    vector<float> vals;
    int inliers = 0;
    float initial_val = patch.at<float>((filter_width-1)/2,(filter_width-1)/2);

    for (int r=0; r<filter_width; ++r) {
        for (int c=0; c<filter_width; ++c) {
            if (patch.at<float>(r,c) >= 0) {
                vals.push_back(patch.at<float>(r,c));
                ++inliers;
            }
        }
    }

    sort(vals.begin(), vals.end());
    
    float med;

    if (inliers < num_inliers) {
        med = initial_val;
    } else {
        med = vals[static_cast<int>(inliers/2)];
    }

    return med;
}


float mean_filt(const Mat &patch, int filter_width, int num_inliers) {
    float sum = 0.0;
    int inliers = 0;
    float initial_val = patch.at<float>((filter_width-1)/2,(filter_width-1)/2);

    for (int r=0; r<filter_width; ++r) {
        for (int c=0; c<filter_width; ++c) {
            if (patch.at<float>(r,c) >= 0) {
                sum += patch.at<float>(r,c);
                ++inliers;
            }
        }
    }

    if (inliers < num_inliers) {
        sum = initial_val;
    } else {
        sum /= inliers;
    }

    return sum;
}

void write_ply(const Mat &depth_map, const Mat &K, const Mat &P, const string filename, const vector<int> color) {
    Size size = depth_map.size();

    int crop_val = 0;

    int rows = size.height;
    int cols = size.width;

    vector<Mat> ply_points;

    for (int r=crop_val; r<rows-(crop_val*2); ++r) {
        for (int c=crop_val; c<cols-(crop_val*2); ++c) {
            float depth = depth_map.at<float>(r,c);

            if (depth <= 0) {
                continue;
            }

            // compute corresponding (x,y) locations
            Mat x_1(4,1,CV_32F);
            x_1.at<float>(0,0) = depth * c;
            x_1.at<float>(1,0) = depth * r;
            x_1.at<float>(2,0) = depth;
            x_1.at<float>(3,0) = 1;

            // find 3D world coord of back projection
            Mat cam_coords = K.inv() * x_1;
            Mat X_world = P.inv() * cam_coords;
            X_world.at<float>(0,0) = X_world.at<float>(0,0) / X_world.at<float>(0,3);
            X_world.at<float>(0,1) = X_world.at<float>(0,1) / X_world.at<float>(0,3);
            X_world.at<float>(0,2) = X_world.at<float>(0,2) / X_world.at<float>(0,3);
            X_world.at<float>(0,3) = X_world.at<float>(0,3) / X_world.at<float>(0,3);

            Mat ply_point = Mat::zeros(1,6,CV_32F);

            ply_point.at<float>(0,0) = X_world.at<float>(0,0);
            ply_point.at<float>(0,1) = X_world.at<float>(0,1);
            ply_point.at<float>(0,2) = X_world.at<float>(0,2);

            ply_point.at<float>(0,3) = color[0];
            ply_point.at<float>(0,4) = color[1];
            ply_point.at<float>(0,5) = color[2];

            ply_points.push_back(ply_point);
        }
    }

    ofstream ply_file;
    ply_file.open(filename);
    ply_file << "ply\n";
    ply_file << "format ascii 1.0\n";
    ply_file << "element vertex " << ply_points.size() << "\n";
    ply_file << "property float x\n";
    ply_file << "property float y\n";
    ply_file << "property float z\n";
    ply_file << "property uchar red\n";
    ply_file << "property uchar green\n";
    ply_file << "property uchar blue\n";
    ply_file << "element face 0\n";
    ply_file << "end_header\n";

    vector<Mat>::iterator pt(ply_points.begin());
    for (; pt != ply_points.end(); ++pt) {
        ply_file << pt->at<float>(0,0) << " " << pt->at<float>(0,1) << " " << pt->at<float>(0,2) << " " << pt->at<float>(0,3) << " " << pt->at<float>(0,4) << " " << pt->at<float>(0,5) << "\n";
    }

    ply_file.close();

}

// down-sample images
void down_sample(vector<Mat> *images, vector<Mat> *intrinsics, const float scale) {
    if (scale <= 0) {
        return;
    }
    Size size = (*images)[0].size();
    Size scaled_size;
    scaled_size.height = size.height * scale;
    scaled_size.width = size.width * scale;

    vector<Mat>::iterator img(images->begin());
    vector<Mat>::iterator k(intrinsics->begin());

    for (; img != images->end(); ++img,++k) {
        for (int i = 0; i < scale; ++i) {
            Mat temp_img = Mat::zeros(size, CV_32F);

            resize(*img,temp_img, scaled_size);
            *img = temp_img;
        }

        k->at<float>(0,0) = k->at<float>(0,0)*scale;
        k->at<float>(1,1) = k->at<float>(1,1)*scale;
        k->at<float>(0,2) = k->at<float>(0,2)*scale;
        k->at<float>(1,2) = k->at<float>(1,2)*scale;
    }
}

// down-sample intrinsics
void down_sample_k(vector<Mat> *intrinsics, const float scale) {
    if (scale <= 0) {
        return;
    }

    vector<Mat>::iterator k(intrinsics->begin());

    for (; k != intrinsics->end(); ++k) {
        k->at<float>(0,0) = k->at<float>(0,0)*scale;
        k->at<float>(1,1) = k->at<float>(1,1)*scale;
        k->at<float>(0,2) = k->at<float>(0,2)*scale;
        k->at<float>(1,2) = k->at<float>(1,2)*scale;
    }
}

// up-sample images (used for writing images)
Mat up_sample(const Mat *image, const int scale) {
    Size size = image->size();
    Mat enlarged_img = *image;

    for (int i = 0; i < scale; ++i) {
        Mat temp_img = Mat::zeros(size, CV_32F);
        pyrUp(enlarged_img,temp_img);
        enlarged_img = temp_img;
    }

    return enlarged_img;
}

// Image display utility (scales to [0,255])
void display_map(const Mat map, string filename) {
    Size size = map.size();
    // crop 20 pixels
    Mat cropped = map(Rect(24,24,size.width-50,size.height-50));

    double max;
    double min;
    Point min_loc;
    Point max_loc;
    minMaxLoc(cropped, &min, &max, &min_loc, &max_loc);
    cropped = (cropped)*(255/(max));
    imwrite(filename, cropped);
}

// Image writing utility (stores map)
void write_map(const Mat map, const string filename) {
    /*
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    fs << "map" << map;
    fs.release();
    */
    Size size = map.size();
    int rows = size.height;
    int cols = size.width;

    string curr_line;

    ofstream output;
    output.open(filename);

    for(int r=0; r<rows; ++r) {
        output << map.at<float>(r,0);
        for(int c=1; c<cols; ++c) {
            output << "," << map.at<float>(r,c);
        }
        output << "\n";
    }
    output.close();
}

Mat read_csv(const string file_path) {
    int rows = 0;
    int cols = 0;

    vector<vector<float>> data;
    string curr_line;

    ifstream input(file_path);

    while(getline(input, curr_line)) {
        vector<float> row;
        stringstream line(curr_line);
        string curr_val;

        while(getline(line,curr_val,',')) {
            row.push_back(stof(curr_val));
        }
        data.push_back(row);
    }

    rows = data.size();
    cols = data[0].size();
    Mat map = Mat::zeros(rows,cols,CV_32F);

    for (int r=0; r<rows; ++r) {
        for (int c=0; c<cols; ++c){
            map.at<float>(r,c) = data[r][c];            
        }
    }
    return map;
}
