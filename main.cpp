//
//  main.cpp
//  DiameterCount
//
//  Created by 周益辰 on 14-3-28.
//  Copyright (c) 2014年 周益辰. All rights reserved.
//  g++ main.cpp -o main -static `pkg-config --cflags --libs opencv` -mmacosx-version-min=10.8

#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "cv.h"
#include "cmath"


/* Table:
 0 Background
 255 Interior
 100 Boundary
 250 Interior with measured diameter
 200 Unspecified region
 */

#define MAXD 20
#define PI 3.1416

using namespace std;
using namespace cv;

Mat img;
Mat raw;
int ansCo[1000000][2];
double ansD[100000];
int nans;
double fsin[500][500];
double fcos[500][500];


inline double dis(double x1, double y1, double x2, double y2)
{
    return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

inline int int5(double x)
{
    return floor(x+0.5);
}

int init(const char * filename, int thresh)
{
    namedWindow("Display", 0);
    cout << "Loading the image." << endl;
    img = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    
    cout << "Adjusting the color." << endl;
    int i, j;
    for(i=1; i<500; i++)
        for(j=0; j<i; j++)
        {
            fsin[j][i] = sin(1.0*j/i*2*PI);
            fcos[j][i] = cos(1.0*j/i*2*PI);
        }
    //  adding a MAXD pixel frame to the original image
    raw = Mat::zeros(img.rows+MAXD*2, img.cols+MAXD*2, CV_8UC1);
    for(i = 0; i<img.rows; i++)
        for(j = 0; j<img.cols; j++)
            if(img.at<uchar>(i,j) < thresh) raw.at<uchar>(i+MAXD,j+MAXD) = 0; else raw.at<uchar>(i+MAXD,j+MAXD) = 255;
    imshow("Display", raw);
    imwrite("raw.jpg", raw);
    waitKey(0);
    return 0;
}

int tanCheck(int x, int y, double r, double& d)
{
    int sampleRate = int5(2*PI*r/2)*4;
    int sample[sampleRate];
    
    int wx, wy, whead, wsample, wprev, k, i, j, wk;
    k = 0;
    
    bool flag;
    
    flag = false;
    for(i = 0; i<sampleRate; i++)
    {
        wx = x + int5(r*fcos[i][sampleRate]);
        wy = y + int5(r*fsin[i][sampleRate]);
        wsample = raw.at<uchar>(wx, wy);
        if(wsample > 100) wsample = 255;
        if(wsample==0) return -1;
        if(wsample==100) flag = true;
        sample[i] = wsample;
    }
    
    if(!flag) return 0;
    
    int l;
    for(i = 0; i<sampleRate; i++)
        if(sample[i] == 255)
        {
            l = 1;
            j = i+1;
            while(j<sampleRate && sample[j]==255)
            {
                l++;
                j++;
            }
            if(j==sampleRate) j = 0;
            while(j<sampleRate && sample[j]==255)
            {
                l++;
                j++;
            }
            j = i-1;
            while(j>=0 && sample[j]==255)
            {
                l++;
                j--;
            }
            if(j==-1) j = sampleRate-1;
            while(j>=0 && sample[j]==255)
            {
                l++;
                j--;
            }
            if(l>1.0*sampleRate/5)
            {
                k++;
                j = i+1;
                while(j<sampleRate && sample[j]==255)
                {
                    sample[j] = 254;
                    j++;
                }
                if(j==sampleRate) j = 0;
                while(j<sampleRate && sample[j]==255)
                {
                    sample[j] = 254;
                    j++;
                }
                j = i-1;
                while(j>=0 && sample[j]==255)
                {
                    sample[j] = 254;
                    j--;
                }
                if(j==-1) j = sampleRate-1;
                while(j>=0 && sample[j]==255)
                {
                    sample[j] = 254;
                    j--;
                }
            }
        }
    
    d = 0;
    wk = 0;
    for(i = 0; i<sampleRate; i++)
    {
        wx = x + int5(r*fcos[i][sampleRate]);
        wy = y + int5(r*fsin[i][sampleRate]);
        wsample = raw.at<uchar>(wx, wy);
        if(wsample == 100)
        {
            wk++;
            d += dis(x,y,wx,wy);
        }
    }
    d /= wk;
    d *= 2;
    return k;
}


int erase(int x, int y, int r)
{
    int i, j;
    for(i = x-r; i<x+r+1; i++)
        for(j = y-r; j<y+r+1; j++)
            if(dis(x,y,i,j)<=r) raw.at<uchar>(i,j) = 200;
    return 0;
}

int diameterCount()
{
    int i, j, k;
    Mat raw2 = Mat::zeros(img.rows+MAXD*2, img.cols+MAXD*2, CV_8UC1);
    cout << "Detecting boundary." << endl;
    for(i = 1; i<raw.rows-1; i++)
        for(j = 1; j<raw.cols-1; j++)
        {
            k = raw.at<uchar>(i+1,j) + raw.at<uchar>(i+1,j+1) + raw.at<uchar>(i,j+1) + raw.at<uchar>(i-1,j+1) + raw.at<uchar>(i-1,j) + raw.at<uchar>(i-1,j-1) + raw.at<uchar>(i,j-1) + raw.at<uchar>(i+1,j-1);
            if((k>0) && (k<2040) && (raw.at<uchar>(i,j)==255)) raw2.at<uchar>(i,j) = 100; else raw2.at<uchar>(i,j) = raw.at<uchar>(i,j);
        }
    raw = Mat(raw2);
    imshow("Display", raw);
    imwrite("raw.jpg", raw);
    waitKey(0);
    
    /*
     cout << "Zooming." << endl;
     
     raw2 = Mat::zeros(raw.rows*2, raw.cols*2, CV_8UC1);
     for(i = 0; i<raw2.rows; i++)
     for(j = 0; j<raw2.cols; j++)
     raw2.at<Vec3b>(i,j) = raw.at<Vec3b>(i/2, j/2);
     raw = Mat(raw2);
     */
    
    nans = 0;
    double r, d;
    int tanNo;
    bool flag;
    cout << "Counting diameters." << endl;
    for(i = 0; i<raw.rows; i++)
        for(j = 0; j<raw.cols; j++)
            if(raw.at<uchar>(i,j) == 255)
            {
                flag = false;
                for(r = 1; r<MAXD; r+= 0.25)
                {
                    tanNo = tanCheck(i, j, r, d);
                    if(tanNo==-1) break;
                    else if(tanNo==0) continue;
                    else if(tanNo==1) continue;
                    else if(tanNo==2)
                    {
                        raw.at<uchar>(i,j) = 250;
                        ansCo[nans][0] = i;
                        ansCo[nans][1] = j;
                        ansD[nans] = d;
                        nans++;
                        break;
                    }
                    else
                    {
                        //                     erase(i,j,floor(r));
                        break;
                    }
                }
            }
    return 0;
}

int inCheck(int x, int y, double r)
{
    int sampleRate = int5(2*PI*r/6)*4;
    int sample[sampleRate];
    
    int wx, wy, whead, wsample, wprev, k, i, j, wk;
    k = 0;
    
    bool flag;
    
    flag = false;
    for(i = 0; i<sampleRate; i++)
    {
        wx = x + int5(r*fcos[i][sampleRate]);
        wy = y + int5(r*fsin[i][sampleRate]);
        wsample = raw.at<uchar>(wx, wy);
        if(wsample==0 || wsample == 100) return 0;
        if(wsample > 251) wsample = 255; else wsample = 250;
        sample[i] = wsample;
    }
    
    int l;
    for(i = 1; i<sampleRate; i++)
        if(sample[i]!=sample[i-1]) k++;
    if(sample[0]!=sample[sampleRate-1]) k++;
    return k;
}

int output()
{
    ofstream ouf("output.txt");
    int i, j;
    double w;
    for(i = 0; i<nans; i++)
        for(w = 1; w<ansD[i]/2; w+=1)
           if(inCheck(ansCo[i][0],ansCo[i][1], w)>4)
           {
               erase(ansCo[i][0],ansCo[i][1], ansD[i]/2);
               break;
           }
    for(i = 0; i<nans; i++)
        if(raw.at<uchar>(ansCo[i][0],ansCo[i][1]) == 250) ouf << ansD[i] << endl;
    ouf.close();
    
    Mat outImg = Mat(raw.rows, raw.cols, CV_8UC3);
    for(i = 0; i<raw.rows; i++)
        for(j = 0; j<raw.cols; j++)
            if(raw.at<uchar>(i,j) == 0)
            {
                outImg.at<Vec3b>(i,j) = Vec3b(0,0,0);
            }
            else if(raw.at<uchar>(i,j) == 100)
            {
                outImg.at<Vec3b>(i,j) = Vec3b(100,100,100);
            }
            else if(raw.at<uchar>(i,j) == 200)
            {
                outImg.at<Vec3b>(i,j) = Vec3b(50, 50, 100);
            }
            else if(raw.at<uchar>(i,j) == 250)
            {
                outImg.at<Vec3b>(i,j) = Vec3b(200,0,0);
            }
            else if(raw.at<uchar>(i,j) == 255)
            {
                outImg.at<Vec3b>(i,j) = Vec3b(255,255,255);
            }
    imshow("Display", outImg);
    waitKey(0);
    imwrite("Result.jpg", outImg);
    return 0;
}

int main(int argc, const char * argv[])
{
    // insert code here...
    init(argv[1], atoi(argv[2]));
    diameterCount();
    output();
    return 0;
}
