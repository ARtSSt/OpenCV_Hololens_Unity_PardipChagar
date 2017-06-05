#pragma once

extern "C" {
	__declspec(dllexport) int DetectContours(unsigned char* inPixels, int width, int height, unsigned char* outPixels);
}