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
    int iLowH = 71;
    int iHighH = 107;

    int iLowS = 0;
    int iHighS = 255;

    int iLowV = 45;
    int iHighV = 82;
    
    int blurSize = 30;
    //Create trackbars in window
    createTrackbar("LowH", "Thresholded Image", &iLowH, 179); //Hue (0 - 179)
    createTrackbar("HighH", "Thresholded Image", &iHighH, 179);

    createTrackbar("LowS", "Thresholded Image", &iLowS, 255); //Saturation (0 - 255)
    createTrackbar("HighS", "Thresholded Image", &iHighS, 255);

    createTrackbar("LowV", "Thresholded Image", &iLowV, 255);//Value (0 - 255)
    createTrackbar("HighV", "Thresholded Image", &iHighV, 255);

    createTrackbar("blurSize", "Thresholded Image", &blurSize, 200);
    cout << "When done thresholding, press escape to continue" << endl;
    while (true) {
        retImg = blankImg.clone();
        //hsv is image with hsv conversion already, threshold it now
        inRange(hsv, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), hsvThresh); //Threshold the image based on sliders
        
        //morphological opening (removes small objects from the foreground)
        erode(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        dilate(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

        //morphological closing (removes small holes from the foreground)
        dilate(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        erode(hsvThresh, hsvThresh, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        
        //use blur to smooth the picture
        GaussianBlur(hsvThresh, hsvThresh, Size(blurSize*2+1, blurSize*2+1), 0, 0);
        threshold(hsvThresh, hsvThresh, 50, 255, 0/*Binary*/);
        
        namedWindow("Thresholded Image", CV_WINDOW_AUTOSIZE);
        imshow("Thresholded Image", hsvThresh); //show the thresholded image
        //img.size();
        imgCpy.copyTo(retImg, ~hsvThresh);
        namedWindow("Overlaid", CV_WINDOW_AUTOSIZE);
        imshow("Overlaid", retImg); //show the original image
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
    return retImg;
}

//real time ball finding while setting the playing area
Mat setPlayingArea(Mat img/*, vector<Vec3f> circles*/) {
    Mat cpyImg = img.clone();
    Mat contourMapping = Mat::zeros(img.size(), CV_8UC1);
    cout << "Left click the pockets, Right click to remove a point, Middle click to finish" << endl;
    vector<vector<Point> > playingAreaContours; vector<Vec4i> hierarchy;
    while (middleMousePressed == false) {
        cpyImg = img.clone();
        contourMapping = Mat::zeros(img.size(), CV_8UC1);
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
        namedWindow("Set Area", CV_WINDOW_AUTOSIZE);
        imshow("Set Area", cpyImg); //show the original image
        waitKey(30); //needed delay for picture to actually show up
    }
    //draw play area for contours
    for (int j = 0; j < playingArea.size(); j++)
    {
        line(contourMapping, playingArea[j], playingArea[(j + 1) % playingArea.size()], Scalar(255), 1, 8);
    }
    if(playingArea.size() > 1){
        findContours(contourMapping, playingAreaContours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
    }
    Mat mask = Mat::zeros(img.size(), CV_8UC1);
    drawContours(mask, playingAreaContours, -1, 255, CV_FILLED);
    floodFill(mask, Point(mask.rows/2, mask.cols/2), 255);
    Mat finalImg(img.size(), img.type());
    img.copyTo(finalImg, mask);
    namedWindow("Masked Window", 1);
    imshow("Masked Window", finalImg);
    destroyAllWindows();
    return finalImg;
}

//uses hough circles to find circles in passed in image
vector<Vec3f> findAllCircles(Mat img, Mat hsvThresh) {
    //Show everything that looks like a ball
    cout << "Adjust sliders until just the balls are seen, then press escape to continue" << endl;
    vector<Vec3f> circles;
    namedWindow("All Circles", CV_WINDOW_AUTOSIZE); 
    imshow("All Circles", emptyImg);
    int minDist = 21;
    int param1 = 20;
    int param2 = 5;
    int minRadius = 7;
    int maxRadius = 15;
    int blurSize = 2;
    //Create trackbars in "Control Circles" window
    createTrackbar("minDist", "All Circles", &minDist, 300); //minDist (0 - 500)
    createTrackbar("Param 1", "All Circles", &param1, 100); //Param1 (0 - 500)
    createTrackbar("Param 2", "All Circles", &param2, 300); //Param2 (0 - 500)
    createTrackbar("minRadius", "All Circles", &minRadius, 200); //minRadius (0 - 500)
    createTrackbar("maxRadius", "All Circles", &maxRadius, 200); //maxRadius (0 - 500)
    createTrackbar("blurSize", "All Circles", &blurSize, 200); //maxRadius (0 - 500)
    while (true) {
        Mat imgCpy = img.clone();
        Mat hsvCopy = hsvThresh.clone();
        cvtColor(hsvCopy, hsvCopy, COLOR_BGR2GRAY);
        //use blur to smooth the picture
        GaussianBlur(hsvCopy, hsvCopy, Size(blurSize*2+1, blurSize*2+1), 0, 0);
        namedWindow("HSV copy", 1);
        imshow("HSV copy", hsvCopy);
        HoughCircles(hsvCopy, circles, HOUGH_GRADIENT, 1, minDist+1, param1+1, param2+1, minRadius+1, maxRadius+1);
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

//to determine if it's on the playing area, have to find the contours
//to do this we must draw it on its own area and overlay it on the picture
vector<vector<Point> > makePlayingArea(Mat img) {
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

//draws final images
void findBalls(Mat finalImg, Mat projectorImg, vector<Vec3f> circles, vector<vector<Point> > playingAreaContours) {
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
    //set up the playing area (make it so pockets don't get seen as balls)
    Mat playingArea = setPlayingArea(copyImg);
    //set the thresholds to find the balls
    hsvThresh = hsvSliders(playingArea);
    //Look at all "seen" balls
    vector<Vec3f> allCircles = findAllCircles(copyImg, hsvThresh);
    //Make the playing area contours
    vector<vector<Point> > playingAreaContours = makePlayingArea(img);
    //Find only circles within the playing area
    findBalls(finalImg, projectorImg, allCircles, playingAreaContours);
    //Display final result
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
