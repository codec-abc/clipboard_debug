#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <iomanip>
#include <sstream>


//int width = 2;
//int height = 2;
//int32 bitmap2x2[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0x00000000 };

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


void rgba8o_bgra8(unsigned char* input_image, size_t nb_pixel, unsigned char* output_image)
{
    for (size_t i = 0; i < nb_pixel * 4; i += 4)
    {
        unsigned char r = input_image[i + 0];
        unsigned char g = input_image[i + 1];
        unsigned char b = input_image[i + 2];
        unsigned char a = input_image[i + 3];

        output_image[i + 0] = (unsigned char)((float) b * (a / 255.0f));
        output_image[i + 1] = (unsigned char)((float) g * (a / 255.0f));
        output_image[i + 2] = (unsigned char)((float) r * (a / 255.0f));
        output_image[i + 3] = a;
        
        //if (a == 255) {
        //    int r_int = r;
        //    int g_int = g;
        //    int b_int = b;
        //    std::cout << "r " << r_int << " g " << g_int << " b " << b_int << std::endl;
        //}
    }
}


int main_1(int argc, char* argv[])
{
    const char* path = "C:\\Users\\cviot\\Desktop\\Wifi.png";
    unsigned char* image = nullptr;
    size_t image_width;
    size_t image_height;
    size_t result = open_image(path, &image, &image_width, &image_height);

    //int32* image_as_int32 = (int32*) image;

    unsigned char* output_image = (unsigned char*) calloc(4 * image_width * image_height, sizeof(unsigned char));

    int32_t* output_image_as_int32 = (int32_t*)output_image;

    rgba8o_bgra8(image, image_width * image_height, output_image);

    if (OpenClipboard(NULL))
    {
        unsigned char* image_as_png = nullptr;
        size_t image_png_encoded_size = encode_image_as_png(path, &image_as_png);

        if (image_png_encoded_size) {
            std::ofstream binFile("C:\\Users\\cviot\\Desktop\\WifiCopy.png", std::ios::out | std::ios::binary);
            binFile.write((const char*) image_as_png, image_png_encoded_size);
        }

        UINT png_format = RegisterClipboardFormatA("PNG");

       /* if (png_format) {
            HGLOBAL hmem = GlobalAlloc(
                GHND,
                image_png_encoded_size
            );

            unsigned char* global_mem_png_image = (unsigned char*) GlobalLock(hmem);
            memcpy(global_mem_png_image, image_as_png, image_png_encoded_size);
            GlobalUnlock(hmem);
            SetClipboardData(png_format, hmem);
        }*/
        

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
        //memcpy(dst, &bitmap2x2[0], 4 * sizeof(int32));
        //memcpy(dst, &image_as_int32[0], image_width * image_height * sizeof(int32));
        memcpy(dst, &output_image_as_int32[0], image_width * image_height * sizeof(int32_t));
        GlobalUnlock(hmem);

        EmptyClipboard();
        SetClipboardData(CF_DIBV5, hmem);
        CloseClipboard();

        GlobalFree(hmem);
    }

    return 0;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

const char* cfNames[] = {
    "CFEXT",
    "CF_BITMAP",
    "CF_METAFILEPICT",
    "CF_SYLK",
    "CF_DIF",
    "CFIFF",
    "CF_OEMTEXT",
    "CF_DIB",
    "CF_PALETTE",
    "CF_PENDATA",
    "CF_RIFF",
    "CF_WAVE",
    "CF_UNICODETEXT",
    "CF_ENHMETAFILE",
    "CF_HDROP",
    "CF_LOCALE",
    "CF_DIBV5"
};

int LookupFormat(const char* name)
{
    for (int i = 0; i != ARRAY_SIZE(cfNames); ++i)
    {
        if (strcmp(cfNames[i], name) == 0)
            return i + 1;
    }

    return RegisterClipboardFormatA(name);
}

void PrintFormatName(int format)
{
    if (!format)
        return;

    if ((format > 0) && (format <= ARRAY_SIZE(cfNames)))
    {
        printf(("%s\n"), cfNames[format - 1]);
    }
    else
    {
        char buffer[100];

        if (GetClipboardFormatNameA(format, buffer, ARRAY_SIZE(buffer)))
            printf(("%s\n"), buffer);
        else
            printf(("#%i\n"), format);
    }
}

void WriteFormats()
{
    int count = 0;
    int format = 0;
    do
    {
        format = EnumClipboardFormats(format);
        if (format)
        {
            ++count;
            PrintFormatName(format);
        }
    } while (format != 0);

    if (!count)
        printf(("Clipboard is empty!\n"));
}

void SaveFormat(int format, const char* filename)
{
    HGLOBAL hData = (HGLOBAL)GetClipboardData(format);

    LPVOID data = GlobalLock(hData);

    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        WriteFile(hFile, data, GlobalSize(hData), &bytesWritten, 0);
        CloseHandle(hFile);
    }

    GlobalUnlock(hData);
}

int main_2(int argc, char* argv[])
{
    if (!OpenClipboard(0))
    {
        printf("Cannot open clipboard\n");
        return 1;
    }

    if (argc == 1)
    {
        WriteFormats();
    }
    else if (argc == 3)
    {
        int format = LookupFormat(argv[1]);
        if (format == 0)
        {
            printf("Unknown format\n");
            return 1;
        }

        SaveFormat(format, argv[2]);
    }
    else
    {
        printf(("lscf\n"));
        printf(("List available clipboard formats\n\n"));
        printf(("lscf CF_NAME filename\n"));
        printf(("Write format CF_NAME to file filename\n\n"));
    }

    CloseClipboard();

    return 0;
}

int main(int argc, char* argv[])
{
    main_2(argc, argv);
}