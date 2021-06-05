# Real-Time Visibility-Based Fusion of Depth Maps
An independent implementation of the Confidence-Based fusion method as described by Paul Merrell et al. in the [paper](https://graphics.stanford.edu/%7Epmerrell/Merrell_DepthMapFusion07.pdf), 'Real-Time Visibility-Based Fusion of Depth Maps'.

## Version Information (for reference only)
* **ubuntu**:   20.04
* **g++**:      9.3.0
* **make**:     4.2.1
* **opencv**:   4.5.1

## Instructions

### Installation
* Clone the repository
* Update or install the appropriate versions of the above tools and libraries

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
    │   ├── images
    │       ├── 00000000.jpg
    │       ├── 00000001.jpg
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

The *output*, *post_fusion_points*, and *pre_fusion_points* directories are used to store the output from the fusion algorithm.
**NOTE**: These directories need to be initially added to the scene directory manually. The code does not yet check for existance of these directories and does not create them automatically.

### Pair.txt
The ```pair.txt``` file is used to list the 10 best supporting views for each reference view in the scene. The current implementation uses the first 5 views. This file will need to be created if the user does not wish to modify the ```util.cpp``` file. If modified, the user may choose their own method of providing this information to the fusion function. For more information on the ```pair.txt``` file, please refer to the [DTU - MVS Data Set](http://roboimagedata.compute.dtu.dk/?page_id=36).

### Running Fusion
* navigate to the ```src``` directory in the repository,
```
> cd fusion/src
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
> ./depth_fusion ~/Data/fusion/dtu/all_samples/scan9/ 5 0.1 0.8 4.0
```

where ***~/Data/fusion/dtu/all_samples/scan9/*** is the path to the scene we are fusing, ***5*** is the number of supporting views (including the reference view), ***0.1*** is the pre-fusion confidence threshold, ***0.8*** is the post-fusion confidence threshold, and ***4.0*** is the support region size.


