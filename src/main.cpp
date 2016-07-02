#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/tracking.hpp"

const std::string windowName = "ObjectTracker v0.1";

//Is the user selecting a region
bool isSelectingRegion = false;

//Should we be tracking an object?
//TODO
int trackObject = 0;

//Region representation
cv::Rect regionOfInterest;

//Location of where the mouse was clicked
cv::Point mouseClickOrigin;

//Container to hold the image
cv::Mat frame;


//Mouse callback function
static void onMouse(int event, int x, int y, int flags, void* userdata);

int main(int argc, char** argv)
{
     //Create an image to show mouse tracking on
     cv::Mat canvas(600, 800, CV_8UC3);
     
     //And a window we can show off our KalmanFilter in
     cv::namedWindow(windowName, cv::WINDOW_NORMAL);
     cv::imshow(windowName, canvas);

     //Register mouse callbacks so we can get the position
     MousePosition mousePos;
     cv::setMouseCallback(windowName, callbackFunction, (void*)&mousePos);

     //Create a KalmanFilter class
     cv::KalmanFilter filter(4, 2, 0);

     //Initialize KalmanFilter
     filter.transitionMatrix  = *(cv::Mat_<float>(4,4) << 1,0,1,0,
                                                          0,1,0,1,
                                                          0,0,1,0,
                                                          0,0,0,1);
    
     //And measurement matrix
     cv::Mat_<float> measurement(2,1); measurement.setTo(cv::Scalar(0));

     filter.statePre.at<float>(0) = mousePos.x;
     filter.statePre.at<float>(1) = mousePos.y;
     filter.statePre.at<float>(2) = 0;
     filter.statePre.at<float>(3) = 0;

     cv::setIdentity(filter.measurementMatrix);
     cv::setIdentity(filter.processNoiseCov, cv::Scalar::all(1e-4));
     cv::setIdentity(filter.measurementNoiseCov, cv::Scalar::all(1e-1));
     cv::setIdentity(filter.errorCovPost, cv::Scalar::all(0.1));

     //Set velocities
     cv::vector<cv::Point> mousePoints;
     cv::vector<cv::Point> kalmanPoints;

     bool isStillRunning = true;
     while(isStillRunning)
     {
         //Predict
         cv::Mat prediction = filter.predict();
         cv::Point predictionPoint(prediction.at<float>(0), prediction.at<float>(1));

         //Get the mouse points
         measurement(0) = mousePos.x;
         measurement(1) = mousePos.y;

         //Updated phase
         cv::Mat estimated = filter.correct(measurement);

         cv::Point statePoint(estimated.at<float>(0), estimated.at<float>(1));
         cv::Point measuredPoint(measurement(0), measurement(1));

         //Plot the points
         cv::imshow(windowName, canvas);
         canvas = cv::Scalar::all(0);

         mousePoints.push_back(measuredPoint);
         kalmanPoints.push_back(statePoint);

         for(size_t i = 0; i < mousePoints.size() - 1; ++i)
         {
             cv::line(canvas, mousePoints[i], mousePoints[i+1], cv::Scalar(255,255,0), 1);
         }
         for(size_t i = 0; i < kalmanPoints.size() - 1; ++i)
         {
             cv::line(canvas, kalmanPoints[i], kalmanPoints[i+1], cv::Scalar(0,155,255), 1);
         }

        //Run at 100 hz
        auto key = cv::waitKey(10);
        switch(key)
        {
            case(27):
            {
                isStillRunning = false;
                break;
            }
            case(32):
            {
                mousePoints.clear();
                kalmanPoints.clear();
            }
        }
     }

     return 0;
}

static void onMouse(int event, int x, int y, int flags, void* userdata)
{
    if(isSelectingRegion)
    {
        regionOfInterest.x = std::MIN(x, mouseClickOrigin.x);
        regionOfInterest.y = std::MIN(y, mouseClickOrigin.y);
        regionOfInterest.width = std::abs(x - mouseClickOrigin.x);
        regionOfInterest.height = std::abs(y - mouseClickOrigin.y);

        regionOfInterest &= cv::Rect(0, 0, frame.cols, frame.rows);

    }
    switch(event)
    {
        case(cv::EVENT_LBUTTONDOWN):
        {
            mouseClickOrigin = cv::Point(x, y);
            regionOfInterest = cv::Rect(x, y, 0, 0);
            isSelectingRegion = true;
            break;
        }
        case(cv::EVENT_LBUTTONUP):
        {
            isSelectingRegion = false;
            if(regionOfInterest.width > 0 && regionOfInterest.height > 0)
            {
                shouldTrackObject = -1;
            }
            break;
        }
    }
}