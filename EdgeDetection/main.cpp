#include <fstream>
#include <iostream>
#include <string>
#include <cmath>

#define BLUE 0
#define GREEN 1
#define RED 2
#define GRAYSCALE 0

#define PI 3.14159265

#define AVERAGE_SOBEL 0
#define AVERAGE_ABS_SOBEL 1
#define GEOM_AVERAGE_SOBEL 2
#define PROPER_SOBEL 3

using namespace std;

struct bmpFileInfo {
	char wholeInfo[54]{};
	int infoIndex{};
	char fileType[3];
	int fileSize;
	int reserved1, reserved2;
	int pixelDataOffset;
	int headerSize;
	int pixelWidth, pixelHeight;
	int planesNum, bitsPerPixel;
	int compressionAlgorithm;
	int imageSizeInBytes;
	int pixelsPerMeterX, pixelsPerMeterY;
	int colourUsedNum, importantColoursNum;
};

int convertBytesToInt(char info[], int bytesNum) {
	int result = 0;
	for (int i = 0; i < bytesNum; i++) {
		result += (int)(unsigned char)info[i] * (int)pow(256, i);
	}
	return result;
}

int readByte(ifstream& input) {
	char buffor[2];
	input.read(buffor, 1);
	int result = convertBytesToInt(buffor, 1);
	return result;
}

void readBytesForBMPInfo(ifstream& input, int& destination, int bytesNum, char* wholeInfo, int& infoIndex) {
	char buffor[8];
	input.read(buffor, bytesNum);
	destination = convertBytesToInt(buffor, bytesNum);
	for (int i = 0; i < bytesNum; i++) {
		wholeInfo[infoIndex + i] += buffor[i];
	}
	infoIndex += bytesNum;
}

void bmpFileInfoRead(ifstream& input, bmpFileInfo& bmpInfo) {
	int tmp;
	readBytesForBMPInfo(input, tmp, 2, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	bmpInfo.fileType[0] = bmpInfo.wholeInfo[0];
	bmpInfo.fileType[1] = bmpInfo.wholeInfo[1];
	bmpInfo.fileType[2] = '\00';

	readBytesForBMPInfo(input, bmpInfo.fileSize, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.reserved1, 2, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.reserved2, 2, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.pixelDataOffset, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);

	readBytesForBMPInfo(input, bmpInfo.headerSize, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.pixelWidth, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.pixelHeight, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.planesNum, 2, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.bitsPerPixel, 2, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.compressionAlgorithm, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.imageSizeInBytes, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.pixelsPerMeterX, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.pixelsPerMeterY, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.colourUsedNum, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
	readBytesForBMPInfo(input, bmpInfo.importantColoursNum, 4, bmpInfo.wholeInfo, bmpInfo.infoIndex);
}

struct pixel {
	int BGR[3]{};
};

void readImage(ifstream& input, int pixelOffset, int pixelHeight, int pixelWidth, pixel** originalImage) {
	int additionalBytes = pixelWidth % 4;
	input.seekg(pixelOffset);
	for (int y = pixelHeight; y >= 1; y--) {
		for (int x = 1; x <= pixelWidth; x++) {
			originalImage[y][x].BGR[BLUE] = readByte(input);
			originalImage[y][x].BGR[GREEN] = readByte(input);
			originalImage[y][x].BGR[RED] = readByte(input);
		}
		for (int i = 0; i < additionalBytes; i++) {
			readByte(input);
		}
	}
}

int masks[8][9] = {
	{-1, 0, 1, -2, 0, 2, -1, 0, 1},
	{0, 1, 2, -1, 0, 1, -2, -1, 0},
	{1, 2, 1, 0, 0, 0, -1, -2, -1},
	{2, 1, 0, 1, 0, -1, 0, -1, -2},
	{1, 0, -1, 2, 0, -2, 1, 0, -1},
	{0, -1, -2, 1, 0, -1, 2, 1, 0},
	{-1, -2, -1, 0, 0, 0, 1, 2, 1},
	{-2, -1, 0, -1, 0, 1, 0, 1, 2},
};

int mask(pixel** originalImage, int y, int x, int colour, int maskIndex) {
	int sum{};
	for (int dy = -1; dy < 2; dy++) {
		for (int dx = -1; dx < 2; dx++) {
			sum += originalImage[y + dy][x + dx].BGR[colour] * masks[maskIndex][(dy + 1) * 3 + (dx + 1) * 1];
		}
	}
	return sum;
}

int normalizePoint(int value) {
	if (value < 0)value = 0;
	if (value > 255)value = 255;
	return value;
}

int averageOfMasks(pixel** originalImage, int y, int x, int colour, int mode) {
	int sum{}, element;
	if (mode == AVERAGE_SOBEL) {
		for (int i = 0; i < 8; i++) {
			element = mask(originalImage, y, x, colour, i);
			sum += normalizePoint(element);
		}
		return sum / 8;
	}
	else if (mode == AVERAGE_ABS_SOBEL) {
		for (int i = 0; i < 8; i++) {
			element = (int)abs(mask(originalImage, y, x, colour, i));
			sum += element;
		}
		return sum / 8;
	}
	else if (mode == GEOM_AVERAGE_SOBEL) {
		for (int i = 0; i < 8; i++) {
			sum += (int)pow(mask(originalImage, y, x, colour, i), 2);
		}
		return (int)sqrt(sum);
	}
}

char convertToChar(int colourInput) {
	char output;
	output = (char)normalizePoint(colourInput);
	return output;
}

void colourSobel(pixel** originalImage, pixel** destinationImage, int imageSizeX, int imageSizeY, int mode) {
	for (int y = 1; y <= imageSizeY; y++) {
		for (int x = 1; x <= imageSizeX; x++) {
			destinationImage[y][x].BGR[BLUE] = averageOfMasks(originalImage, y, x, BLUE, mode);
			destinationImage[y][x].BGR[GREEN] = averageOfMasks(originalImage, y, x, GREEN, mode);
			destinationImage[y][x].BGR[RED] = averageOfMasks(originalImage, y, x, RED, mode);
		}
	}
}

void turnIntoGrayImage(pixel** originalImage, pixel** destinationImage, int imageSizeX, int imageSizeY) {
	for (int y = 1; y <= imageSizeY; y++) {
		for (int x = 1; x <= imageSizeX; x++) {
			destinationImage[y][x].BGR[GRAYSCALE] = 30 * originalImage[y][x].BGR[RED]
				+ 59 * originalImage[y][x].BGR[GREEN] + 11 * originalImage[y][x].BGR[BLUE];
			destinationImage[y][x].BGR[GRAYSCALE] /= 100;
		}
	}
}

pixel hsvToBGR(double hue, double saturation, double value) {
	pixel out;
	double C, X, m;
	double r, g, b;
	C = (value / 100.0f) * (saturation / 100.0f);
	X = C * (1.0f - abs(double(int(hue / 60.0f) % 2) - 1.0f));
	m = (value / 100.0f) * C;
	if (hue < 60.0f) {
		r = C;
		g = X;
		b = 0;
	}
	else if (hue < 120.0f) {
		r = X;
		g = C;
		b = 0;
	}
	else if (hue < 180.0f) {
		r = 0;
		g = C;
		b = X;
	}
	else if (hue < 240.0f) {
		r = 0;
		g = X;
		b = C;
	}
	else if (hue < 300.0f) {
		r = X;
		g = 0;
		b = C;
	}
	else {
		r = C;
		g = 0;
		b = X;
	}
	out.BGR[RED] = int(((r + m)) * 255.0f);
	out.BGR[GREEN] = int(((g + m)) * 255.0f);
	out.BGR[BLUE] = int(((b + m)) * 255.0f);
	return out;
}

void sobelOperator(pixel** originalImage, pixel** destinationImage, int imageSizeX, int imageSizeY) {
	int Gx, Gy;
	int teta;
	int grayValue;
	for (int y = 1; y <= imageSizeY; y++) {
		for (int x = 1; x <= imageSizeX; x++) {
			Gx = mask(originalImage, y, x, GRAYSCALE, 4);
			Gy = mask(originalImage, y, x, GRAYSCALE, 2);
			grayValue = (int)sqrt(pow(Gx, 2) + pow(Gy, 2));
			destinationImage[y][x].BGR[GRAYSCALE] = grayValue;
			teta = atan2((double)Gy, (double)Gx) * 180.0f / PI;
			destinationImage[y][x] = hsvToBGR(teta, 100, grayValue * 100 / 255);
		}
	}
}

void saveImage(bmpFileInfo& bmpInfo, pixel** newImage, string fileLocation) {
	fileLocation.erase(fileLocation.end() - 4, fileLocation.end());
	fileLocation += "_out.bmp";
	ofstream output;
	output.open(fileLocation, ofstream::binary);
	output.write(bmpInfo.wholeInfo, 54);
	char bgr[3];
	int additionalBytes = bmpInfo.pixelWidth % 4;
	for (int y = bmpInfo.pixelHeight; y >= 1; y--) {
		for (int x = 1; x <= bmpInfo.pixelWidth; x++) {
			bgr[BLUE] = convertToChar(newImage[y][x].BGR[BLUE]);
			bgr[GREEN] = convertToChar(newImage[y][x].BGR[GREEN]);
			bgr[RED] = convertToChar(newImage[y][x].BGR[RED]);
			output.write(bgr, 3);
		}
		bgr[BLUE] = 0;
		bgr[GREEN] = 0;
		bgr[RED] = 0;
		if (additionalBytes > 0) {
			output.write(bgr, additionalBytes);
		}
	}
	output.close();
}

int main() {
	ifstream input;
	ofstream output;
	string fileLocation;
	cout << "Pass bmp file location(like: TEST/TEST.bmp): ";
	cin >> fileLocation;

	input.open(fileLocation, ifstream::binary);

	if (!input) {
		cout << "File does not exist, write the file location again: ";
		cin >> fileLocation;
		input.open(fileLocation, ifstream::binary);
	}

	bmpFileInfo bmpInfo;
	bmpFileInfoRead(input, bmpInfo);

	pixel** originalImage = new pixel * [bmpInfo.pixelHeight + 2];
	for (int i = 0; i < bmpInfo.pixelHeight + 2; i++) {
		originalImage[i] = new pixel[bmpInfo.pixelWidth + 2];
	}
	readImage(input, bmpInfo.pixelDataOffset, bmpInfo.pixelHeight, bmpInfo.pixelWidth, originalImage);

	pixel** newImage = new pixel * [bmpInfo.pixelHeight + 2];
	for (int i = 0; i < bmpInfo.pixelHeight + 2; i++) {
		newImage[i] = new pixel[bmpInfo.pixelWidth + 2];
	}
	int mode;
	cout << "Mode:\n0 - average sobel\n1 - average abs sober\n2 - geometric average sober\n3 - sober with edge direction\nPass mode:";
	cin >> mode;
	if (mode < 3 && mode >-1) {
		colourSobel(originalImage, newImage, bmpInfo.pixelWidth, bmpInfo.pixelHeight, mode);
	}
	else if (mode == 3) {
		turnIntoGrayImage(originalImage, newImage, bmpInfo.pixelWidth, bmpInfo.pixelHeight);
		sobelOperator(originalImage, newImage, bmpInfo.pixelWidth, bmpInfo.pixelHeight);
	}
	saveImage(bmpInfo, newImage, fileLocation);
	for (int i = 0; i < bmpInfo.pixelHeight + 2; i++) {
		delete[]originalImage[i];
		delete[]newImage[i];
	}
	delete[]originalImage;
	delete[]newImage;

	return 0;
}