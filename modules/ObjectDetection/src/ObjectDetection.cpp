#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/types.hpp>
#include "../modules/ObjectDetection/include/ObjectDetection.hpp"

#define THRESH 100 // Sets a threshold for the Canny algo

// Method draws rectangles over the contours found
void ObjectDetection::contourDraw(cv::Mat image, std::vector<cv::Rect> shapeBoundary, std::vector<std::vector<cv::Point>> contours_color, cv::Scalar color){
    //Drawing rectangles over the contours of the detected shapes in yellow/blue
    for(unsigned long i = 1; i <= contours_color.size(); i++) {
        cv::rectangle(image, shapeBoundary[i].tl(), shapeBoundary[i].br(), color, 1);
    }
}

// Method returns the contours of the masked shapes filtered by the desired color
std::vector<std::vector<cv::Point>> ObjectDetection::contourFilter(cv::Mat imgHSV, cv::Scalar min, cv::Scalar max) {
    // Creating a Mat object for the color space and output Mat for the contour finder
    cv::Mat imgColorSpace, canny_output;
    // Checking that the HSV image is within the range, filtering out the desired colors, and displaying it
    cv::inRange(imgHSV, min, max, imgColorSpace);
    filtering(imgColorSpace);
    // Input the color mask, output object, threshold number and thresh*2 (why?)
    cv::Canny(imgColorSpace, canny_output, THRESH, THRESH*2);
    // Output for the contours
    std::vector<std::vector<cv::Point>> contours;
    // Find the contours using the Canny output
    cv::findContours(canny_output, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    return contours;
}

// Method finds the bounding boxes of the contour of color filtered objects
std::vector<cv::Rect>ObjectDetection::findBoundingBox(std::vector<std::vector<cv::Point>> contours, std::vector<cv::Rect> boundRect) {
    std::vector<std::vector<cv::Point>> contours_poly(contours.size());

    for(size_t i = 0; i < contours.size(); i++) {
        // Approximates a curve/polygon with another curve/polygon
        cv::approxPolyDP(contours[i], contours_poly[i], 3, true);
        // Rectangle shape to be drawn on image where cone appears
        boundRect[i] = cv::boundingRect(contours_poly[i]);
    }
    return boundRect;
}

// Method filters noise around the cones
// Referenced from: https://www.opencv-srf.com/2010/09/object-detection-using-color-separation.html
void ObjectDetection::filtering(cv::Mat imgThresh) {
    // Removing small objects in foreground with an elliptic shape
    cv::erode(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    cv::dilate(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    // Filling small holes in the foreground with an elliptic shape
    cv::dilate(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
    cv::erode(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));
}

std::vector<cv::Point> ObjectDetection::objectCenterCoordinates(const std::vector<cv::Rect>& objectRects){
    std::vector<cv::Point> objectCoordinates;
    for(const cv::Rect& rc : objectRects){ objectCoordinates.emplace_back(cv::Point(rc.tl().x + rc.width / 2, rc.tl().y + rc.height / 2)); }
    return objectCoordinates;
}