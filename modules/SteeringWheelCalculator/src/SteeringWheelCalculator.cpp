#include "../include/SteeringWheelCalculator.hpp"

/**
 * Calculates steering wheel angle based on cone color and direction using the ROI windowSize
 * 
 * @param  direction      clockwise = 0, counterclockwise = 1
 * @param  coneColor      yellow = 0, blue = 1
 * @param  coneCoordinate x position of detected cone on the ROI
 * @param  windowSize     pixel width of the ROI
 * @return                steering wheel angle
 */
float SteeringWheelCalculator::steeringWheelAngle(bool direction, int coneColor, cv::Point coneCoordinate, int windowSize) {
    int maxSteeringAreaLeft = (windowSize / 2) - 5 ; // 5 px < center px
    int maxSteeringAreaRight = (windowSize / 2) + 5; // 5 px > center px
    float steeringAngle = 0.290888f; // Default to be max Steering

    // counterclockwise & yellow OR clockwise & blue = LEFT SIDE
    if ((direction && !coneColor) || (!direction && coneColor)) {
        if (coneCoordinate.x >= maxSteeringAreaLeft) {
            steeringAngle = -1 * steeringAngle; // -1 since we'll need to steer to the right
        } else if (coneCoordinate.x < maxSteeringAreaLeft) {
            steeringAngle = -1 * (steeringAngle / (float) maxSteeringAreaLeft) * (float) coneCoordinate.x;
        }
    // counterclockwise & blue OR clockwise & yellow = RIGHT SIDE
    } else if ((direction && coneColor) || (!direction && !coneColor)) {
        if ((float) coneCoordinate.x <= (float) maxSteeringAreaRight) {
            return steeringAngle; // + since we'll need to steer to the left
        } else if (coneCoordinate.x > maxSteeringAreaRight) {
            // Calculates how much steering each of the pixels on the area on the right side is worth
            // & then multiplies it with the number of pixels the cone is at with an offset (maxSteeringAreaRight)
            // Finally subtracts the value from the maximum steering to get a result that decreases as the
            // cone goes closer to the right
            steeringAngle = steeringAngle - ((steeringAngle / (float) (windowSize - maxSteeringAreaRight)) * 
                    ((float) coneCoordinate.x - (float) maxSteeringAreaRight));
        }
    } else {
        steeringAngle = -0;
    }

    return steeringAngle;
}