/*
* @file poolTableSource.cpp
* @brief Track Pool Balls on a Table and find Trajectories
* @author Jake Thomas
* Todo:
* * Implement a simple GUI (app?) for with options to play and recalibration
* * Take pictures with pool stick to work on trajectories and finding the stick
* * Implement with video input
* * Output to a projector with just the circles and trajectory lines (circles done)
* * Learn how to use with PI or Arduino
*/

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>

using namespace cv;
using namespace std;

vector<Point> playingArea;
vector<Point> removedPoints;
bool middleMousePressed = false;
Mat emptyImg = Mat::zeros(35,500, CV_8UC1);

void playingAreaMouse(int event, int x, int y, int flags, void* userdata)
{
    if (event == EVENT_LBUTTONDOWN){
        playingArea.push_back(Point(x, y));
        cout << "Pocket " << playingArea.size() << " selected." << endl;
    }
    else if (event == EVENT_RBUTTONDOWN){
        if (!playingArea.empty()) {
            cout << "Deleted pocket " << playingArea.size() << "." << endl;
            removedPoints.push_back(playingArea[playingArea.size()-1]);
            if (playingArea.size() > 1) {
                removedPoints.push_back(playingArea[playingArea.size() - 2]);
            }
            playingArea.pop_back();
        }
    }
    else if (event == EVENT_MBUTTONDOWN)
    {
        cout << "Finding Balls!" << endl;
        middleMousePressed = true;
    }
    /*else if (event == EVENT_MOUSEMOVE)
    {
        cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

    }*/
}

Mat hsvSliders(Mat img) {
    Mat hsvThresh, hsv;
    //turn it into a hsv image
    cvtColor(img, hsv, COLOR_BGR2HSV);
    namedWindow("Control", CV_WINDOW_AUTOSIZE); 
    imshow("Control", emptyImg);
    //create a window called "Control"
    /*values that seem good for cropped 1: 114-126, 100-255, 75-255
    int iLowH = 114;
    int iHighH = 126;

    int iLowS = 100;
    int iHighS = 255;

    int iLowV = 75;
    int iHighV = 255;
    */
    //*values that seem good for cropped 2: 114-126, 98-255, 28-255
    int iLowH = 111;
    int iHighH = 126;

    int iLowS = 85;
    int iHighS = 255;

    int iLowV = 28;
    int iHighV = 255;
    //*/
    //Create trackbars in "Control" window
    createTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
    createTrackbar("HighH", "Control", &iHighH, 179);

    createTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
    createTrackbar("HighS", "Control", &iHighS, 255);

    createTrackbar("LowV", "Control", &iLowV, 255);//Value (0 - 255)
    createTrackbar("HighV", "Control", &iHighV, 255);

    cout << "When done thresholding, press escape to continue" << endl;
    while (true) {
        //hsv is image with hsv conversion already, threshold it now
        inRange(hsv, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), hsvThresh); //Threshold the image based on sliders

        //morphological opening (removes small objects from the foreground)
        erode(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        dilate(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

        //morphological closing (removes small holes from the foreground)
        dilate(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        erode(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

        //Calculate the moments of the thresholded image
        Moments oMoments = moments(hsvThresh);

        double dM01 = oMoments.m01;
        double dM10 = oMoments.m10;
        double dArea = oMoments.m00;

        namedWindow("Thresholded Image", CV_WINDOW_AUTOSIZE);
        imshow("Thresholded Image", hsvThresh); //show the thresholded image

        namedWindow("Original", CV_WINDOW_AUTOSIZE);
        imshow("Original", img); //show the original image
        if (waitKey(30) >= 0) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
        {
            cout << "Now set the playing area!" << endl;
            //cout << "User has clicked points:" << endl;
            //for (int i = 0; i < playingArea.size(); i++) {
            //	cout << playingArea[i] << endl;
            //}
            break;
        }
    }
    destroyAllWindows();
    return hsvThresh;
}

void setPlayingArea(Mat img, vector<Vec3f> circles) {
    cout << "Left click the pockets, Right click to remove a point, Middle click to finish" << endl;
    vector<vector<Point> > playingAreaContours; vector<Vec4i> hierarchy;
    while (middleMousePressed == false) {
        Mat cpyImg = img.clone();
        Mat contourMapping = Mat::zeros(img.size(), CV_8UC1);
        //set the callback function for any mouse event until mouse input for points is made
        setMouseCallback("Set Area", playingAreaMouse, NULL);
        if (playingArea.size() > 0) {
            for (int i = 0; i < playingArea.size(); i++) {
                circle(cpyImg, playingArea[i], 3, Scalar(255, 255, 0), -1, 8, 0);
            }
        }
        // Draw playing area over picture
        if (playingArea.size() > 1) {
            for (int k = 0; k < playingArea.size(); k++) {
                line(cpyImg, playingArea[k], playingArea[(k+1)%playingArea.size()], Scalar(255, 255, 0), 1, 8);
            }
        }
        //draw play area for contours
        for (int j = 0; j < playingArea.size(); j++)
        {
            line(contourMapping, playingArea[j], playingArea[(j + 1) % playingArea.size()], Scalar(255), 3, 8);
        }
        // Get the contours
        if (playingArea.size() > 1) {
            findContours(contourMapping, playingAreaContours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
            for (size_t i = 0; i < circles.size(); i++) {
                Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
                //if(true){ //used to find which circles are cut off
                if (pointPolygonTest(playingAreaContours[0], Point2f(center), false) == 1) {
                    int radius = cvRound(circles[i][2]);
                    // draw the circle center
                    circle(cpyImg, center, 3, Scalar(0, 255, 0), -1, 8, 0);
                    // draw the circle outline
                    circle(cpyImg, center, radius, Scalar(0, 0, 255), 3, 8, 0);
                }
            }
        }
        namedWindow("Set Area", CV_WINDOW_AUTOSIZE);
        imshow("Set Area", cpyImg); //show the original image
        waitKey(30);
    }
    destroyAllWindows();
}

vector<Vec3f> findAllCircles(Mat img, Mat hsvThresh) {
    //Show everything that looks like a ball
    cout << "Adjust sliders until just the balls are seen, then press escape to continue" << endl;
    vector<Vec3f> circles;
    namedWindow("Control Circles", CV_WINDOW_AUTOSIZE); 
    imshow("Control Circles", emptyImg);
    int minDist = 14;
    int param1 = 20;
    int param2 = 9;
    int minRadius = 7;
    int maxRadius = 15;
    //Create trackbars in "Control Circles" window
    createTrackbar("minDist", "Control Circles", &minDist, 300); //minDist (0 - 500)
    createTrackbar("Param 1", "Control Circles", &param1, 100); //Param1 (0 - 500)
    createTrackbar("Param 2", "Control Circles", &param2, 300); //Param2 (0 - 500)
    createTrackbar("minRadius", "Control Circles", &minRadius, 200); //minRadius (0 - 500)
    createTrackbar("maxRadius", "Control Circles", &maxRadius, 200); //maxRadius (0 - 500)
    while (true) {
        //HoughCircles(hsvThresh, circles, HOUGH_GRADIENT, 1, 20, 20, 20, 7, 15);
        Mat imgCpy = img.clone();
        HoughCircles(hsvThresh, circles, HOUGH_GRADIENT, 1, minDist+1, param1+1, param2+1, minRadius+1, maxRadius+1);
        for (size_t i = 0; i < circles.size(); i++)
        {
            Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);
            // draw the circle center
            circle(imgCpy, center, 3, Scalar(0, 255, 0), -1, 8, 0);
            // draw the circle outline
            circle(imgCpy, center, radius, Scalar(0, 0, 255), 3, 8, 0);
        }
        namedWindow("All Circles", 1);
        imshow("All Circles", imgCpy);
        if (waitKey(30) >= 0) { //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
            cout << "Now set the playing area!" << endl;
            break;
        }
    }
    destroyAllWindows();
    return circles;
}


vector<vector<Point> > makePlayingArea(Mat img) {
    //to determine if it's on the playing area, have to find the contours
    //to do this we must draw it on its own area and overlay it on the picture
    vector<vector<Point> > contours; vector<Vec4i> hierarchy;
    Mat contourMapping = Mat::zeros(img.size(), CV_8UC1);
    for (int j = 0; j < playingArea.size(); j++)
    {
        line(contourMapping, playingArea[j], playingArea[(j + 1) % playingArea.size()], Scalar(255), 3, 8);
    }
    // Get the contours
    findContours(contourMapping, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    return contours;
}

void findBalls(Mat finalImg, Mat projectorImg, vector<Vec3f> circles, vector<vector<Point> > playingAreaContours) {
    //finally use hough circles function to find the balls that have a center on the playing area
    int ballSize = 15;
    for (size_t i = 0; i < circles.size(); i++){
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        //if(true){ //used to find which circles are cut off
        if (pointPolygonTest(playingAreaContours[0], Point2f(center), false) == 1) {
            int radius = cvRound(circles[i][2]);
            // draw the circle center
            circle(finalImg, center, 3, Scalar(0, 255, 0), -1, 8, 0);
            circle(projectorImg, center, 3, Scalar(255, 255, 255), -1, 8, 0);
            // draw the circle outline
            circle(finalImg, center, ballSize, Scalar(0, 0, 255), 3, 8, 0);
            circle(projectorImg, center, ballSize, Scalar(255, 255, 255), 3, 8, 0);
        }
    }
}

int main(int argc, char** argv)
{
    Mat img, copyImg, hsvThresh, finalImg, projectorImg;
    if (argc != 2) {
        cout << "Did not read image" << endl;
        return -1;
    }
    //read the image and create copies
    img = imread(argv[1], IMREAD_COLOR);
    img.copyTo(copyImg);
    img.copyTo(finalImg);
    projectorImg = Mat::zeros(img.size(), CV_8UC1);
    //set the thresholds to find the balls
    hsvThresh = hsvSliders(img);
    //use blur to smooth the picture
    GaussianBlur(hsvThresh, hsvThresh, Size(9, 9), 2, 2);
    //Look at all "seen" balls
    vector<Vec3f> allCircles = findAllCircles(copyImg, hsvThresh);
    //set up the playing area (make it so pockets don't get seen as balls)
    setPlayingArea(copyImg, allCircles);
    //Make the playing area contours
    vector<vector<Point> > playingAreaContours = makePlayingArea(img);
    //Find only circles within the playing area
    findBalls(finalImg, projectorImg, allCircles, playingAreaContours);
    //Display final result
    // Draw playing area over picture
    for (int j = 0; j < playingArea.size(); j++)
    {
        line(finalImg, playingArea[j], playingArea[(j + 1) % playingArea.size()], Scalar(0, 0, 255), 1, 8);
    }
    //What the projector would output
    namedWindow("Projected Image", 1);
    imshow("Projected Image", projectorImg);
    //The picture with the balls found
    namedWindow("Balls Found", 1);
    imshow("Balls Found", finalImg);
    cout << "Balls found! Press escape to end" << endl;
    waitKey(0);
    return 0;
}
