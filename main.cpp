/*******************************************************************************
 * Elektrotehnicki fakultet Sarajevo
 * Predmet: Multimedijalni sistemi
 * Nastavnik: Vanr. prof. dr Haris Supic
 * Asistent: Dr Dinko Osmankovic
 * Studenti: Ackar Haris, Kico Iris, Tahirbegovic Anel, Tokic Rahima
 * Tema: Razvoj panorama aplikacije koristeci OpenCV
********************************************************************************/

#include <QCoreApplication>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/stitching.hpp>


using namespace cv;
using namespace std;

String inttostr(int input) {
    stringstream ss;
    ss << input;
    return ss.str();
}

bool provjeraGranica(const Mat &mask, const Rect &interiorBB, int &top, int &bottom, int &left, int &right)
{

    bool returnVal = true;

    Mat sub = mask(interiorBB);

    unsigned int x = 0;
    unsigned int y = 0;

    unsigned int cTop = 0;
    unsigned int cBottom = 0;
    unsigned int cLeft = 0;
    unsigned int cRight = 0;


    for (y = 0, x = 0; x<sub.cols; ++x)
    {
        if (sub.at<unsigned char>(y, x) == 0)
        {
            returnVal = false;
            ++cTop;
        }
    }

    for (y = sub.rows - 1, x = 0; x<sub.cols; ++x)
    {
        if (sub.at<unsigned char>(y, x) == 0)
        {
            returnVal = false;
            ++cBottom;
        }
    }

    for (y = 0, x = 0; y<sub.rows; ++y)
    {
        if (sub.at<unsigned char>(y, x) == 0)
        {
            returnVal = false;
            ++cLeft;
        }
    }

    for (x = sub.cols - 1, y = 0; y<sub.rows; ++y)
    {
        if (sub.at<unsigned char>(y, x) == 0)
        {
            returnVal = false;
            ++cRight;
        }
    }


    if (cTop > cBottom)
    {
        if (cTop > cLeft)
            if (cTop > cRight)
                top = 1;
    }
    else
        if (cBottom > cLeft)
            if (cBottom > cRight)
                bottom = 1;

    if (cLeft >= cRight)
    {
        if (cLeft >= cBottom)
            if (cLeft >= cTop)
                left = 1;
    }
    else
        if (cRight >= cTop)
            if (cRight >= cBottom)
                right = 1;



    return returnVal;
}

bool sortX(Point a, Point b)
{
    return a.x < b.x;
}

bool sortY(Point a, Point b)
{
    return a.y < b.y;
}

void Cropuj(Mat& input)
{
    Mat gray;
    cvtColor(input, gray, CV_BGR2GRAY);

    Mat mask = gray>0;

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0));


    Mat contourImage = cv::Mat::zeros(input.size(), CV_8UC3);;

    unsigned int maxSize = 0;
    unsigned int id = 0;
    for (unsigned int i = 0; i<contours.size(); ++i)
    {
        if (contours.at(i).size() > maxSize)
        {
            maxSize = contours.at(i).size();
            id = i;
        }
    }

    Mat contourMask = cv::Mat::zeros(input.size(), CV_8UC1);
    drawContours(contourMask, contours, id, Scalar(255), -1, 8, hierarchy, 0, Point());

    vector<Point> cSortedX = contours.at(id);
    sort(cSortedX.begin(), cSortedX.end(), sortX);

    vector<Point> cSortedY = contours.at(id);
    sort(cSortedY.begin(), cSortedY.end(), sortY);


    unsigned int minXId = 0;
    unsigned int maxXId = cSortedX.size() - 1;

    unsigned int minYId = 0;
    unsigned int maxYId = cSortedY.size() - 1;

    Rect interiorBB;

    while ((minXId<maxXId) && (minYId<maxYId))
    {
        Point min(cSortedX[minXId].x, cSortedY[minYId].y);
        Point max(cSortedX[maxXId].x, cSortedY[maxYId].y);

        interiorBB = Rect(min.x, min.y, max.x - min.x, max.y - min.y);

        int ocTop = 0;
        int ocBottom = 0;
        int ocLeft = 0;
        int ocRight = 0;

        bool finished = provjeraGranica(contourMask, interiorBB, ocTop, ocBottom, ocLeft, ocRight);
        if (finished)
        {
            break;
        }

        if (ocLeft)++minXId;
        if (ocRight) --maxXId;

        if (ocTop) ++minYId;
        if (ocBottom)--maxYId;
    }
    input = input(interiorBB);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int numPic;
    int i = 0;
    vector<Mat> Img;
    Mat oImg;

    cout << "ETF Sarajevo - Multimedijalni sistemi" << endl;
    cout << "         PANORAMA APLIKACIJA          " << endl << endl;


        cin.clear();
        cout << "Unesite broj slika za panoramu: ";
        cin >> numPic;
        cin.clear();


        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(100);

        VideoCapture cap;
        if (!cap.open(0))
            return 0;
        while (1) {
            if (i == numPic) break;
            Mat frame;
            cap >> frame;
            if (frame.empty()) break;
            namedWindow("Prikaz sa kamere", CV_WINDOW_AUTOSIZE);
            imshow("Prikaz sa kamere", frame);
            char ch = waitKey(25);
            if (ch == 's') {
                Img.push_back(frame);
                imwrite("/home/haris/Qt OPENCV projects/untitled/image" + inttostr(i) + ".jpg", frame, compression_params);
                i++;
            }
        }
        cap.release();
        destroyWindow("Prikaz sa kamere");

        cout << "Stitching in progress..." << endl;
        unsigned long StartTime = 0, StopTime = 0;
        StartTime = getTickCount();

        Stitcher stitcher = Stitcher::createDefault();
        Stitcher::Status status = stitcher.stitch(Img, oImg);
        if (Stitcher::OK == status) {
            imwrite("/home/haris/Qt OPENCV projects/untitled/StitchImage.jpg", oImg, compression_params);
            StopTime = getTickCount();
            cout << "Vrijeme stitchanja je: " << float(StopTime - StartTime) / getTickFrequency() << " sec" << endl;
            imshow("/home/haris/Qt OPENCV projects/untitled/Stitching result (ORIGINAL)", oImg);
            Mat cImg = oImg;
            Cropuj(cImg);
            imshow("Stitching result (CROP)", cImg);
            imwrite("/home/haris/Qt OPENCV projects/untitled/StitchImage_CROP.jpg", cImg, compression_params);
            waitKey(0);
            destroyAllWindows();
        }
        else cout << "Stitchanje nije uspjelo!";
    return 0;
}
