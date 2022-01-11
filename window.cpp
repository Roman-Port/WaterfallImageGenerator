#define _USE_MATH_DEFINES
#include "window.h"

#include <math.h>
#include <algorithm>
#include <numeric>

//https://github.com/gnuradio/gnuradio/blob/master/gr-fft/lib/window.cc
void coswindow(float* taps, int ntaps, float c0, float c1, float c2) {
    float M = ntaps - 1;
    for (int n = 0; n < ntaps; n++)
        taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
        c2 * cosf((4.0f * M_PI * n) / M);
}

void create_window(float* taps, int ntaps) {
    coswindow(taps, ntaps, 0.44959, 0.49364, 0.05677);
}