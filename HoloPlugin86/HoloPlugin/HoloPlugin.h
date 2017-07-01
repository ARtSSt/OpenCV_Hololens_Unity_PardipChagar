#pragma once
#include <opencv2\core.hpp>
#include <opencv2\imgproc.hpp>
#include <roapi.h>

using namespace cv;
using namespace std;
extern "C" {
	__declspec(dllexport) int DetectContours(unsigned char* inPixels, int width, int height, unsigned char* outPixels);
	__declspec(dllexport) Mat GetSkin(Mat const &src);
	__declspec(dllexport) bool R1(int R, int G, int B);
	__declspec(dllexport) bool R2(float Y, float Cr, float Cb);
	__declspec(dllexport) bool R3(float H, float S, float V);
}