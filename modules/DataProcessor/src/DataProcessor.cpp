#include "../modules/DataProcessor//include/DataProcessor.hpp"

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
    int maxSteeringArea;
    if((direction&&coneColor) || (!direction&&!coneColor)){
        maxSteeringArea = windowSize/2;
    }
    else if ((direction&&!coneColor) || (!direction&&coneColor)) {
        maxSteeringArea = windowSize/2;
    }

    if(coneCoordinate.x == maxSteeringArea){
        return 0.290888;
    }
    else if(coneCoordinate.x < maxSteeringArea){
        return (0.290888/maxSteeringArea)*coneCoordinate.x;
    }
    else if(coneCoordinate.x > maxSteeringArea)
    {
        return (0.290888/maxSteeringArea)*coneCoordinate.x;
    }
}

float DataProcessor::steeringWheelDirection(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize) {
    float swa = steeringWheelAngle(direction, coneColor, coneCoordinate, windowSize);
    if(coneColor == 1){
        if(!direction){
            return swa*-1;
        }
        else{
            return (float)0.290888 - swa;
        }
    }
    else if(coneColor == 0) {
        if (direction) {
            return swa;
        } else {
            return (float)-0.290888 - swa;
        }
    }
}