# Real-Time Visibility-Based Fusion of Depth Maps
An independent implementation of the Confidence-Based fusion method as described by Paul Merrell et al. in their [paper](https://graphics.stanford.edu/%7Epmerrell/Merrell_DepthMapFusion07.pdf), 'Real-Time Visibility-Based Fusion of Depth Maps'.

## Version Information (for reference only)
* **ubuntu**:   20.04
* **g++**:      9.3.0
* **make**:     4.2.1
* **opencv**:   4.5.1

## Instructions

### Installation
* Clone the repository
* Update or install the appropriate versions of the above tools and libraries

### Download
[Here](https://drive.google.com/file/d/18HDeECJ84QH9BA-13W56ByyJepnCGpv5/view?usp=sharing) is an sample dataset. It is structured following the guidelines below and includes data from the [DTU - MVS Data Set](http://roboimagedata.compute.dtu.dk/?page_id=36). This is the dataset that I use in the example below.

### File Structure
The fusion code was written to only produce fused depth maps for a single scene. The fusion of several scenes, as well as any other functionality (such as evaluation) is left as a scripting exercise for the reader.

The expected file structure:
```
  root
    ├─ scene000
    ├─ scene001
    ├─ scene002
    ├─ scene003
    │   ├── cams
    │       ├── 00000000_cam.txt
    │       ├── 00000001_cam.txt
            .
            .
    │   ├── conf_maps
    │       ├── 00000000_conf.pfm
    │       ├── 00000001_conf.pfm
            .
            .
    │   ├── depth_maps
    │       ├── 00000000_depth.pfm
    │       ├── 00000001_depth.pfm
            .
            .
    │   ├── output
    │   ├── post_fusion_points
    │   ├── pre_fusion_points
    │   └── pair.txt
    ├─ scene004
    ├─ scene005
    ├─ scene006               
    .
    .
    .
```

This is the structure that is being used in the current implementation. If modifications are made to the ```util.cpp``` file, the user can use any structure that is compatible with their modifications.

The ***output***, ***post_fusion_points***, and ***pre_fusion_points*** directories are used to store the output from the fusion algorithm.

**NOTE**: These directories need to be initially added to the scene directory manually. The code does not yet check for existance of these directories and does not create them automatically.

### Pair.txt
The ```pair.txt``` file is used to list the 10 best supporting views for each reference view in the scene. The current implementation uses the first 5 views. This file will need to be created if the user does not wish to modify the ```util.cpp``` file. If modified, the user may choose their own method of providing this information to the fusion function.

### Running Fusion
* navigate to the ```src``` directory in the repository,
```
> cd fusion/src
```
* before compiling the code, first check the ```depth_fusion.cpp``` file to see if the appropriate number of cores are being used for the OpenMP loop parallelization. The current implementation takes advantage of 24 CPU cores. Please adjust this number for your host system. This adjustment is isolated to a single line of code (at the time of writing, this is around line 80).
  * For example, if the host system has 8 CPU cores:
  ```
  #pragma omp parallel num_threads(8)
  ```
* create and navigate to a ```build``` directory,
```
> mkdir build && cd build
```
* run ```cmake``` on the parent ```src``` directory,
```
> cmake ..
```
* run ```make``` to compile and link the source code,
```
> make
```

If successful, we can now run the fusion algorithm.

* An example of execution:
```
> ./depth_fusion ~/Data/fusion/dtu/scan9/ 5 0.1 0.8 0.01
```

where ***~/Data/fusion/dtu/scan9/*** is the path to the scene we are fusing, ***5*** is the number of supporting views (including the reference view), ***0.1*** is the pre-fusion confidence threshold, ***0.8*** is the post-fusion confidence threshold, and ***0.01*** defines the support region size (For example: DTU depth values range from about [450mm-950mm], so a value of 0.01 would produce support regions [4.5mm-9.5mm]).

### Output
For each view in the scene, this fusion algorithm produces the following:

* A fused depth map (.pfm)
* A fused confidence map (.pfm)
* A visual of the fused depth map (.png)
* A visual of the fused confidence map (.png)
* A point cloud of the fused depth map (.ply)
* A point cloud of the input depth map (.ply)

This algorithm only produces point clouds for individual views. It is left as an exercise to the user to manipulate or merge point clouds for a given scene.


