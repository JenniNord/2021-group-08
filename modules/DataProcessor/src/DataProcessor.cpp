#include "../modules/DataProcessor//include/DataProcessor.hpp"
#include <utility>

// Calculates FPS based on number of iterations/frames the program can process per second
void DataProcessor::getFPS(cv::TickMeter tm, int *number_of_frames, int32_t *fps) {
    // Increment the number of frames/loop iterations
    *number_of_frames = *number_of_frames + 1;

    // Calculate FPS every 10 frames/iterations
    if (*number_of_frames == 10) {
        tm.stop();
        double current_time = tm.getTimeSec();
        *fps = (int) (*number_of_frames / current_time);
        tm.reset();
        *number_of_frames = 0;
    }
}

float DataProcessor::steeringWheelAngle(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize) {
    int maxSteeringArea = windowSize/2;;
    float SteeringAngle = 0.290888f;//Default to be max Steering
    //Define maxSteeringArea value
    if((direction&&coneColor) || (!direction&&!coneColor)){ //Cone color: 0 for Yellow, 1 for blue
        maxSteeringArea = windowSize/2;
    }
    else if ((direction&&!coneColor) || (!direction&&coneColor)) {
        maxSteeringArea = windowSize/2;
    }

    //Calculate SteeringAngle
    if(coneCoordinate.x < maxSteeringArea){
        SteeringAngle = (SteeringAngle/(float)maxSteeringArea)*(float)coneCoordinate.x;
    }
    else if(coneCoordinate.x > maxSteeringArea)
    {
        SteeringAngle = -(SteeringAngle/(float)maxSteeringArea)*(float)coneCoordinate.x;
    }
    return SteeringAngle;
}

float DataProcessor::steeringWheelDirection(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize) {
    float swa = steeringWheelAngle(direction, coneColor, std::move(coneCoordinate), windowSize);
    //For different cone colors
    switch (coneColor) {
        case 0: //Yellow
            if (direction) { return -1 * swa; }
            else { return 0.290888f - swa; }
        case 1: //Blue
            if (direction) { return 0.290888f - swa;}
            else { return swa * -1; }
        default:
            return 0;
    }
}