#include "Image.h"

#define LOG_ACTIVATED
#define LOG_SRC AMLIMAGE
#include "KvalLogging.h"


/**
 * @brief Image::Image
 * @param format
 * @param width
 * @param height
 * @param stride
 * @param data
 */
Image::Image(ImageFormatEnum format,
             int width,
             int height,
             int stride,
             void* data)
    : format(format),
      width(width),
      height(height),
      stride(stride),
      data(data)
{
    INV("format: %u, width: %d, height: %d, stride: %d, data: %p",
        format, width, height, stride, data);

    if (width < 1)
    {
        LOG_ERROR(LOG_TAG, "width < 1");
        OUTV();
        throw ArgumentOutOfRangeException();
    }
    if (height < 1)
    {
        LOG_ERROR(LOG_TAG, "height < 1");
        OUTV();
        throw ArgumentOutOfRangeException();
    }
    if (stride < 1)
    {
        LOG_ERROR(LOG_TAG, "stride < 1");
        OUTV();
        throw ArgumentOutOfRangeException();
    }
    if (data == nullptr)
    {
        LOG_ERROR(LOG_TAG, "data NULL");
        OUTV();
        throw ArgumentNullException();
    }
    OUTV();
}

/**
 * @brief Image::~Image
 */
Image::~Image()
{
    INV();
    LOG_INFO(LOG_TAG, "Destroy Image obj");
    OUTV();
}

/**
 * @brief AllocatedImage::GetBytesPerPixel
 * @param format
 * @return
 */
int AllocatedImage::GetBytesPerPixel(ImageFormatEnum format)
{
    INV("format: %u", format);
    int bytesPerPixel;
    switch (format)
    {
        case ImageFormatEnum::R8G8B8:
            bytesPerPixel = 3;
            break;

        case ImageFormatEnum::R8G8B8A8:
            bytesPerPixel = 4;
            break;

        default:
            LOG_ERROR(LOG_TAG, "Not supported format");
            OUTV();
            throw NotSupportedException();
    }
    OUTV("bytesPerPixel: %d", bytesPerPixel);
    return bytesPerPixel;
}

/**
 * @brief AllocatedImage::CalculateStride
 * @param width
 * @param format
 * @return
 */
int AllocatedImage::CalculateStride(int width, ImageFormatEnum format)
{
    INV("width: %d, format: %u", width, format);
    int strides = width * GetBytesPerPixel(format);
    OUTV("strides: %d", strides);
    return strides;
}

/**
 * @brief AllocatedImage::Allocate
 * @param width
 * @param height
 * @param format
 * @return
 */
void* AllocatedImage::Allocate(int width, int height, ImageFormatEnum format)
{
    INV("width: %d, height: %d, format: %u", width, height, format);
    int stride = CalculateStride(width, format);
    void * data = malloc(stride * height);
    OUTV("data: %p", data);
    return data;
}

/**
 * @brief AllocatedImage::AllocatedImage
 * @param format
 * @param width
 * @param height
 */
AllocatedImage::AllocatedImage(ImageFormatEnum format, int width, int height)
    : Image(format,
            width,
            height,
            CalculateStride(width, format),
            Allocate(width, height, format))
{
    INV("width: %d, height: %d, format: %u", width, height, format);
    OUTV();
}

/**
 * @brief AllocatedImage::~AllocatedImage
 */
AllocatedImage::~AllocatedImage()
{
    INV();
    free(Data());
    OUTV();
}
