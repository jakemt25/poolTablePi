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

//used to click to set the playing area
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

//used to manually threshold the playing area with hsv, also blurs image
Mat hsvSliders(Mat img) {
    Mat hsvThresh, hsv, imgCpy;
    Mat blankImg;
    Mat retImg;
    imgCpy = img.clone();
    //turn it into a hsv image
    cvtColor(img, hsv, COLOR_BGR2HSV);
    namedWindow("Thresholded Image", CV_WINDOW_AUTOSIZE); 
    imshow("Thresholded Image", emptyImg);
    //init values
    int iLowH = 116;
    int iHighH = 125;
    int iLowS = 89;
    int iHighS = 189;
    int iLowV = 0;
    int iHighV = 255;
    int blurSize = 3;
    //Create trackbars in window
    createTrackbar("LowH", "Thresholded Image", &iLowH, 179); 
    createTrackbar("HighH", "Thresholded Image", &iHighH, 179);
    createTrackbar("LowS", "Thresholded Image", &iLowS, 255); 
    createTrackbar("HighS", "Thresholded Image", &iHighS, 255);
    createTrackbar("LowV", "Thresholded Image", &iLowV, 255);
    createTrackbar("HighV", "Thresholded Image", &iHighV, 255);
    createTrackbar("blurSize", "Thresholded Image", &blurSize, 200);
    cout << "When done thresholding, press escape to continue" << endl;
    while (true) {
        retImg = blankImg.clone();
        //use slider values to threshold
        inRange(hsv, Scalar(iLowH, iLowS, iLowV), 
                Scalar(iHighH, iHighS, iHighV), hsvThresh); 
        
        //use blur to smooth the picture, needs to be odd number
        GaussianBlur(hsvThresh, hsvThresh, Size(blurSize*2+1, blurSize*2+1), 0, 0);
        threshold(hsvThresh, hsvThresh, 50, 255, 0/*Binary*/);
        
        namedWindow("Thresholded Image", CV_WINDOW_AUTOSIZE);
        imshow("Thresholded Image", hsvThresh);
        //want the opposite of the table to show up
        imgCpy.copyTo(retImg, ~hsvThresh);
        namedWindow("Overlaid", CV_WINDOW_AUTOSIZE);
        imshow("Overlaid", retImg);
        //wait for key press to break loop
        if (waitKey(30) >= 0) {
            cout << "Now set the playing area!" << endl;
            break;
        }
    }
    destroyAllWindows();
    return retImg;
}

//set the playing area
Mat setPlayingArea(Mat img/*, vector<Vec3f> circles*/) {
    Mat cpyImg = img.clone();
    Mat contourMapping = Mat::zeros(img.size(), CV_8UC1);
    cout << "Left click the pockets, Right click to remove a point, Middle click to finish" << endl;
    vector<vector<Point> > playingAreaContours; vector<Vec4i> hierarchy;
    while (middleMousePressed == false) {
        //start with fresh image every time to be able to rewrite changes
        cpyImg = img.clone();
        contourMapping = Mat::zeros(img.size(), CV_8UC1);
        //set the callback function for any mouse event until mouse input for points is made
        setMouseCallback("Set Area", playingAreaMouse, NULL);
        //creates points where clicked
        if (playingArea.size() > 0) {
            for (int i = 0; i < playingArea.size(); i++) {
                circle(cpyImg, playingArea[i], 3, Scalar(255, 255, 0), -1, 8, 0);
            }
        }
        //connects points to form contour
        if (playingArea.size() > 1) {
            for (int k = 0; k < playingArea.size(); k++) {
                line(cpyImg, playingArea[k], playingArea[(k+1)%playingArea.size()], Scalar(255, 255, 0), 1, 8);
            }
        }
        namedWindow("Set Area", CV_WINDOW_AUTOSIZE);
        imshow("Set Area", cpyImg); //show the image with playing area over it
        waitKey(30); //needed delay for picture to actually show up
    }
    //draw play area for contours on own canvas
    for (int j = 0; j < playingArea.size(); j++)
    {
        line(contourMapping, playingArea[j], 
                playingArea[(j + 1) % playingArea.size()], Scalar(255), 1, 8);
    }
    //find contours of playing area
    if(playingArea.size() > 1){
        findContours(contourMapping, playingAreaContours, 
                hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    }
    //create mask to only see the playing area
    Mat mask = Mat::zeros(img.size(), CV_8UC1);
    drawContours(mask, playingAreaContours, -1, 255, CV_FILLED);
    //table should be the center of the picture so this should be fine
    floodFill(mask, Point(mask.rows/2, mask.cols/2), 255);
    Mat finalImg(img.size(), img.type());
    img.copyTo(finalImg, mask);
    destroyAllWindows();
    return finalImg;
}

//uses hough circles to find circles in passed in image
vector<Vec3f> findAllCircles(Mat img, Mat hsvThresh) {
    cout << "Adjust sliders until just the balls are seen, then press escape to continue" << endl;
    vector<Vec3f> circles;
    namedWindow("All Circles", CV_WINDOW_AUTOSIZE); 
    imshow("All Circles", emptyImg);
    int minDist = 21;
    int param1 = 20;
    int param2 = 7;
    int minRadius = 7;
    int maxRadius = 15;
    int blurSize = 3;
    //Create trackbars in "Control Circles" window
    createTrackbar("minDist", "All Circles", &minDist, 300); //minDist (0 - 500)
    createTrackbar("Param 1", "All Circles", &param1, 300); //Param1 (0 - 500)
    createTrackbar("Param 2", "All Circles", &param2, 30); //Param2 (0 - 500)
    createTrackbar("minRadius", "All Circles", &minRadius, 200); //minRadius (0 - 500)
    createTrackbar("maxRadius", "All Circles", &maxRadius, 200); //maxRadius (0 - 500)
    createTrackbar("blurSize", "All Circles", &blurSize, 200); //maxRadius (0 - 500)
    while (true) {
        //fresh copies for redrawing
        Mat imgCpy = img.clone();
        Mat hsvCopy = hsvThresh.clone();
        //need to be single channel for hough circle function
        cvtColor(hsvCopy, hsvCopy, COLOR_BGR2GRAY);
        //use blur to smooth the picture
        GaussianBlur(hsvCopy, hsvCopy, Size(blurSize*2+1, blurSize*2+1), 0, 0);
        namedWindow("Blurred B&W", 1);
        imshow("Blurred B&W", hsvCopy);
        //gets location of circles on playing area. +1's so they are never 0
        HoughCircles(hsvCopy, circles, HOUGH_GRADIENT,
                1, minDist+1, param1+1, param2+1, minRadius+1, maxRadius+1);
        //Draw circles on the image where "balls" are found
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
        if (waitKey(30) >= 0) { //wait for key press to break out
            cout << "Now set the playing area!" << endl;
            break;
        }
    }
    destroyAllWindows();
    return circles;
}

//draws final images
void finalOutput(Mat finalImg, Mat projectorImg, vector<Vec3f> circles) {
    int ballSize = 15;
    for (size_t i = 0; i < circles.size(); i++){
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        // draw the circle center
        circle(finalImg, center, 3, Scalar(0, 255, 0), -1, 8, 0);
        circle(projectorImg, center, 3, Scalar(255, 255, 255), -1, 8, 0);
        // draw the circle outline
        circle(finalImg, center, ballSize, Scalar(0, 0, 255), 3, 8, 0);
        circle(projectorImg, center, ballSize, Scalar(255, 255, 255), 3, 8, 0);
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
    //Choose the playing area to focus on
    Mat playingArea = setPlayingArea(copyImg);
    //Separate the balls from the table
    hsvThresh = hsvSliders(playingArea);
    //Look at all "seen" balls
    vector<Vec3f> allCircles = findAllCircles(copyImg, hsvThresh);
    finalOutput(finalImg, projectorImg, allCircles);
    //Display final result
    //What the projector would output
    namedWindow("Projected Image", 1);
    imshow("Projected Image", projectorImg);
    //The picture with the balls found
    namedWindow("Balls Found", 1);
    imshow("Balls Found", finalImg);
    cout << "Balls found! Press a key to end" << endl;
    waitKey(0);
    return 0;
}
