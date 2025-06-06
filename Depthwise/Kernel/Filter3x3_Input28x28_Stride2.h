/*
Depthwise Convolution Kernel.

Case: filter 3 x 3, input 28 x 28, stride 2, padding 1

The number of channel must be multiple of 8.
Used in the MobileNet V2 and EfficientNet B0, in case of
	1)	28 x 28 x 192 -> 14 x 14 x 192, stride = 2, filter = 3
	1)	28 x 28 x 240 -> 14 x 14 x 240, stride = 2, filter = 3
*/
__global__ void Filter3x3_Input28x28_Stride2(const float* input, const float* filter, float* output,
	int inputBatchNumber, int inputChannel, int inputHeight, int inputWidth,
	int filterLayerNumber, int filterHeight, int filterWidth,
	int outputBatchNumber, int outputChannel, int outputHeight, int outputWidth,
	int padding, int stride,
	float alpha, float beta) {

	// every 8 channels is a group.
	__shared__ float filterData[8 * 9];	// filter is 3 x 3 = 9
	__shared__ float inputData[8 * 28 * 30]; // original input is 28 x 28, padded to be 30 x 30. ignore up and bottom padding, so 28 x 30

	float inTemp0, inTemp1, inTemp2;
	float sum0, sum1;  // to accumulate the row sum result. rolling recycle.

	int channelGroupSize = 8;
	int paddedWidth = inputWidth + 2 * padding;

	// load filter
	int filterLoadSrcIdx = blockIdx.y * channelGroupSize * filterWidth * filterHeight + threadIdx.x;
	if (threadIdx.x < 8 * 9) {
		filterData[threadIdx.x] = filter[filterLoadSrcIdx];
	}

	// set left and right padding
	int leftPaddingIdx = threadIdx.x * paddedWidth;
	inputData[leftPaddingIdx] = 0;
	inputData[leftPaddingIdx + 29] = 0;
	inputData[leftPaddingIdx + (channelGroupSize / 2) * inputHeight * paddedWidth] = 0;
	inputData[leftPaddingIdx + (channelGroupSize / 2) * inputHeight * paddedWidth + 29] = 0;

	__syncthreads();

	// load input
	// for all threads in the same block, use blockIdx.x to find correct batch index, use blockIdx.y to find correct input channel.
	int inputLoadIdxBase = blockIdx.x * inputChannel * inputHeight * inputWidth + blockIdx.y * channelGroupSize * inputHeight * inputWidth;
	int inputLoadSrcIdx = inputLoadIdxBase + threadIdx.x;	// each thread find its own load source.
	int inputLoadDstIdx = (threadIdx.x / inputWidth) * 2 + threadIdx.x + 1;	// each thread find its own load destination.

#pragma unroll
	for (int i = 0; i < 56; i++) {
		inputData[inputLoadDstIdx + 4 * 30 * i] = input[inputLoadSrcIdx + 4 * 28 * i];
	}
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
	// convolve with filter 1 time:
	// 		1. filter's 2nd row (when filter is sliding through the 1st row of input) 
	inTemp0 = inputData[inputAccessBase];
	sum0 = filterData[filterAccessBase + 3] * inTemp0;

	inTemp1 = inputData[inputAccessBase + 1];
	sum0 = sum0 + filterData[filterAccessBase + 4] * inTemp1;

	inTemp2 = inputData[inputAccessBase + 2];
	sum0 = sum0 + filterData[filterAccessBase + 5] * inTemp2;

	// 2nd row
	// convolve with filter 2 times:
	//		1. filter's 3rd row (when filter is sliding through the 1st row of input)
	//		2. filter's 1st row (when filter is sliding through the 3rd row of input) 
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[filterAccessBase + 6] * inTemp0;
	sum1 = filterData[filterAccessBase + 0] * inTemp0;

	inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[filterAccessBase + 7] * inTemp1;
	sum1 = sum1 + filterData[filterAccessBase + 1] * inTemp1;

	inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[filterAccessBase + 8] * inTemp2;
	sum1 = sum1 + filterData[filterAccessBase + 2] * inTemp2;

	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

#pragma unroll
	for (int i = 0; i < 6; i++) {
		// 3rd row
		// convolve with filter 1 time:
		// 		1. filter's 2nd row (when filter is sliding through the 3rd row of input)
		inputAccessOffset += paddedWidth;
		inTemp0 = inputData[inputAccessBase + inputAccessOffset];
		sum1 = sum1 + filterData[filterAccessBase + 3] * inTemp0;

		inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
		sum1 = sum1 + filterData[filterAccessBase + 4] * inTemp1;

		inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
		sum1 = sum1 + filterData[filterAccessBase + 5] * inTemp2;

		// 4th row
		// convolve with filter 2 times:
		//		1. filter's 3rd row (when filter is sliding through the 3rd row of input)
		//		2. filter's 1st row (when filter is sliding through the 5th row of input) 
		inputAccessOffset += paddedWidth;
		inTemp0 = inputData[inputAccessBase + inputAccessOffset];
		sum1 = sum1 + filterData[filterAccessBase + 6] * inTemp0;
		sum0 = filterData[filterAccessBase + 0] * inTemp0;

		inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
		sum1 = sum1 + filterData[filterAccessBase + 7] * inTemp1;
		sum0 = sum0 + filterData[filterAccessBase + 1] * inTemp1;

		inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
		sum1 = sum1 + filterData[filterAccessBase + 8] * inTemp2;
		sum0 = sum0 + filterData[filterAccessBase + 2] * inTemp2;

		output[outputIdx] = sum1 * alpha + beta;
		outputIdx += outputWidth;

		// 5th row
		// convolve with filter 1 time:
		// 		1. filter's 2nd row (when filter is sliding through the 5th row of input)
		inputAccessOffset += paddedWidth;
		inTemp0 = inputData[inputAccessBase + inputAccessOffset];
		sum0 = sum0 + filterData[filterAccessBase + 3] * inTemp0;

		inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
		sum0 = sum0 + filterData[filterAccessBase + 4] * inTemp1;

		inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
		sum0 = sum0 + filterData[filterAccessBase + 5] * inTemp2;

		// 6th row
		// convolve with filter 2 times:
		//		1. filter's 3rd row (when filter is sliding through the 5th row of input)
		//		2. filter's 1st row (when filter is sliding through the 7th row of input) 
		inputAccessOffset += paddedWidth;
		inTemp0 = inputData[inputAccessBase + inputAccessOffset];
		sum0 = sum0 + filterData[filterAccessBase + 6] * inTemp0;
		sum1 = filterData[filterAccessBase + 0] * inTemp0;

		inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
		sum0 = sum0 + filterData[filterAccessBase + 7] * inTemp1;
		sum1 = sum1 + filterData[filterAccessBase + 1] * inTemp1;

		inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
		sum0 = sum0 + filterData[filterAccessBase + 8] * inTemp2;
		sum1 = sum1 + filterData[filterAccessBase + 2] * inTemp2;

		output[outputIdx] = sum0 * alpha + beta;
		outputIdx += outputWidth;
	}

	// 27th row
	// convolve with filter 1 time:
	// 		1. filter's 2nd row (when filter is sliding through the 27th row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 3] * inTemp0;

	inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[filterAccessBase + 4] * inTemp1;

	inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[filterAccessBase + 5] * inTemp2;

	// 28th row
	// convolve with filter 1 time:
	// 		1. filter's 3rd row (when filter is sliding through the 27th row of input)
	inputAccessOffset += paddedWidth;
	inTemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[filterAccessBase + 6] * inTemp0;

	inTemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[filterAccessBase + 7] * inTemp1;

	inTemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[filterAccessBase + 8] * inTemp2;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;
}
