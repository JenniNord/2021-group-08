#ifndef DATAPROCESSOR
#define DATAPROCESSOR

#include <opencv2/core/utility.hpp>

class SteeringWheelCalculator{
    public:
        float steeringWheelAngle(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize);
};

#endif //DATAPROCESSOR
