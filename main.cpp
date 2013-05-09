
#include "opencv2/core/core.hpp"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <sstream>

using namespace cv;
using namespace std;

const unsigned char valueThreshold = 40;
const unsigned char blackValueThreshold = 70;
const unsigned char whiteValueThreshold = 150;
const unsigned char saturationThreshold = 200;
const unsigned char initBlurDiameter = 7;
const unsigned char blackBlurDiameter = 13;

const int halfAngleLimit = 45; // degrees from 0

int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
double fontScale = 2;
int thickness = 3;

int main(int argc, char** argv)
{
    //const char* filename = argc >= 2 ? argv[1] : "pic1.png";

    CvCapture* capture;

    //-- 2. Read the video stream
    capture = cvCaptureFromCAM( -1 );

    if( capture )
    {
        while( true )
        {

            Mat src = cvQueryFrame( capture );
            if(src.empty())
            {
                cout << "can not open camera" << endl;
                return -1;
            }

            Mat dst, cdst, HSVsrc;
            stringstream anglestream (stringstream::in | stringstream::out);

            // blur?
//            GaussianBlur(src, src, Size(initBlurDiameter,initBlurDiameter), 1.5, 1.5);

            //  Filtering ////////////////////

            // Equalize histogram
            //equalizeHist( src, src ); // run on each channel?

            // Convert to HSV
            Mat HSVsrcs[3];
            cvtColor(src, HSVsrc, CV_RGB2HSV);
            split(HSVsrc, HSVsrcs);

            // equalize value channel
 //           equalizeHist(HSVsrcs[2], HSVsrcs[2]);

            // Threshold on Saturation near 255
            Mat highSaturation;
            threshold(HSVsrcs[1], highSaturation, saturationThreshold, 255, THRESH_BINARY);

            // Threshold on Value near 0
            Mat lowValue;
            threshold(HSVsrcs[2], lowValue, blackValueThreshold, 255, THRESH_BINARY_INV);

            // Threshold on Value near 255
            Mat highValue;
            threshold(HSVsrcs[2], highValue, whiteValueThreshold, 255, THRESH_BINARY);

            // And both of above
            Mat black(/*highSaturation &*/ lowValue);
            Mat white(/*highSaturation &*/ highValue);


            ///////////////////////////////////////

            Mat lowDilated, highDilated;
            dilate(lowValue, lowDilated, Mat());
            dilate(highValue, highDilated, Mat());

            Mat borders = lowDilated & highDilated;


            Mat showBorders;
            cvtColor(borders,showBorders,CV_GRAY2RGB);

            Mat horizEdgePos;
            reduce(borders, horizEdgePos, 0, CV_REDUCE_AVG);

            equalizeHist(horizEdgePos, horizEdgePos);

            float_t lineX = 0;

            for(int x = 0; x < horizEdgePos.cols; x++){
                lineX += (float)x * ((float)horizEdgePos.at<uchar>(x)/255);

            }

            lineX /= horizEdgePos.cols;

            float_t lineStdDev = 0;


            for(int x = 0; x < horizEdgePos.cols; x++){
                lineStdDev += powf(((float)x * ((float)horizEdgePos.at<uchar>(x)/255)) - lineX, 2);

            }

            lineStdDev /= horizEdgePos.cols;
            lineStdDev = sqrtf(lineStdDev);

            horizEdgePos.at<uchar>((int)lineX) = 255;

            Mat dispHorizEdge;
            repeat(horizEdgePos, 480, 1, dispHorizEdge);


            //equalizeHist(dispHorizEdge, dispHorizEdge);

            imshow("Horizontal posisions", dispHorizEdge);



//            GaussianBlur(black, black, Size(blackBlurDiameter,blackBlurDiameter), 1.5, 1.5);
//            threshold(black, black, 127, 255, THRESH_BINARY);
//            GaussianBlur(white, white, Size(blackBlurDiameter,blackBlurDiameter), 1.5, 1.5);
//            threshold(white, white, 127, 255, THRESH_BINARY);
            //cvtColor(dst, cdst, CV_GRAY2BGR);




            vector<Vec2f> lines;
            Mat blackEdges, whiteEdges;
            // find edges of black area
            Canny(black, blackEdges, 50, 200, 3);
            // find edges of white area
            Canny(white, whiteEdges, 50, 200, 3);

            Mat edges = whiteEdges + blackEdges;

            // convert to RGB so we can put red lines on it
            Mat showEdges;
            cvtColor(edges,showEdges,CV_GRAY2RGB);

            Mat showEdgesAndLines;
            cvtColor(edges,showEdgesAndLines,CV_GRAY2RGB);

            // find straight lines on edges of black area
            HoughLines(blackEdges, lines, 1, CV_PI/180, 100, 0, 0 );

            Mat linesFrame = src.clone();

            float lineAngle;

            // draw lines on frame if they are in the angle range
            for( size_t i = 0; i < lines.size(); i++ )
            {
                float rho = lines[i][0], theta = lines[i][1];
                if(theta < (halfAngleLimit*CV_PI/180) || theta > (CV_PI - (halfAngleLimit*CV_PI/180))){
                    lineAngle = theta;
                    Point pt1, pt2;
                    double a = cos(theta), b = sin(theta);
                    double x0 = a*rho, y0 = b*rho;
                    pt1.x = cvRound(x0 + 1000*(-b));
                    pt1.y = cvRound(y0 + 1000*(a));
                    pt2.x = cvRound(x0 - 1000*(-b));
                    pt2.y = cvRound(y0 - 1000*(a));
                    // draw lines on frames
                    line( linesFrame, pt1, pt2, Scalar(0,0,255), 3, CV_AA);
                    line( showEdgesAndLines, pt1, pt2, Scalar(0,0,255), 3, CV_AA);
                }
            }

            anglestream << lineStdDev;





//            imshow("Source Video", src);
//            imshow("Hue", HSVsrcs[0]);
//            imshow("Saturation", HSVsrcs[1]);
//            imshow("Value", HSVsrcs[2]);
//            imshow("High Saturation", highSaturation);
//            imshow("Low Value", lowValue);
//            imshow("Edges", src+showEdges);


//            imshow("Borders", borders);

            // Put angle on image

            string angle = anglestream.str();

            int baseline=0;
            Size textSize = getTextSize(angle, fontFace,
            fontScale, thickness, &baseline);
            baseline += thickness;
            // center the text
            Point textOrg((src.cols - textSize.width)/2,
            (src.rows + textSize.height)/2);

            // then put the text itself
            putText(src, angle, Point(10, 80), fontFace, fontScale,
            Scalar::all(255), thickness, 8);


//            imshow("Edges and Lines", src+showEdgesAndLines);
            imshow("Orig w/ Borders", src+showBorders);
            imshow("Value w/ Borders", HSVsrcs[2]+borders);

            //displayOverlay("Edges and Lines", angle);
//            imshow("Black", black);
//            imshow("Lines", linesFrame);


              int c = waitKey(10);
              if( (char)c == 'c' ) { break; }
        }
    }
    waitKey();

    return 0;
}

