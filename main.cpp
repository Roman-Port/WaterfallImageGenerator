#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>
#include <png.h>
#include <volk/volk.h>

#include "wav.h"
#include "color_table.h"
#include "window.h"

void offset_spectrum(lv_32fc_t* buffer, int count) {
	count /= 2;
	lv_32fc_t* left = buffer;
	lv_32fc_t* right = buffer + count;
	lv_32fc_t temp;
	for (int i = 0; i < count; i++)
	{
		temp = *left;
		*left++ = *right;
		*right++ = temp;
	}
}

void apply_window(fftwf_complex* in, float* taps, int count) {
	float* dst = (float*)in;
	for (int i = 0; i < count; i++) {
		*dst++ *= *taps;
		*dst++ *= *taps++;
	}
}

bool parse_args(int argc, char* argv[], int* width, int* height, int* offset, int* range, FILE** input, FILE** output) {
	//Make sure there is a valid number
	if (argc != 7) {
		printf("Usage: %s [input filename (.wav)] [width (px)] [height (px)] [offset (dB)] [range (dB)] [output filename (.png)]\n", argv[0]);
		return 1;
	}

	//Read
	(*width) = atoi(argv[2]);
	(*height) = atoi(argv[3]);
	(*offset) = atoi(argv[4]);
	(*range) = atoi(argv[5]);

	//Validate
	if (*width < 1 || *height < 1) {
		printf("Invalid width or height: %ix%i\n", *width, *height);
		return 1;
	}

	//Open input file
	(*input) = fopen(argv[1], "rb");
	if (*input == 0) {
		printf("Unable to open input file.\n");
		return 1;
	}

	//Open output file
	(*output) = fopen(argv[6], "wb");
	if (*output == 0) {
		printf("Unable to open output file.\n");
		return 1;
	}
}

int main(int argc, char* argv[])
{
	//Log name
	printf("Spectrum Image Generator by RomanPort\n");

	//Read args
	int width;
	int height;
	int offset;
	int range;
	FILE* input;
	FILE* output;
	if (!parse_args(argc, argv, &width, &height, &offset, &range, &input, &output))
		return 1;

	//Parse WAV file
	uint8_t headerData[WAV_HEADER_SIZE];
	wav_header_data header;
	if (fread(headerData, 1, WAV_HEADER_SIZE, input) != WAV_HEADER_SIZE || !read_wav_header(headerData, &header)) {
		printf("Unable to parse WAV file: %s\n", argv[1]);
		return 1;
	}
	if (header.bits_per_sample != 16 || header.channels != 2) {
		printf("Sorry, this WAV file is unsupported. Only two channel, 16 bits-per-sample files are supported.\n");
		return 1;
	}

	//Log info
	printf("Processing image...\n");
	printf("Width  (px) : %i\n", width);
	printf("Height (px) : %i\n", height);
	printf("Offset (db) : %i\n", offset);
	printf("Range  (dB) : %i\n", range);

	//Allocate buffers
	int16_t* bufferFileInput = (int16_t*)fftwf_malloc(sizeof(int16_t) * 2 * width);
	float* bufferPower = (float*)fftwf_malloc(sizeof(float) * width);
	float* window = (float*)fftwf_malloc(sizeof(float) * width);

	//Create FFT window
	create_window(window, width);

	//Set up FFTW
	fftwf_complex* bufferFftInput = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * width);
	fftwf_complex* bufferFftOutput = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * width);
	fftwf_plan plan = fftwf_plan_dft_1d(width, bufferFftInput, bufferFftOutput, FFTW_FORWARD, FFTW_ESTIMATE);

	//Allocate a row for the image
	png_byte* row = (png_byte*)malloc(3 * width * sizeof(png_byte));

	//Initialize PNG writing
	png_struct* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info* info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, output);
	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	//Loop through each row
	for (int y = 0; y < height; y++) {
		//Read block from the WAV file
		fread(bufferFileInput, sizeof(int16_t) * 2, width, input);

		//Convert to float
		for (int i = 0; i < width; i++) {
			bufferFftInput[i][0] = bufferFileInput[(i * 2) + 0] / 32768.0;
			bufferFftInput[i][1] = bufferFileInput[(i * 2) + 1] / 32768.0;
		}

		//Apply windowing function
		apply_window(bufferFftInput, window, width);

		//Compute FFT
		fftwf_execute(plan);

		//Offset the spectrum to center
		offset_spectrum((lv_32fc_t*)bufferFftOutput, width);

		//Calculate magnitude squared
		volk_32fc_magnitude_squared_32f(bufferPower, (lv_32fc_t*)bufferFftOutput, width);

		//Draw image
		float value;
		int valueInt;
		for (int i = 0; i < width; i++) {
			//Scale to dB https://github.com/gnuradio/gnuradio/blob/1a0be2e6b54496a8136a64d86e372ab219c6559b/gr-utils/plot_tools/plot_fft_base.py#L88
			value = -(20 * log10(abs((bufferPower[i] + 1e-15f) / width)));

			//Scale by offset and range, then scale up to [0-255]
			valueInt = (int)(((value - offset) / range) * 255);

			//Constrain
			if (valueInt > 255)
				valueInt = 255;
			if (valueInt < 0)
				valueInt = 0;

			//Lookup this color in the color table and apply
			memcpy(&row[i * 3], &WATERFALL_COLOR_TABLE[(int)(valueInt * 3)], 3);
		}

		//Write row
		png_write_row(png_ptr, row);
	}

	//Finish PNG writing
	png_write_end(png_ptr, NULL);

	//Close files
	fclose(input);
	fclose(output);

	//Clean up
	fftwf_free(bufferFileInput);
	fftwf_free(bufferPower);
	fftwf_free(window);
	fftwf_free(bufferFftInput);
	fftwf_free(bufferFftOutput);
	fftwf_destroy_plan(plan);
	png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	free(row);

	//Log
	printf("Success.\n");

	return 0;
}
