#include "pch.h"
#include "HoloPlugin.h" 
#include <opencv2\core.hpp>
#include <opencv2\imgproc.hpp>
#include <roapi.h>

#define DLL_API __declspec(dllexport) 
#define THRESH_1 100
#define THRESH_2 300
#define APERTURE_SIZE 3

using namespace cv;
using namespace std;

extern "C" {

	DLL_API int DetectContours(unsigned char* inPixels, int width, int height, unsigned char* outPixels) {

		int frameSize = width * height * 4;
		int dilationSize = 3;

		RNG rng(12345);

		Mat frame = Mat(height, width, CV_8UC4);
		Mat gray1Channel = Mat(height, width, CV_8UC3);
		Mat rgbChannels = Mat(height, width, CV_8UC3);
		Mat rgbaCombined = Mat(height, width, CV_8UC4);

		vector<Mat> rgbaSeparated;
		vector<Mat> rgbSeparated;
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;

		//Must explicitly copy data from unsigned char* inPixels to Mat frame
		memcpy(frame.data, inPixels, frameSize);

		//Image comes in flipped on vertical axis, correcting below
		flip(frame, frame, 1);

#pragma region FIND_CONTOURS

		//Split RGBA channels from Mat into vector
		split(frame, rgbaSeparated);

		//Convert to 1 channel grayscale Mat of size height, width 
		cvtColor(frame, gray1Channel, COLOR_BGR2GRAY, 1);
		equalizeHist(gray1Channel, gray1Channel);
		
		//Blur picture for better results, also try Size(3, 3)
		//3,3 too fine grained, 7,7 too fine grained, 11,11 just right
		GaussianBlur(gray1Channel, gray1Channel, Size(11, 11), 0, 0);

		//Edge detection on gray frame 
		Canny(gray1Channel, gray1Channel, THRESH_1, THRESH_2, APERTURE_SIZE);

		gray1Channel.convertTo(gray1Channel, CV_8U);

		//Dilate in order to connect contours better 
		Mat element = getStructuringElement(MORPH_ELLIPSE, 
			Size(2 * dilationSize + 1, 2 * dilationSize + 1),
			Point(dilationSize, dilationSize));
		dilate(gray1Channel, gray1Channel, element);

		//Find contours in image, stored in contours vector
		findContours(gray1Channel, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

		Mat drawing = Mat::zeros(Size(width,height), CV_8UC3);
		for (auto i = 0; i< contours.size(); i++)
		{
			//Random color set
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			//Draw contours stored in contours vector onto Mat drawing
			drawContours(drawing, contours, (int)(i), color, FILLED);
		}

		//Detected contours transferred from Mat drawing to vector rgbSeparated
		split(drawing, rgbSeparated);

		//Reset rgba channels with contours data in graySeparated
		for (auto i = 0; i < rgbSeparated.size(); ++i) {
			rgbaSeparated[i] = rgbSeparated[i];
		}

		//Merge into a mat so that we can set outPixels correctly
		merge(rgbaSeparated, rgbaCombined);

		//Reset size var to size of gray4Channel, ideally should be same as width*height*4
		frameSize = rgbaCombined.cols * rgbaCombined.rows * rgbaCombined.elemSize();
#pragma endregion 

		//Explicitly copy to the outbuffer
		memcpy(outPixels, rgbaCombined.data, frameSize);

		//Sanity check for releasing cv::Mat memory
		frame.release();

		return 1;
	}

}