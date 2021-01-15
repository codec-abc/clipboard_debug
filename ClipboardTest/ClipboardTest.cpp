#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <iomanip>
#include <vector>
#include <sstream>
#include <shlwapi.h>
#include <wincodec.h>

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)


//int width = 2;
//int height = 2;
//int32 bitmap2x2[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0x00000000 };

extern "C"
{
    size_t open_image
    (
        const char* path, 
        unsigned char** image_data,
        size_t* width,
        size_t* height
    );

    size_t encode_image_as_png
    (
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

        output_image[i + 0] = (unsigned char)((float) b * (a / 255.0f));
        output_image[i + 1] = (unsigned char)((float) g * (a / 255.0f));
        output_image[i + 2] = (unsigned char)((float) r * (a / 255.0f));
        output_image[i + 3] = a;
    }
}


int main_1(int argc, char* argv[])
{
    const char* path = "C:\\Users\\cviot\\Desktop\\Wifi.png";
    unsigned char* image = nullptr;
    size_t image_width;
    size_t image_height;
    size_t result = open_image(path, &image, &image_width, &image_height);

    bool use_png = true;
    bool use_dibv5 = true;

    //int32* image_as_int32 = (int32*) image;

    unsigned char* output_image = (unsigned char*) calloc(4 * image_width * image_height, sizeof(unsigned char));
    int32_t* output_image_as_int32 = (int32_t*)output_image;
    rgba8_to_bgra8(image, image_width * image_height, output_image);

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();

        if (use_png) 
        {
            unsigned char* image_as_png = nullptr;
            size_t image_png_encoded_size = encode_image_as_png(path, &image_as_png);

            if (image_png_encoded_size)
            {
                std::ofstream binFile("C:\\Users\\cviot\\Desktop\\WifiCopy.png", std::ios::out | std::ios::binary);
                binFile.write((const char*)image_as_png, image_png_encoded_size);
            }

            UINT png_format = RegisterClipboardFormatA("PNG");

            if (png_format) {
                HGLOBAL hmem = GlobalAlloc(
                    GHND,
                    image_png_encoded_size
                );

                unsigned char* global_mem_png_image = (unsigned char*)GlobalLock(hmem);
                memcpy(global_mem_png_image, image_as_png, image_png_encoded_size);
                GlobalUnlock(hmem);

                if (SetClipboardData(png_format, hmem))
                {
                    std::cout << "set clipboard with png suceeded" << std::endl;
                }
                else
                {
                    std::cout << "Cannot set png clipboard" << std::endl;
                }

            }
        }
        
        if (use_dibv5)
        {

            HDC hdc = CreateCompatibleDC(NULL);

            HGLOBAL hmem = GlobalAlloc(
                GHND,
                sizeof(BITMAPV5HEADER) + image_width * image_height * 4
            );

            BITMAPV5HEADER* header = (BITMAPV5HEADER*)GlobalLock(hmem);
            header->bV5Size = sizeof(BITMAPV5HEADER);
            header->bV5Width = (long)image_width;
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
            memcpy(dst, &output_image_as_int32[0], image_width * image_height * sizeof(int32_t));
            GlobalUnlock(hmem);
            SetClipboardData(CF_DIBV5, hmem);
            CloseClipboard();

            GlobalFree(hmem);
        }
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
        {
            return i + 1;
        }
    }

    return RegisterClipboardFormatA(name);
}

void PrintFormatName(int format)
{
    if (!format) 
    {
        std::cout << "invalid format" << std::endl;
        return;
    }

    if ((format > 0) && (format <= ARRAY_SIZE(cfNames))) 
    {
        printf(("normal formats: %s\n"), cfNames[format - 1]);
    }
    else 
    {
        char buffer[100];

        if (GetClipboardFormatNameA(format, buffer, ARRAY_SIZE(buffer))) 
        {
            printf(("custom format: %s\n"), buffer);
            HANDLE data = GetClipboardData(format);
            if (data) 
            {
                std::vector<unsigned char> html_string;
                unsigned char* html_memory_content = (unsigned char*)data;
                while (*html_memory_content != 0)
                {
                    html_string.push_back(*html_memory_content);
                    html_memory_content++;
                }
                
                std::string s(html_string.begin(), html_string.end());
                std::cout << s << std::endl;

                int debug = 1;
            }
        }
        else 
        {
            printf(("custom format: #%i\n"), format);
        }
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
    {
        printf(("Clipboard is empty!\n"));
    }
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

const char* html_test_str = 
"Version:0.9\r\n"
"StartHTML:0000000105\r\n"
"EndHTML : 0000000285\r\n"
"StartFragment : 0000000141\r\n"
"EndFragment : 0000000249\r\n"
"<html>\r\n"
"<body>\r\n"
"<!--StartFragment-->\r\n"
//
"<!--EndFragment-->\r\n"
"</body>\r\n"
"</html>\r\n\0";

int main_3(int argc, char* argv[])
{
    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        UINT html_format = RegisterClipboardFormatA("HTML Format");

        char *buf = new char[40000];

        strcpy(buf,
            "Version:0.9\r\n"
            "StartHTML:00000000\r\n"
            "EndHTML:00000000\r\n"
            "StartFragment:00000000\r\n"
            "EndFragment:00000000\r\n"
            "<html><body>\r\n"
            "<!--StartFragment -->\r\n");

        // Append the HTML...
        strcat(buf, 
            "<img src=\"https://upload.wikimedia.org/wikipedia/commons/thumb/4/44/WIFI_icon.svg/200px-WIFI_icon.svg.png\" />\r\n"
            "<p>Hello world</p>");

        strcat(buf, "\r\n");
        // Finish up the HTML format...
        strcat(buf,
            "<!--EndFragment-->\r\n"
            "</body>\r\n"
            "</html>");

        // Now go back, calculate all the lengths, and write out the
        // necessary header information. Note, wsprintf() truncates the
        // string when you overwrite it so you follow up with code to replace
        // the 0 appended at the end with a '\r'...
        char *ptr = strstr(buf, "StartHTML");
        sprintf(ptr + 10, "%08u", strstr(buf, "<html>") - buf);
        *(ptr + 10 + 8) = '\r';

        ptr = strstr(buf, "EndHTML");
        sprintf(ptr + 8, "%08u", strlen(buf));
        *(ptr + 8 + 8) = '\r';

        ptr = strstr(buf, "StartFragment");
        sprintf(ptr + 14, "%08u", strstr(buf, "<!--StartFrag") - buf);
        *(ptr + 14 + 8) = '\r';

        ptr = strstr(buf, "EndFragment");
        sprintf(ptr + 12, "%08u", strstr(buf, "<!--EndFrag") - buf);
        *(ptr + 12 + 8) = '\r';

         if (html_format) 
         {
             HGLOBAL hText = GlobalAlloc(
                 GMEM_MOVEABLE | GMEM_DDESHARE,
                 strlen(buf) + 4
             );

             char *ptr = (char *)GlobalLock(hText);
             strcpy(ptr, buf);
             GlobalUnlock(hText);
             SetClipboardData(html_format, hText);

             CloseClipboard();
             GlobalFree(hText);
        }
    }

    return 0;
}

template<class T>
class comptr 
{
public:

    comptr() : m_ptr(nullptr) 
    {

    }

    explicit comptr(T* ptr) : m_ptr(ptr) 
    {

    }

    comptr(const comptr&) = delete;

    comptr& operator=(const comptr&) = delete;

    ~comptr() 
    { 
        reset(); 
    }

    T** operator&() 
    { 
        return &m_ptr; 
    }

    T* operator->() 
    { 
        return m_ptr; 
    }

    bool operator!() const 
    { 
        return !m_ptr; 
    }

    T* get() 
    { 
        return m_ptr; 
    }

    void reset() 
    {
        if (m_ptr) 
        {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }
private:
    T* m_ptr;
};

bool write_png_on_stream(
    IStream* stream,
    uint8_t* image_data,
    int width,
    int height,
    int bytes_per_row
)
{
    comptr<IWICBitmapEncoder> encoder;

    HRESULT hr = 
        CoCreateInstance(
            CLSID_WICPngEncoder,
            nullptr, 
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&encoder)
        );

    if (FAILED(hr)) 
    {
        return false;
    }

    hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);

    if (FAILED(hr)) 
    {
        return false;
    }

    comptr<IWICBitmapFrameEncode> frame;
    comptr<IPropertyBag2> options;

    hr = encoder->CreateNewFrame(&frame, &options);

    if (FAILED(hr)) 
    {
        return false;
    }

    hr = frame->Initialize(options.get());

    if (FAILED(hr))
    {
        return false;
    }

    // PNG encoder (and decoder) only supports GUID_WICPixelFormat32bppBGRA for 32bpp.
    // See: https://docs.microsoft.com/en-us/windows/win32/wic/-wic-codec-native-pixel-formats#png-native-codec

    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;

    hr = frame->SetPixelFormat(&pixelFormat);
    if (FAILED(hr)) 
    {
        return false;
    }

    hr = frame->SetSize(width, height);

    if (FAILED(hr)) 
    {
        return false;
    }

    std::vector<uint32_t> buf;
    uint8_t* ptr = image_data;

    hr = frame->WritePixels(
        height,
        bytes_per_row,
        bytes_per_row * height,
        (BYTE*)ptr
    );

    if (FAILED(hr)) 
    {
        return false;
    }

    hr = frame->Commit();

    if (FAILED(hr)) 
    {
        return false;
    }

    hr = encoder->Commit();

    if (FAILED(hr)) 
    {
        return false;
    }

    return true;
}

struct coinit 
{
    HRESULT hr;

    coinit() 
    {
        hr = CoInitialize(nullptr);
    }

    ~coinit() 
    {
        if (hr == S_OK || hr == S_FALSE) 
        {
            CoUninitialize();
        }
    }
};

LARGE_INTEGER intToLargeInt(int i) 
{
    LARGE_INTEGER li;
    li.QuadPart = i;
    return li;
}

HGLOBAL write_png_inner
(
    uint8_t* image_data,
    int width,
    int height,
    int bytes_per_row
) 
{
    coinit com;

    comptr<IStream> stream;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, false, &stream);
    if (FAILED(hr)) 
    {
        return nullptr;
    }

    bool result = write_png_on_stream(stream.get(), image_data , width, height, bytes_per_row);

    HGLOBAL handle;
    IStream* stream_ptr = stream.get();
    int nb_bytes_to_read = 10193;
    unsigned char* ptr = (unsigned char*) calloc(nb_bytes_to_read, sizeof(unsigned char));
    ULONG bytes_read;
    ULARGE_INTEGER new_ptr_after_seek;
    stream_ptr->Seek(intToLargeInt(0), STREAM_SEEK_SET, &new_ptr_after_seek);
    stream_ptr->Read(ptr, nb_bytes_to_read, &bytes_read);

    std::ofstream binFile("C:\\Users\\cviot\\Desktop\\WifiCopy2.png", std::ios::out | std::ios::binary);
    binFile.write((const char*)ptr, nb_bytes_to_read);


    hr = GetHGlobalFromStream(stream_ptr, &handle);
    if (result) 
    {
        return handle;
    }

    GlobalFree(handle);
    return nullptr;
}

HGLOBAL write_png()
{
    const char* path = "C:\\Users\\cviot\\Desktop\\Wifi.png";
    unsigned char* image = nullptr;
    size_t image_width;
    size_t image_height;
    size_t result = open_image(path, &image, &image_width, &image_height);

    unsigned char* output_image = (unsigned char*)calloc(4 * image_width * image_height, sizeof(unsigned char));
    int32_t* output_image_as_int32 = (int32_t*)output_image;
    rgba8_to_bgra8(image, image_width * image_height, output_image);

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        int bytes_per_row = 4 * image_width;
        return write_png_inner(output_image, image_width, image_height, bytes_per_row);
    }

    return 0;
}

int main_4(int argc, char* argv[])
{

    UINT png_format = RegisterClipboardFormatA("PNG");
    if (png_format) 
    {
        HGLOBAL png_handle = write_png();
        if (png_handle)
        {
            if (SetClipboardData(png_format, png_handle))
            {
                std::cout << "set clipboard with png suceeded" << std::endl;
            }
            else
            {
                std::cout << "Cannot set png clipboard" << std::endl;
            }
        }
    }


    return 0;
}


int main(int argc, char* argv[])
{
    main_1(argc, argv);
}