#include <jni.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <iostream>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bfType;       // File type ("BM")
    uint32_t bfSize;       // Size of the file
    uint16_t bfReserved1;  // Reserved
    uint16_t bfReserved2;  // Reserved
    uint32_t bfOffBits;    // Offset to the pixel data
};

struct BMPInfoHeader {
    uint32_t biSize;           // Size of this header
    int32_t biWidth;           // Image width
    int32_t biHeight;          // Image height
    uint16_t biPlanes;         // Number of color planes
    uint16_t biBitCount;       // Number of bits per pixel
    uint32_t biCompression;    // Compression type
    uint32_t biSizeImage;      // Size of image data
    int32_t biXPelsPerMeter;   // X pixels per meter
    int32_t biYPelsPerMeter;   // Y pixels per meter
    uint32_t biClrUsed;        // Number of colors
    uint32_t biClrImportant;   // Important colors
};
#pragma pack(pop)

struct BMPImage {
    BMPHeader header;
    BMPInfoHeader infoHeader;
    std::vector<uint8_t> pixels;
};

BMPImage readBMP(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Error opening BMP file" << std::endl;
        throw std::runtime_error("Error opening BMP file");
    }

    BMPImage image;
    file.read((char*)&image.header, sizeof(image.header));
    file.read((char*)&image.infoHeader, sizeof(image.infoHeader));

    image.pixels.resize(image.infoHeader.biSizeImage);
    file.seekg(image.header.bfOffBits, std::ios::beg);
    file.read((char*)image.pixels.data(), image.infoHeader.biSizeImage);

    file.close();
    return image;
}

void writeBMP(const BMPImage& image, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Error writing BMP file");
    }

    file.write((char*)&image.header, sizeof(image.header));
    file.write((char*)&image.infoHeader, sizeof(image.infoHeader));
    file.write((char*)image.pixels.data(), image.infoHeader.biSizeImage);

    file.close();
}

// Function to create a 640x480 BMP image with a gradient test pattern
BMPImage CreateBMP() {
    int width = 640;
    int height = 480;
    int channels = 3;  // RGB (3 bytes per pixel)

    BMPImage image;

    // Initialize BMPHeader
    image.header.bfType = 0x4D42;  // "BM"
    image.header.bfSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + (width * height * channels);
    image.header.bfReserved1 = 0;
    image.header.bfReserved2 = 0;
    image.header.bfOffBits = sizeof(BMPHeader) + sizeof(BMPInfoHeader);

    // Initialize BMPInfoHeader
    image.infoHeader.biSize = sizeof(BMPInfoHeader);
    image.infoHeader.biWidth = width;
    image.infoHeader.biHeight = height;
    image.infoHeader.biPlanes = 1;
    image.infoHeader.biBitCount = 24;  // 24 bits (3 bytes per pixel)
    image.infoHeader.biCompression = 0;
    image.infoHeader.biSizeImage = width * height * channels;
    image.infoHeader.biXPelsPerMeter = 0;
    image.infoHeader.biYPelsPerMeter = 0;
    image.infoHeader.biClrUsed = 0;
    image.infoHeader.biClrImportant = 0;

    // Create pixel data (simple gradient pattern)
    image.pixels.resize(width * height * channels);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;

            // Simple gradient pattern (R varies along x, G varies along y, B is constant)
            image.pixels[index + 0] = static_cast<uint8_t>((255 * x) / width);  // Red
            image.pixels[index + 1] = static_cast<uint8_t>((255 * y) / height); // Green
            image.pixels[index + 2] = 128;  // Blue constant
        }
    }

    return image;
}

void applyGaussianBlur(BMPImage& image) {
    // Gaussian kernel 3x3
    const float kernel[3][3] = {
            { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f },
            { 2 / 16.0f, 4 / 16.0f, 2 / 16.0f },
            { 1 / 16.0f, 2 / 16.0f, 1 / 16.0f }
    };

    int width = image.infoHeader.biWidth;
    int height = image.infoHeader.biHeight;
    int channels = image.infoHeader.biBitCount / 8;  // Usually 3 (RGB)

    std::vector<uint8_t> result = image.pixels;  // Create a copy of pixel data

    // Iterate over every pixel (ignoring the borders for simplicity)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < channels; c++) {
                float newValue = 0.0f;
                for (int ky = 0; ky < 3; ky++) {
                    for (int kx = 0; kx < 3; kx++) {
                        int pixelX = x + kx - 1;
                        int pixelY = y + ky - 1;
                        newValue += kernel[ky][kx] * image.pixels[(pixelY * width + pixelX) * channels + c];
                    }
                }
                result[(y * width + x) * channels + c] = static_cast<uint8_t>(newValue);
            }
        }
    }

    image.pixels = result;  // Assign the blurred pixels back to the image
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapp_MainActivity_processImage(JNIEnv *env, jobject, jstring inputPath, jstring outputPath) {
    const char* inputFile = env->GetStringUTFChars(inputPath, 0);
    const char* outputFile = env->GetStringUTFChars(outputPath, 0);

    try {
        BMPImage image = readBMP(inputFile);
        applyGaussianBlur(image);
        writeBMP(image, outputFile);
        } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        }

    env->ReleaseStringUTFChars(inputPath, inputFile);
    env->ReleaseStringUTFChars(outputPath, outputFile);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_cameraispapplication_MainActivity_processImage(JNIEnv *env, jobject thiz,
                                                                jstring input_path,
                                                                jstring output_path) {
    // TODO: implement processImage()
    const char* inputFile = env->GetStringUTFChars(input_path, 0);
    const char* outputFile = env->GetStringUTFChars(output_path, 0);

    try {
        BMPImage image = readBMP(inputFile);
        //BMPImage image = CreateBMP();
        //applyGaussianBlur(image);
        writeBMP(image, outputFile);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    env->ReleaseStringUTFChars(input_path, inputFile);
    env->ReleaseStringUTFChars(output_path, outputFile);
}