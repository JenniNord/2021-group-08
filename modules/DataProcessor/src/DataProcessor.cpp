#include "../modules/DataProcessor//include/DataProcessor.hpp"
#include <utility>

/**
 * Calculates FPS based on the number of iterations/frames the program can process per second
 * 
 * @param tm               OpenCV TickMeter object to track time
 * @param number_of_frames increments each time method is called (every iteration of main loop)
 * @param fps              pointer to FPS so the FPS can be updated
 */
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

/**
 * Calculates steering wheel angle based on cone color and direction using the ROI windowSize
 * 
 * @param  direction      clockwise = 0, counterclockwise = 1
 * @param  coneColor      yellow = 0, blue = 1
 * @param  coneCoordinate x position of detected cone on the ROI
 * @param  windowSize     pixel width of the ROI
 * @return                steering wheel angle
 */
float DataProcessor::steeringWheelAngle(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize) {
    int maxSteeringAreaLeft = windowSize / 2 ; // < center px
    int maxSteeringAreaRight = windowSize / 2; // > center px
    float steeringAngle = 0.290888f; // Default to be max Steering

    // counterclockwise & yellow OR clockwise & blue = LEFT SIDE
    if ((direction && !coneColor) || (!direction && coneColor)) {
        if (coneCoordinate.x >= maxSteeringAreaLeft) {
            steeringAngle = -1 * steeringAngle; // -1 since we'll need to steer to the right
        } else if (coneCoordinate.x < maxSteeringAreaLeft) {
            steeringAngle = -1 * (steeringAngle / (float) maxSteeringAreaLeft) * (float) coneCoordinate.x;
        }
    // counterclockise & blue OR clockwise & yellow = RIGHT SIDE
    } else if ((direction && coneColor) || (!direction && !coneColor)) {
        if (coneCoordinate.x <= (float) maxSteeringAreaRight) {
            return steeringAngle; // + since we'll need to steer to the left
        } else if (coneCoordinate.x > maxSteeringAreaRight) {
            // Calculates how much steering each of the pixels on the area on the right side is worth
            // & then multiplies it with the number of pixels the cone is at with an offset (maxSteeringAreaRight)
            // Finally subtracts the value from the maximum steering to get a result that decreases as the
            // cone goes closer to the right
            steeringAngle = steeringAngle - ((steeringAngle / (float) (windowSize - maxSteeringAreaRight)) * 
                    ((float) coneCoordinate.x - maxSteeringAreaRight));
        }
    } else {
        steeringAngle = -0;
    }

    return steeringAngle;
}