#ifndef DATAPROCESSOR
#define DATAPROCESSOR

#include <opencv2/core/utility.hpp>
#include "../../cone/include/cone.hpp"

class DataProcessor{
    public:
        float steeringWheelAngle(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize);
        float steeringWheelDirection(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize);
        void getFPS(cv::TickMeter tm, int* number_of_frames, int32_t* fps);
};

#endif //DATAPROCESSOR
