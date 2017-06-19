#include "pch.h"
#include "HoloPlugin.h" 

#define DLL_API __declspec(dllexport) 
#define THRESH_1 100
#define THRESH_2 300
#define APERTURE_SIZE 3

using namespace cv;
using namespace std;

extern "C" {

#pragma region SKIN_DETECTION

	/*
	* Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
	* Released to public domain under terms of the BSD Simplified license.
	*
	* Redistribution and use in source and binary forms, with or without
	* modification, are permitted provided that the following conditions are met:
	*   * Redistributions of source code must retain the above copyright
	*     notice, this list of conditions and the following disclaimer.
	*   * Redistributions in binary form must reproduce the above copyright
	*     notice, this list of conditions and the following disclaimer in the
	*     documentation and/or other materials provided with the distribution.
	*   * Neither the name of the organization nor the names of its contributors
	*     may be used to endorse or promote products derived from this software
	*     without specific prior written permission.
	*
	*   See <http:www.opensource.org/licenses/bsd-license>
	*/
	DLL_API bool R1(int R, int G, int B) {
		bool e1 = (R>95) && (G>40) && (B>20) && ((max(R, max(G, B)) - min(R, min(G, B)))>15) && (abs(R - G)>15) && (R>G) && (R>B);
		bool e2 = (R>220) && (G>210) && (B>170) && (abs(R - G) <= 15) && (R>B) && (G>B);
		return (e1 || e2);
	}

	DLL_API bool R2(float Y, float Cr, float Cb) {
		bool e3 = Cr <= 1.5862*Cb + 20;
		bool e4 = Cr >= 0.3448*Cb + 76.2069;
		bool e5 = Cr >= -4.5652*Cb + 234.5652;
		bool e6 = Cr <= -1.15*Cb + 301.75;
		bool e7 = Cr <= -2.2857*Cb + 432.85;
		return e3 && e4 && e5 && e6 && e7;
	}

	DLL_API bool R3(float H, float S, float V) {
		return (H<25) || (H > 230);
	}

	DLL_API Mat GetSkin(Mat const &src) {
		// allocate the result matrix
		Mat dst = src.clone();

		Vec3b cwhite = Vec3b::all(255);
		Vec3b cblack = Vec3b::all(0);

		Mat src_ycrcb, src_hsv;
		// OpenCV scales the YCrCb components, so that they
		// cover the whole value range of [0,255], so there's
		// no need to scale the values:
		cvtColor(src, src_ycrcb, CV_BGR2YCrCb);
		// OpenCV scales the Hue Channel to [0,180] for
		// 8bit images, so make sure we are operating on
		// the full spectrum from [0,360] by using floating
		// point precision:
		src.convertTo(src_hsv, CV_32FC3);
		cvtColor(src_hsv, src_hsv, CV_BGR2HSV);
		// Now scale the values between [0,255]:
		normalize(src_hsv, src_hsv, 0.0, 255.0, NORM_MINMAX, CV_32FC3);

		for (int i = 0; i < src.rows; i++) {
			for (int j = 0; j < src.cols; j++) {

				Vec3b pix_bgr = src.ptr<Vec3b>(i)[j];
				int B = pix_bgr.val[0];
				int G = pix_bgr.val[1];
				int R = pix_bgr.val[2];
				// apply rgb rule
				bool a = R1(R, G, B);

				Vec3b pix_ycrcb = src_ycrcb.ptr<Vec3b>(i)[j];
				int Y = pix_ycrcb.val[0];
				int Cr = pix_ycrcb.val[1];
				int Cb = pix_ycrcb.val[2];
				// apply ycrcb rule
				bool b = R2(Y, Cr, Cb);

				Vec3f pix_hsv = src_hsv.ptr<Vec3f>(i)[j];
				float H = pix_hsv.val[0];
				float S = pix_hsv.val[1];
				float V = pix_hsv.val[2];
				// apply hsv rule
				bool c = R3(H, S, V);

				if (!(a&&b&&c))
					dst.ptr<Vec3b>(i)[j] = cblack;
			}
		}
		return dst;
	}

	//END COPYRIGHT
#pragma endregion

	DLL_API int DetectContours(unsigned char* inPixels, int width, int height, unsigned char* outPixels) {
		RNG rng(12345);

		Mat gray, edges, equal, blurred, dilated;

		int max = 0;
		int maxInd = 0;
		int dilation_size = 3;
		int frameSize = width * height * 4;

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

#pragma region DETECT_HANDS

		//Get skin pixels from input frame
		Mat skin = GetSkin(frame);
		//Split RGBA channels from Mat into vector
		split(frame, rgbaSeparated);

		//Convert to 1 channel grayscale Mat of size height, width 
		cvtColor(frame, gray1Channel, COLOR_BGR2GRAY, 1);
		equalizeHist(gray1Channel, gray1Channel);

		//Blur the image
		GaussianBlur(gray1Channel, blurred, Size(17, 17), 0, 0);

		//Dilate in order to connect contours better 
		Mat element = getStructuringElement(MORPH_ELLIPSE, Size(2 * dilation_size + 1, 2 * dilation_size + 1), Point(dilation_size, dilation_size));

		erode(blurred, dilated, element);
		dilate(dilated, dilated, element);
		erode(dilated, dilated, element);

		//Find contours in the image, hierarchy stored in tree order
		findContours(dilated, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
		
		if (contours.size() > 0) {
			Mat drawing = Mat::zeros(dilated.size(), CV_8UC3);

			//Get the max contour in the image, preferably a hand or face
			for (size_t i = 0; i < contours.size(); i++)
			{
				if (contourArea(contours.at(i)) > max)
				{
					max = contourArea(contours.at(i));
					maxInd = i;
				}
			}

			//Light Blue
			Scalar color = Scalar(255, 245, 6);

			//Draw the largest contour on the Mat drawing
			drawContours(drawing, contours, maxInd, color, FILLED);

			//Detected contours transferred from Mat drawing to vector rgbSeparated
			split(drawing, rgbSeparated);
		}
		else {
			//Explicitly copy to the outbuffer
			memcpy(outPixels, frame.data, frameSize);

			//Sanity check for releasing cv::Mat memory
			frame.release();

			return 1;
		}

		//Reset rgba channels with contours data in graySeparated
		for (auto i = 0; i < rgbSeparated.size(); ++i) {
			rgbaSeparated[i] = rgbSeparated[i];
		}

		//Merge into a mat so that we can set outPixels correctly
		merge(rgbaSeparated, rgbaCombined);

		//Reset size var to size of gray4Channel, ideally should be same as width*height*4
		frameSize = rgbaCombined.cols * rgbaCombined.rows * rgbaCombined.elemSize();
#pragma endregion

#pragma region FIND_CONTOURS

		////Split RGBA channels from Mat into vector
		//split(frame, rgbaSeparated);
		//
		////Convert to 1 channel grayscale Mat of size height, width 
		//cvtColor(frame, gray1Channel, COLOR_BGR2GRAY, 1);
		//equalizeHist(gray1Channel, gray1Channel);
		//
		////Blur picture for better results, also try Size(3, 3)
		////3,3 too fine grained, 7,7 too fine grained, 11,11 just right
		//GaussianBlur(gray1Channel, gray1Channel, Size(11, 11), 0, 0);
		//
		////Edge detection on gray frame 
		//Canny(gray1Channel, gray1Channel, THRESH_1, THRESH_2, APERTURE_SIZE);
		//
		//gray1Channel.convertTo(gray1Channel, CV_8U);
		//
		////Dilate in order to connect contours better 
		//Mat element = getStructuringElement(MORPH_ELLIPSE, 
		//	Size(2 * dilationSize + 1, 2 * dilationSize + 1),
		//	Point(dilationSize, dilationSize));
		//dilate(gray1Channel, gray1Channel, element);
		//
		////Find contours in image, stored in contours vector
		//findContours(gray1Channel, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
		//
		//Mat drawing = Mat::zeros(Size(width,height), CV_8UC3);
		//for (auto i = 0; i< contours.size(); i++)
		//{
		//	//Random color set
		//	Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		//	//Draw contours stored in contours vector onto Mat drawing
		//	drawContours(drawing, contours, (int)(i), color, FILLED);
		//}
		//
		////Detected contours transferred from Mat drawing to vector rgbSeparated
		//split(drawing, rgbSeparated);
		//
		////Reset rgba channels with contours data in graySeparated
		//for (auto i = 0; i < rgbSeparated.size(); ++i) {
		//	rgbaSeparated[i] = rgbSeparated[i];
		//}
		//
		////Merge into a mat so that we can set outPixels correctly
		//merge(rgbaSeparated, rgbaCombined);
		//
		////Reset size var to size of gray4Channel, ideally should be same as width*height*4
		//frameSize = rgbaCombined.cols * rgbaCombined.rows * rgbaCombined.elemSize();
#pragma endregion 

		//Explicitly copy to the outbuffer
		memcpy(outPixels, rgbaCombined.data, frameSize);

		//Sanity check for releasing cv::Mat memory
		frame.release();

		return 1;
	}

}