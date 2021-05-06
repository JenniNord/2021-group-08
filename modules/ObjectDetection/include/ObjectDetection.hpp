#ifndef OBJECTDETECTION
#define OBJECTDETECTION

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/types.hpp>

class ObjectDetection {
    public:
        void contourDraw(cv::Mat image, std::vector<cv::Rect> shapeBoundary, std::vector<std::vector<cv::Point>> contours_color, cv::Scalar color);
        std::vector<std::vector<cv::Point>> contourFilter(cv::Mat imgHSV, cv::Scalar min, cv::Scalar max);
        std::vector<cv::Rect> findBoundingBox(std::vector<std::vector<cv::Point>> contours, std::vector<cv::Rect> boundRect);
        void filtering(cv::Mat imgThresh);
        std::vector<cv::Point> objectCenterCoordinates(const std::vector<cv::Rect>& objectRects);
};

#endif
