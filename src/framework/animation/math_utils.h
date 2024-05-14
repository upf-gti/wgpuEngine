#pragma once

#include <cmath>
#include <iostream>

#define PI 3.14159265358979323846264338327950288
// Function to calculate the weight using Gaussian distribution
inline float gaussian_pdf(int frame, int peak_frame, float sigma)
{
    float exponent = -0.5f * pow((float)(frame - peak_frame) / sigma, 2.f);
    return exp(exponent);
}

// Function to calculate Gaussian kernel
inline float gaussian_filter(float value, float sigma)
{
    float exponent = -pow(value, 2) / (2 * pow(sigma, 2));
    return exp(exponent) / (sqrt(2 * PI) * sigma);
        
}
