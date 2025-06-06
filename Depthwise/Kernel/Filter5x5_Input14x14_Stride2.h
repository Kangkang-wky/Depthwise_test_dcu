/*
Depthwise Convolution Kernel.

Case: filter 5 x 5, input 14 x 14, stride 2, padding 2

The number of channel must be multiple of 32.
Used in the MobileNet V2 and EfficientNet B0, in case of.
	1)	14 x 14 x 672 -> 14 x 14 x 672, stride = 2, filter = 5
*/
__global__ void Filter5x5_Input14x14_Stride2(const float* input, const float* filter, float* output,
	int inputBatchNumber, int inputChannel, int inputHeight, int inputWidth,
	int filterLayerNumber, int filterHeight, int filterWidth,
	int outputBatchNumber, int outputChannel, int outputHeight, int outputWidth,
	int padding, int stride,
	float alpha, float beta) {

	// every 32 channels is a group.
	__shared__ float filterData[32 * 25];	// filter is 5 x 5 = 25
	__shared__ float inputData[32 * 14 * 18]; // original input is 14 x 14, padded to be 18 x 18. ignore up and bottom padding, so 14 x 18

	float inTemp0, inTemp1, inTemp2, inTemp3, inTemp4;
	float sum0, sum1, sum2;  // to accumulate the row sum result. rolling recycle.

	int channelGroupSize = 32;
	int paddedWidth = inputWidth + 2 * padding;

	// load filter
	int filterLoadSrcIdx = blockIdx.y * channelGroupSize * filterWidth * filterHeight + threadIdx.x;
	if (threadIdx.x < 8 * 25) {
		filterData[threadIdx.x] = filter[filterLoadSrcIdx];	// 8 * 25
		filterData[threadIdx.x + 8 * 25] = filter[filterLoadSrcIdx + 8 * 25]; // 16 * 25
		filterData[threadIdx.x + 8 * 25 * 2] = filter[filterLoadSrcIdx + 8 * 25 * 2]; // 24 * 25
		filterData[threadIdx.x + 8 * 25 * 3] = filter[filterLoadSrcIdx + 8 * 25 * 3]; // 32 * 25, filter loaded
	}

	// set left and right padding
	int leftPaddingIdx = threadIdx.x * paddedWidth;
	// Upper half, left side
	inputData[leftPaddingIdx] = 0;
	inputData[leftPaddingIdx + 1] = 0;
	// Upper half, right side
	inputData[leftPaddingIdx + 16] = 0;
	inputData[leftPaddingIdx + 17] = 0;

	// Bottom half, left side
	inputData[leftPaddingIdx + 16 * inputHeight * paddedWidth] = 0;
	inputData[leftPaddingIdx + 16 * inputHeight * paddedWidth + 1] = 0;
	// Bottom half, right side
	inputData[leftPaddingIdx + 16 * inputHeight * paddedWidth + 16] = 0;
	inputData[leftPaddingIdx + 16 * inputHeight * paddedWidth + 17] = 0;
	__syncthreads();


	// load input
	// for all threads in the same block, use blockIdx.x to find correct batch index, use blockIdx.y to find correct input channel.
	int inputLoadIdxBase = blockIdx.x * inputChannel * inputHeight * inputWidth + blockIdx.y * channelGroupSize * inputHeight * inputWidth;
	int inputLoadSrcIdx = inputLoadIdxBase + threadIdx.x;	// each thread find its own load source.
	int inputLoadDstIdx = (threadIdx.x / inputWidth) * 4 + threadIdx.x + 2;	// each thread find its own load destination.

	inputData[inputLoadDstIdx] = input[inputLoadSrcIdx];
	inputData[inputLoadDstIdx + 16 * 18 * 1] = input[inputLoadSrcIdx + 16 * 14 * 1];
	inputData[inputLoadDstIdx + 16 * 18 * 2] = input[inputLoadSrcIdx + 16 * 14 * 2];
	inputData[inputLoadDstIdx + 16 * 18 * 3] = input[inputLoadSrcIdx + 16 * 14 * 3];
	inputData[inputLoadDstIdx + 16 * 18 * 4] = input[inputLoadSrcIdx + 16 * 14 * 4];
	inputData[inputLoadDstIdx + 16 * 18 * 5] = input[inputLoadSrcIdx + 16 * 14 * 5];
	inputData[inputLoadDstIdx + 16 * 18 * 6] = input[inputLoadSrcIdx + 16 * 14 * 6];
	inputData[inputLoadDstIdx + 16 * 18 * 7] = input[inputLoadSrcIdx + 16 * 14 * 7];
	inputData[inputLoadDstIdx + 16 * 18 * 8] = input[inputLoadSrcIdx + 16 * 14 * 8];
	inputData[inputLoadDstIdx + 16 * 18 * 9] = input[inputLoadSrcIdx + 16 * 14 * 9];
	inputData[inputLoadDstIdx + 16 * 18 * 10] = input[inputLoadSrcIdx + 16 * 14 * 10];
	inputData[inputLoadDstIdx + 16 * 18 * 11] = input[inputLoadSrcIdx + 16 * 14 * 11];
	inputData[inputLoadDstIdx + 16 * 18 * 12] = input[inputLoadSrcIdx + 16 * 14 * 12];
	inputData[inputLoadDstIdx + 16 * 18 * 13] = input[inputLoadSrcIdx + 16 * 14 * 13];
	inputData[inputLoadDstIdx + 16 * 18 * 14] = input[inputLoadSrcIdx + 16 * 14 * 14];
	inputData[inputLoadDstIdx + 16 * 18 * 15] = input[inputLoadSrcIdx + 16 * 14 * 15];
	inputData[inputLoadDstIdx + 16 * 18 * 16] = input[inputLoadSrcIdx + 16 * 14 * 16];
	inputData[inputLoadDstIdx + 16 * 18 * 17] = input[inputLoadSrcIdx + 16 * 14 * 17];
	inputData[inputLoadDstIdx + 16 * 18 * 18] = input[inputLoadSrcIdx + 16 * 14 * 18];
	inputData[inputLoadDstIdx + 16 * 18 * 19] = input[inputLoadSrcIdx + 16 * 14 * 19];
	inputData[inputLoadDstIdx + 16 * 18 * 20] = input[inputLoadSrcIdx + 16 * 14 * 20];
	inputData[inputLoadDstIdx + 16 * 18 * 21] = input[inputLoadSrcIdx + 16 * 14 * 21];
	inputData[inputLoadDstIdx + 16 * 18 * 22] = input[inputLoadSrcIdx + 16 * 14 * 22];
	inputData[inputLoadDstIdx + 16 * 18 * 23] = input[inputLoadSrcIdx + 16 * 14 * 23];
	inputData[inputLoadDstIdx + 16 * 18 * 24] = input[inputLoadSrcIdx + 16 * 14 * 24];
	inputData[inputLoadDstIdx + 16 * 18 * 25] = input[inputLoadSrcIdx + 16 * 14 * 25];
	inputData[inputLoadDstIdx + 16 * 18 * 26] = input[inputLoadSrcIdx + 16 * 14 * 26];
	inputData[inputLoadDstIdx + 16 * 18 * 27] = input[inputLoadSrcIdx + 16 * 14 * 27];
	__syncthreads();

	// convolution
	int outputIdx = blockIdx.x * outputChannel * outputHeight * outputWidth +
		blockIdx.y * channelGroupSize * outputHeight * outputWidth +
		(threadIdx.x / outputWidth) * outputHeight * outputWidth +
		threadIdx.x % outputWidth;

	int inputAccessBase = (threadIdx.x / outputWidth) * paddedWidth * inputHeight + threadIdx.x % outputWidth * 2;
	int filterAccessBase = (threadIdx.x / outputWidth) * filterHeight * filterWidth;
	int inputAccessOffset = 0;

	// 1st row
	// convolve with filter 2 times:
	// 		1. filter's 3rd row (when filter is sliding through the 1st row of input)
	//		2. filter's 1st row (when filter is sliding through the 3rd row of input)
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = filterData[filterAccessBase + 10] * inTemp0;
	sum1 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 11] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 12] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 13] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 14] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 4] * inTemp4;

	// 2nd row
	// convolve with filter 2 times:
	//		1. filter's 4th row (when filter is sliding through the 1st row of input)
	//		2. filter's 2nd row (when filter is sliding through the 3rd row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 15] * inTemp0;
	sum1 = sum1 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 16] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 17] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 18] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 19] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 9] * inTemp4;

	// 3rd row
	// convolve with filter 3 times:
	//		1. filter's 5th row (when filter is sliding through the 1st row of input)
	//		2. filter's 3rd row (when filter is sliding through the 3rd row of input) 
	//		3. filter's 1st row (when filter is sliding through the 5th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 20] * inTemp0;
	sum1 = sum1 + filterData[filterAccessBase + 10] * inTemp0;
	sum2 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 21] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 11] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 22] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 12] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 23] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 13] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 24] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 14] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 4] * inTemp4;
	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

	// 4th row
	// convolve with filter 2 times:
	// 		1. filter's 4th row (when filter is sliding through the 3rd row of input) 
	//		2. filter's 2nd row (when filter is sliding through the 5th row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 15] * inTemp0;
	sum2 = sum2 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 16] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 17] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 18] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 19] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 9] * inTemp4;

	// 5th row
	// convolve with filter 3 times:
	//		1. filter's 5th row (when filter is sliding through the 3rd row of input)
	//		2. filter's 3rd row (when filter is sliding through the 5th row of input) 
	//		3. filter's 1st row (when filter is sliding through the 7th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 20] * inTemp0;
	sum2 = sum2 + filterData[filterAccessBase + 10] * inTemp0;
	sum0 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 21] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 11] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 22] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 12] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 23] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 13] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 24] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 14] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 4] * inTemp4;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;

	// 6th row
	// convolve with filter 2 times:
	// 		1. filter's 4th row (when filter is sliding through the 5th row of input) 
	// 		2. filter's 2nd row (when filter is sliding through the 7th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 15] * inTemp0;
	sum0 = sum0 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 16] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 17] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 18] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 19] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 9] * inTemp4;

	// 7th row
	// convolve with filter 3 times:
	//		1. filter's 5th row (when filter is sliding through the 5th row of input)
	//		2. filter's 3rd row (when filter is sliding through the 7th row of input)
	//		3. filter's 1st row (when filter is sliding through the 9th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 20] * inTemp0;
	sum0 = sum0 + filterData[filterAccessBase + 10] * inTemp0;
	sum1 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 21] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 11] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 22] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 12] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 23] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 13] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 24] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 14] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 4] * inTemp4;
	output[outputIdx] = sum2 * alpha + beta;
	outputIdx += outputWidth;

	// 8th row
	// convolve with filter 2 times:
	//		1. filter's 4th row (when filter is sliding through the 7th row of input)
	//		2. filter's 2nd row (when filter is sliding through the 9th row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 15] * inTemp0;
	sum1 = sum1 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 16] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 17] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 18] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 19] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 9] * inTemp4;

	// 9th row
	// convolve with filter 3 times:
	//		1. filter's 5th row (when filter is sliding through the 7th row of input)
	//		2. filter's 3rd row (when filter is sliding through the 9th row of input) 
	//		3. filter's 1st row (when filter is sliding through the 11th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 20] * inTemp0;
	sum1 = sum1 + filterData[filterAccessBase + 10] * inTemp0;
	sum2 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 21] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 11] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 22] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 12] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 23] * inTemp3;
	sum1 = sum1 + filterData[filterAccessBase + 13] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 24] * inTemp4;
	sum1 = sum1 + filterData[filterAccessBase + 14] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 4] * inTemp4;
	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

	// 10th row
	// convolve with filter 2 times:
	// 		1. filter's 4th row (when filter is sliding through the 9th row of input) 
	//		2. filter's 2nd row (when filter is sliding through the 11th row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 15] * inTemp0;
	sum2 = sum2 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 16] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 17] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 18] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 19] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 9] * inTemp4;

	// 11th row
	// convolve with filter 3 times:
	//		1. filter's 5th row (when filter is sliding through the 9th row of input)
	//		2. filter's 3rd row (when filter is sliding through the 11th row of input) 
	//		3. filter's 1st row (when filter is sliding through the 13th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 20] * inTemp0;
	sum2 = sum2 + filterData[filterAccessBase + 10] * inTemp0;
	sum0 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 21] * inTemp1;
	sum2 = sum2 + filterData[filterAccessBase + 11] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 22] * inTemp2;
	sum2 = sum2 + filterData[filterAccessBase + 12] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 2] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 23] * inTemp3;
	sum2 = sum2 + filterData[filterAccessBase + 13] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 3] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 24] * inTemp4;
	sum2 = sum2 + filterData[filterAccessBase + 14] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 4] * inTemp4;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;

	// 12th row
	// convolve with filter 2 times:
	// 		1. filter's 4th row (when filter is sliding through the 11th row of input) 
	// 		2. filter's 2nd row (when filter is sliding through the 13th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 15] * inTemp0;
	sum0 = sum0 + filterData[filterAccessBase + 5] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 16] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 6] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 17] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 7] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 18] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 8] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 19] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 9] * inTemp4;

	// 13th row
	// convolve with filter 2 times:
	//		1. filter's 5th row (when filter is sliding through the 11th row of input)
	// 		2. filter's 3rd row (when filter is sliding through the 13th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 20] * inTemp0;
	sum0 = sum0 + filterData[filterAccessBase + 10] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 21] * inTemp1;
	sum0 = sum0 + filterData[filterAccessBase + 11] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 22] * inTemp2;
	sum0 = sum0 + filterData[filterAccessBase + 12] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 23] * inTemp3;
	sum0 = sum0 + filterData[filterAccessBase + 13] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum2 = sum2 + filterData[filterAccessBase + 24] * inTemp4;
	sum0 = sum0 + filterData[filterAccessBase + 14] * inTemp4;

	output[outputIdx] = sum2 * alpha + beta;
	outputIdx += outputWidth;

	// 14th row
	// convolve with filter 1 time:
	//		1. filter's 4th row (when filter is sliding through the 13th row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 15] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 16] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 17] * inTemp2;

	inTemp3 = inputData[inputAccessBase + 3 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 18] * inTemp3;

	inTemp4 = inputData[inputAccessBase + 4 + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 19] * inTemp4;

	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;
}
