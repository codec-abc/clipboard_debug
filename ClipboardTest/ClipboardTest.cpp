#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <iomanip>
#include <sstream>


//int width = 2;
//int height = 2;
//int32_t bitmap2x2[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0x00000000 };

extern "C"
{
    size_t open_image(
        const char* path, 
        unsigned char** image_data,
        size_t* width,
        size_t* height
    );

    size_t encode_image_as_png(
        const char* path,
        unsigned char** image_data_as_png
    );
}


void rgba8_to_bgra8(unsigned char* input_image, size_t nb_pixel, unsigned char* output_image)
{
    for (size_t i = 0; i < nb_pixel * 4; i += 4)
    {
        unsigned char r = input_image[i + 0];
        unsigned char g = input_image[i + 1];
        unsigned char b = input_image[i + 2];
        unsigned char a = input_image[i + 3];

        output_image[i + 0] = b;
        output_image[i + 1] = g;
        output_image[i + 2] = r;
        output_image[i + 3] = a;

        //if (a == 255) {
        //    int r_int = r;
        //    int g_int = g;
        //    int b_int = b;
        //    std::cout << "r " << r_int << " g " << g_int << " b " << b_int << std::endl;
        //}
    }
}


int main()
{
    const char* path = "C:\\Users\\cviot\\Desktop\\Wifi.png";
    unsigned char* image = nullptr;
    size_t image_width;
    size_t image_height;
    size_t result = open_image(path, &image, &image_width, &image_height);

    //int32_t* image_as_int32 = (int32_t*) image;

    unsigned char* output_image = (unsigned char*) calloc(4 * image_width * image_height, sizeof(unsigned char));

    int32_t* output_image_as_int32 = (int32_t*)output_image;

    rgba8_to_bgra8(image, image_width * image_height, output_image);

    if (OpenClipboard(NULL))
    {
        unsigned char* image_as_png = nullptr;
        size_t image_png_encoded_size = encode_image_as_png(path, &image_as_png);

        if (image_png_encoded_size) {
            std::ofstream binFile("C:\\Users\\cviot\\Desktop\\WifiCopy.png", std::ios::out | std::ios::binary);
            binFile.write((const char*) image_as_png, image_png_encoded_size);
        }

        UINT png_format = RegisterClipboardFormatA("PNG");

        if (png_format) {
            HGLOBAL hmem = GlobalAlloc(
                GHND,
                image_png_encoded_size
            );

            unsigned char* global_mem_png_image = (unsigned char*) GlobalLock(hmem);
            memcpy(global_mem_png_image, image_as_png, image_png_encoded_size);
            GlobalUnlock(hmem);
            SetClipboardData(png_format, hmem);
        }
        

        HDC hdc = CreateCompatibleDC(NULL);

        HGLOBAL hmem = GlobalAlloc(
            GHND,
            sizeof(BITMAPV5HEADER) + image_width * image_height * 4
        );

        BITMAPV5HEADER* header = (BITMAPV5HEADER*)GlobalLock(hmem);
        header->bV5Size = sizeof(BITMAPV5HEADER);
        header->bV5Width = (long) image_width;
        header->bV5Height = -((long)image_height);
        header->bV5Planes = 1;
        header->bV5BitCount = 32;
        header->bV5Compression = BI_RGB;
        header->bV5SizeImage = 4 * image_width * image_height;
        header->bV5AlphaMask = 0xff000000;
        header->bV5RedMask = 0x00ff0000;
        header->bV5GreenMask = 0x0000ff00;
        header->bV5BlueMask = 0x000000ff;
        header->bV5CSType = LCS_WINDOWS_COLOR_SPACE;
        header->bV5Intent = LCS_GM_GRAPHICS;
        header->bV5ClrUsed = 0;
        header->bV5ClrImportant = 0;
        header->bV5ProfileData = 0;

        char* dst = (((char*)header) + header->bV5Size);
        //memcpy(dst, &bitmap2x2[0], 4 * sizeof(int32_t));
        //memcpy(dst, &image_as_int32[0], image_width * image_height * sizeof(int32_t));
        memcpy(dst, &output_image_as_int32[0], image_width * image_height * sizeof(int32_t));
        GlobalUnlock(hmem);

        EmptyClipboard();
        //SetClipboardData(CF_DIBV5, hmem);
        CloseClipboard();

        GlobalFree(hmem);
    }

    return 0;
}