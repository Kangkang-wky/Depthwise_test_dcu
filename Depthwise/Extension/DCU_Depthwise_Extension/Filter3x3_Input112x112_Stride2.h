/*
Depthwise Convolution Kernel.

Case: filter 3 x 3, input 112 x 112, stride 2, padding 1
2 blocks process 1 channel.

Used in the MobileNet V2 and EfficientNet B0, in case of
	1)	112 x 112 x 96 -> 56 x 56 x 96, stride = 2, filter = 3
*/
template <typename scalar_t>
__global__ void Filter3x3_Input112x112_Stride2(const scalar_t* __restrict__ input, const scalar_t* __restrict__ filter, scalar_t* __restrict__ output,
	int inputBatchNumber, int inputChannel, int inputHeight, int inputWidth,
	int filterLayerNumber, int filterHeight, int filterWidth,
	int outputBatchNumber, int outputChannel, int outputHeight, int outputWidth,
	int padding, int stride,
	float alpha, float beta) {

	// filter is 3 x 3. 9 elements in total
	__shared__ float filterData[9];
	// 2 blocks handle one 112 x 112 input. Each block handles 56 rows. With padding, 58 rows, each row has 114 elements
	__shared__ float inputData[59 * 114];

	float intemp0, intemp1, intemp2;
	float sum0, sum1;

	int paddedWidth = inputWidth + 2 * padding;
	int blockGroup = 2;

	// load filter
	int filterLoadSrcIdx = blockIdx.y / blockGroup * filterHeight * filterWidth + threadIdx.x;
	if (threadIdx.x < filterWidth * filterHeight) {
		filterData[threadIdx.x] = filter[filterLoadSrcIdx];
	}

	int leftPaddingIdx = 0;
	// set padding
	if (threadIdx.x >= 32 && threadIdx.x < 90) {
		leftPaddingIdx = (threadIdx.x - 32) * paddedWidth;
		inputData[leftPaddingIdx] = 0;						// left padding
		inputData[leftPaddingIdx + paddedWidth - 1] = 0;	// right padding
	}
	if (threadIdx.x >= 112) {
		inputData[threadIdx.x - 111] = 0;					// Top padding
		inputData[threadIdx.x - 111 + 57 * paddedWidth] = 0;// Bottom padding
	}
	__syncthreads();

	int inputLoadIdxBase = blockIdx.x * inputHeight * inputWidth * inputChannel +
		blockIdx.y / blockGroup * inputHeight * inputWidth +
		(blockIdx.y & 1) * inputHeight / blockGroup * inputWidth;

	// block 0 needs to process 56 rows + bottom 1 row, no upper padding.
	// block 1 needs to process 56 rows + upper 1 row + bottom 1 row
	int inputLoadSrcIdx = inputLoadIdxBase + threadIdx.x - inputWidth;
	int inputLoadDstIdx = (threadIdx.x / inputWidth) * 2 + threadIdx.x + 1;
	if ((blockIdx.y & 1) == 0) {
		inputLoadSrcIdx += inputWidth;
		inputLoadDstIdx += paddedWidth;
	}

	// each block load 56 rows, and each time load 2 rows, so 28 times
	#pragma unroll
	for (int i = 0; i < 28; i++) {
		inputData[inputLoadDstIdx + 2 * 114 * i] = input[inputLoadSrcIdx + 2 * 112 * i];
	}
	// block1 do not need to load extra 1 bottom row. 
	if ((blockIdx.y & 1) != 1) {
		inputData[inputLoadDstIdx + 2 * 114 * 28] = input[inputLoadSrcIdx + 2 * 112 * 28];
	}
	else {
		if (threadIdx.x < 112) {
			inputData[inputLoadDstIdx + 2 * 114 * 28] = input[inputLoadSrcIdx + 2 * 112 * 28];
		}
	}
	__syncthreads();

	// for a 224-thread block, every 56-thread group processes 14 rows in the input, and write 7 rows in the output
	int outputIdx = blockIdx.x * outputHeight * outputWidth * outputChannel +
		(blockIdx.y / blockGroup) * outputHeight * outputWidth +
		(blockIdx.y & 1) * (outputHeight / blockGroup) * outputWidth +
		(threadIdx.x / outputWidth) * 7 * outputWidth + 
		threadIdx.x % outputWidth;

	int inputAccessBase = (threadIdx.x / outputWidth) * 14 * paddedWidth + threadIdx.x % outputWidth * 2;
	int inputAccessOffset = 0;

	// row 0
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 =	filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[2] * intemp2;

	// row 1
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[5] * intemp2;

	// row 2
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[6] * intemp0;
	sum1 = filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[7] * intemp1;
	sum1 = sum1 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[8] * intemp2;
	sum1 = sum1 + filterData[2] * intemp2;

	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

	// row 3
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[5] * intemp2;

	// row 4
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = filterData[0] * intemp0;
	sum1 = sum1 + filterData[6] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[1] * intemp1;
	sum1 = sum1 + filterData[7] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[2] * intemp2;
	sum1 = sum1 + filterData[8] * intemp2;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;

	// row 5
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[5] * intemp2;

	// row 6
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[6] * intemp0;
	sum1 = filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[7] * intemp1;
	sum1 = sum1 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[8] * intemp2;
	sum1 = sum1 + filterData[2] * intemp2;

	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

	// row 7
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[5] * intemp2;

	// row 8
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[6] * intemp0;
	sum0 = filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[7] * intemp1;
	sum0 = sum0 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[8] * intemp2;
	sum0 = sum0 + filterData[2] * intemp2;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;

	// row 9
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[5] * intemp2;

	// row 10
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[6] * intemp0;
	sum1 = filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[7] * intemp1;
	sum1 = sum1 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[8] * intemp2;
	sum1 = sum1 + filterData[2] * intemp2;

	output[outputIdx] = sum0 * alpha + beta;
	outputIdx += outputWidth;

	// row 11
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[5] * intemp2;

	// row 12
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum1 = sum1 + filterData[6] * intemp0;
	sum0 = filterData[0] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum1 = sum1 + filterData[7] * intemp1;
	sum0 = sum0 + filterData[1] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum1 = sum1 + filterData[8] * intemp2;
	sum0 = sum0 + filterData[2] * intemp2;

	output[outputIdx] = sum1 * alpha + beta;
	outputIdx += outputWidth;

	// row 13
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[3] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[4] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[5] * intemp2;

	// row 14
	inputAccessOffset += paddedWidth;
	intemp0 = inputData[inputAccessBase + inputAccessOffset];
	sum0 = sum0 + filterData[6] * intemp0;
	intemp1 = inputData[inputAccessBase + inputAccessOffset + 1];
	sum0 = sum0 + filterData[7] * intemp1;
	intemp2 = inputData[inputAccessBase + inputAccessOffset + 2];
	sum0 = sum0 + filterData[8] * intemp2;

	output[outputIdx] = sum0 * alpha + beta;
}
