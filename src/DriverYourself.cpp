/*
 * Copyright (C) 2020  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Include the single-file, header-only middleware libcluon to create high-performance microservices
#include "cluon-complete.hpp"
// Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications 
#include "opendlv-standard-message-set.hpp"
// Include the GUI and image processing header files from OpenCV
#include <cmath>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
//Include header from std library
#include <iostream>
//Include modules
#include "../modules/ObjectDetector/include/ObjectDetector.hpp"
#include "../modules/SteeringWheelCalculator/include/SteeringWheelCalculator.hpp"

// Define section
#define YMINH 19
#define YMAXH 30  
#define YMINS 0     // 50
#define YMAXS 255
#define YMINV 99
#define YMAXV 255

#define BMINH 74    // 50    // 102  // OR 50
#define BMAXH 133   // 145   // 135  // 145
#define BMINS 91    // 95    // 64   // 95
#define BMAXS 255   // 200   // 255  // 200
#define BMINV 40    // 42    // 51   // 42
#define BMAXV 216   // 215   // 255  // 215


/**
 * Calculates FPS based on the number of iterations/frames the program can process per second
 *
 * @param tm               OpenCV TickMeter object to track time
 * @param number_of_frames increments each time method is called (every iteration of main loop)
 * @param fps              pointer to FPS so the FPS can be updated
 */
void getFPS(cv::TickMeter tm, int *number_of_frames, int32_t *fps) {
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

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    // Parse the command line parameters as we require the user to specify some mandatory information on startup.
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        // Extract the values from the command line parameters
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory{new cluon::SharedMemory{NAME}};
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session where network messages are exchanged.
            // The instance od4 allows you to send and receive messages.
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            opendlv::proxy::GroundSteeringRequest gsr;
            std::mutex gsrMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope &&env){
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                //std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);

            // FPS variables
            int32_t fps = 0;
            cv::TickMeter tm;
            int number_of_frames_fps = 0, total_frame_number = 0, number_of_frame_passes_accurate = 0, number_of_frame_passes = 0;

            int roiWidth; // Window size for steering algorithm
            int detectedDirection = -1;   //Not detected: -1
                                          //Clockwise: 0
                                          //Anti-clockwise: 1
            
            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning()) {
                // OpenCV data structure to hold an image & Creating a Mat object for the HSV image
                cv::Mat img, imgHSV, croppedImg, croppedImgOriginalColor;
                cv::Rect roi;
                time_t sample_time_stamp;
                float sample_gsa, gsaAlgoResult;
                ObjectDetector od;
                SteeringWheelCalculator dp;

                // Start time meter for fps counter
                tm.start();

                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy the pixels from the shared memory into our own data structure.
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                    std::lock_guard<std::mutex> lck(gsrMutex); //Lock gsr mutex when record time stamp and received ground steering angle
                    sample_time_stamp = cluon::time::toMicroseconds(sharedMemory->getTimeStamp().second);
                    sample_gsa = gsr.groundSteering();
                }
                // Checking the sampleTimePoint when the current frame was captured.
                sharedMemory->unlock();

                // Converting the RGB image to an HSV image
                cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);

                total_frame_number++;//Count frame number
                // Cropping the image based on if a direction has been detected or not
                if(detectedDirection == -1) {
                    roiWidth = 640;
                    roi = cv::Rect(0, 260, roiWidth, 220);//Wider cropped image
                }
                else {
                    roiWidth = 207;
                    roi = cv::Rect(214, 316, roiWidth, 50);//Smaller cropped image
                }
                croppedImg = imgHSV(roi);
                croppedImgOriginalColor= img(roi);

                // Code adapted (line 146-166) from thresh_callback function found at https://docs.opencv.org/3.4/da/d0c/tutorial_bounding_rects_circles.html
                std::vector<std::vector<cv::Point>> contours_yellow = od.contourFilter(croppedImg, cv::Scalar(YMINH, YMINS, YMINV), cv::Scalar(YMAXH, YMAXS, YMAXV));
                std::vector<std::vector<cv::Point>> contours_blue = od.contourFilter(croppedImg, cv::Scalar(BMINH, BMINS, BMINV), cv::Scalar(BMAXH, BMAXS, BMAXV));

                //Hold bounding boxes data
                std::vector<cv::Rect> boundRect_blue(contours_blue.size()),boundRect_yellow(contours_yellow.size());
                od.findBoundingBox(contours_yellow, boundRect_yellow);
                od.findBoundingBox(contours_blue, boundRect_blue);

                // Drawing rectangles over the cones in relevant colors
                od.contourDraw(croppedImgOriginalColor, boundRect_yellow, contours_yellow, cv::Scalar(0, 255, 255));// Yellow
                od.contourDraw(croppedImgOriginalColor, boundRect_blue, contours_blue, cv::Scalar(255, 0, 0));//Blue

                //Generate center coordinates for detected objects
                std::vector<cv::Point> objectCoordinates_yellow = od.objectCenterCoordinates(boundRect_yellow);
                std::vector<cv::Point> objectCoordinates_blue = od.objectCenterCoordinates(boundRect_blue);

                //Check if the direction is detected
                if((detectedDirection==-1)&&(!objectCoordinates_yellow.empty()&&!objectCoordinates_blue.empty())){
                    detectedDirection = (objectCoordinates_yellow.begin()->x)<320 || (boundRect_blue.begin()->x)>320;
                    std::cout << detectedDirection << std::endl;
                }

                //Create string stream for manipulate message blocks
                std::stringstream ss, gsrss, yellowCoordinatesString, blueCoordinatesString, approachTestResult_accurate, approachTestResult_deviation;

                ss << "ts: " << sample_time_stamp << "; Group 8;";
                cv::putText(img, ss.str(), cv::Point(0,25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                //Detected objects center coordinates
                yellowCoordinatesString << "Yellow objects: ";
                for(const cv::Point& pt: objectCoordinates_yellow) { yellowCoordinatesString << "(" << pt.x << "," << pt.y << ") "; }
                cv::putText(img, yellowCoordinatesString.str(), cv::Point(0,55), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                blueCoordinatesString << "Blue objects: ";
                for(const cv::Point& pt: objectCoordinates_blue) { blueCoordinatesString << "(" << pt.x << "," << pt.y << ") "; }
                cv::putText(img, blueCoordinatesString.str(), cv::Point(0,70), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                // Get FPS
                getFPS(tm, &number_of_frames_fps, &fps);
                // Display FPS on windows
                cv::putText(img, std::to_string(fps), cv::Point(10, 100), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 255, 0));

                // Prints diagnostic steering algo data which can be extracted into a CSV file
                if((objectCoordinates_blue.empty()&&objectCoordinates_yellow.empty()) || detectedDirection ==-1){
                    gsaAlgoResult = 0;
                    std::cout << "group_08;" << sample_time_stamp << ";-0" << std::endl;
                }
                else if(!objectCoordinates_blue.empty()){
                    gsaAlgoResult = dp.steeringWheelAngle(detectedDirection, 1, objectCoordinates_blue.at(0), roiWidth);
                    std::cout << "Blue: group_08;" << sample_time_stamp << ";" << gsaAlgoResult << std::endl;
                }
                else if(!objectCoordinates_yellow.empty()){
                    gsaAlgoResult = dp.steeringWheelAngle(detectedDirection, 0, objectCoordinates_yellow.at(0), roiWidth);
                    std::cout << "Yellow: group_08;" << sample_time_stamp << ";" << gsaAlgoResult << std::endl;
                }

                //Counting frame for test approach results
                if(std::fabs(gsaAlgoResult-sample_gsa) < 1e-15){
                    number_of_frame_passes_accurate++; //Counting the frame where the algo is exactly the same compare to sample gsa
                }
                if(std::fabs(gsaAlgoResult-sample_gsa) >= 0 && std::fabs(gsaAlgoResult-sample_gsa)<=std::fabs(sample_gsa/2) ){
                    number_of_frame_passes++; //Counting the frames with +/-50% deviation compare to sample gsa
                }

                //Ground steering request string
                gsrss << "GroundSteeringRequest: Sample:" << sample_gsa << "; Algorithm: " << gsaAlgoResult;
                cv::putText(img, gsrss.str(), cv::Point(0,40), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                //Statics data about the algorithm calculation
                /* std::cout << "Full accurate frames: " << number_of_frame_passes_accurate << std::endl;
                std::cout << "Within 50% deviation frames: " << number_of_frame_passes << std::endl;
                std::cout << "Total received frames: " << total_frame_number << std::endl; */

                approachTestResult_accurate << "Full accurate %: " << ((double)number_of_frame_passes_accurate/(double)total_frame_number)*100 << "%";
                approachTestResult_deviation << "50% deviation %: " << ((double)number_of_frame_passes/(double)total_frame_number)*100 << "%";

                cv::putText(img, approachTestResult_accurate.str(), cv::Point(0,120), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);
                cv::putText(img, approachTestResult_deviation.str(), cv::Point(0,135), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                // Display image windows on the screen
                if (VERBOSE) {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::imshow("Region of Interest", croppedImgOriginalColor);
                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }
    return retCode;
}