#ifndef YOLOV5_H
#define YOLOV5_H

#include "layer.h"
#include "net.h"

#if defined(USE_NCNN_SIMPLEOCV)
#include "simpleocv.h"
#else
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <float.h>
#include <stdio.h>
#include <vector>
#include <string>
#define YOLOV5_V60 1 //YOLOv5 v6.0
#define MAX_STRIDE 64

// original pretrained model from https://github.com/ultralytics/yolov5
// the ncnn model https://github.com/nihui/ncnn-assets/tree/master/models
class Yolov5
{
public:
    struct Object {
        cv::Rect_<float> rect;
        int label;
        float prob;
    };
public:
    std::vector<std::string> labels;
    ncnn::Mutex lock;
    int target_size;
    float prob_threshold;
    float nms_threshold;
private:
    ncnn::Net yolov5;
    ncnn::UnlockedPoolAllocator blob_pool_allocator;
    ncnn::PoolAllocator workspace_pool_allocator;
public:
    static Yolov5& instance()
    {
        static Yolov5 yolov5;
        return yolov5;
    }
    bool load(const std::string &modelType);
    int detect(const cv::Mat& bgr, std::vector<Object>& objects);
    void draw(cv::Mat &bgr, const std::vector<Object>& objects);
private:
    static inline float sigmoid(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }
    static inline float intersection_area(const Object& a, const Object& b)
    {
        cv::Rect_<float> inter = a.rect & b.rect;
        return inter.area();
    }
    static void qsort_descent_inplace(std::vector<Object>& objects, int left, int right);
    static void qsort_descent_inplace(std::vector<Object>& objects);
    static void generate_proposals(const ncnn::Mat& anchors,
                                   int stride,
                                   const ncnn::Mat& in_pad,
                                   const ncnn::Mat& feat_blob,
                                   float prob_threshold,
                                   std::vector<Object>& objects);
    static void nms_sorted_bboxes(const std::vector<Object>& objects,
                                  std::vector<int>& picked,
                                  float nms_threshold);
private:
    Yolov5();
};

#endif // YOLOV5_H
