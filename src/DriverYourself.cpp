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
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
//Include header from std library
#include <iostream>
#include <sstream>

#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>

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

#define THRESH 100 // Sets a threshold for the Canny algo

// Function declarations
void contourDraw(cv::Mat image, std::vector<cv::Rect> shapeBoundary, std::vector<std::vector<cv::Point>> contours_color, cv::Scalar color);
std::vector<std::vector<cv::Point>> contourFilter(cv::Mat imgHSV, cv::Scalar min, cv::Scalar max);
std::vector<cv::Rect> findBoundingBox(std::vector<std::vector<cv::Point>> contours, std::vector<cv::Rect> boundRect);
void filtering(cv::Mat imgThresh);
void getFPS(cv::TickMeter tm, int* number_of_frames, int32_t* fps);

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
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);

            // FPS variables
            int32_t fps = 0;
            cv::TickMeter tm;
            int number_of_frames = 0;
            
            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning()) {
                // Start time meter for fps counter
                tm.start();

                // OpenCV data structure to hold an image.
                cv::Mat img;

                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy the pixels from the shared memory into our own data structure.
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                }
                // Checking the sampleTimePoint when the current frame was captured.
                time_t sample_time_stamp = cluon::time::toMicroseconds(sharedMemory->getTimeStamp().second);
                sharedMemory->unlock();

                // Processing the frame.
                time_t time_in_microsec = cluon::time::now().seconds();
                struct tm *p = gmtime(&time_in_microsec);
                std::stringstream ss, ss1;
                ss  << "Now: " << 1900+p->tm_year
                    << "-" << p->tm_mon/10 << p->tm_mon%10
                    << "-" << p->tm_mday/10 << p->tm_mday%10
                    << "T" << p->tm_hour/10 << p->tm_hour%10
                    << ":" << p->tm_min/10 << p->tm_min%10
                    << ":" << p->tm_sec/10 << p->tm_sec %10
                    << "Z; ts: " << sample_time_stamp << "; Group 8;";
                ss1 << "GroundSteeringRequest: " << gsr.groundSteering() << ";";
                cv::putText(img, ss.str(), cv::Point(0,25), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);
                cv::putText(img, ss1.str(), cv::Point(0,40), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.7, cv::Scalar(255,255,255),1);

                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);
                    std::cout << "main: groundSteering = " << gsr.groundSteering() << std::endl;
                }

                // Creating a Mat object for the HSV image
                cv::Mat imgHSV;

                // Converting the RGB image to an HSV image
                cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);

                cv::Rect roi(0, 260, 640, 220);
                cv::Mat croppedImg = imgHSV(roi);
                cv::Mat croppedImgOriginalColor = img(roi);

                // Code adapted (line 146-166) from thresh_callback function found at https://docs.opencv.org/3.4/da/d0c/tutorial_bounding_rects_circles.html 
                std::vector<std::vector<cv::Point>> contours_yellow = contourFilter(croppedImg, cv::Scalar(YMINH, YMINS, YMINV), cv::Scalar(YMAXH, YMAXS, YMAXV));
                std::vector<std::vector<cv::Point>> contours_blue = contourFilter(croppedImg, cv::Scalar(BMINH, BMINS, BMINV), cv::Scalar(BMAXH, BMAXS, BMAXV));

                std::vector<std::vector<cv::Point>> contours_yellow_filtered;

                // Creating arrays to hold data
                std::vector<cv::Rect> boundRect_blue(contours_blue.size()),boundRect_yellow(contours_yellow.size());

                boundRect_yellow = findBoundingBox(contours_yellow, boundRect_yellow);
                boundRect_blue = findBoundingBox(contours_blue, boundRect_blue);

                // Drawing rectangles over the cones in relevant colors
                contourDraw(croppedImgOriginalColor, boundRect_yellow, contours_yellow, cv::Scalar(0, 255, 255));// Yellow
                contourDraw(croppedImgOriginalColor, boundRect_blue, contours_blue, cv::Scalar(255, 0, 0));//Blue

                // Get FPS
                getFPS(tm, &number_of_frames, &fps);

                // Display FPS on main image
                cv::putText(img, std::to_string(fps), cv::Point(10,60), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 255, 0));

                // Show window with the outlined cones
                cv::imshow("Bounding Boxes", croppedImgOriginalColor);
                
                // Display the image from the shared memory on the screen
                cv::imshow(sharedMemory->name().c_str(), img);

                // Display image on your screen.
                if (VERBOSE) {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }
    return retCode;
}

// Method draws rectangles over the contours found
void contourDraw(cv::Mat image, std::vector<cv::Rect> shapeBoundary, std::vector<std::vector<cv::Point>> contours_color, cv::Scalar color) {
    //Drawing rectangles over the contours of the detected shapes in yellow/blue
    for(size_t i = 0; i< contours_color.size(); i++) {
        cv::rectangle(image, shapeBoundary[i].tl(), shapeBoundary[i].br(), color, 1);
    }
}

// Method returns the contours of the masked shapes filtered by the desired color
std::vector<std::vector<cv::Point>> contourFilter(cv::Mat imgHSV, cv::Scalar min, cv::Scalar max) {
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
std::vector<cv::Rect> findBoundingBox(std::vector<std::vector<cv::Point>> contours, std::vector<cv::Rect> boundRect) {
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
void filtering(cv::Mat imgThresh) {
    // Removing small objects in foreground with an elliptic shape
    cv::erode(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    cv::dilate(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(8, 8)));
    // Filling small holes in the foreground with an elliptic shape
    cv::dilate(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
    cv::erode(imgThresh, imgThresh, getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));
}

// Calculates FPS based on number of iterations/frames the program can process per second
void getFPS(cv::TickMeter tm, int* number_of_frames, int32_t* fps) {
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