#pragma once

#include <memory>
#include "Exception.h"

/**
 * @brief The ImageFormatEnum enum
 */
enum class ImageFormatEnum
{
    Unknown = 0,
    R8G8B8,
    R8G8B8A8
};


/**
 * @brief The Image class
 */
class Image
{
    ImageFormatEnum format;
    int width;
    int height;
    int stride;
    void* data;

public:
    Image(ImageFormatEnum format,
          int width,
          int height,
          int stride,
          void* data);

    virtual ~Image();

    int Width() const
    {
        return width;
    }
    int Height() const
    {
        return height;
    }
    int Stride() const
    {
        return stride;
    }
    ImageFormatEnum Format() const
    {
        return format;
    }
    void* Data() const
    {
        return data;
    }
};
typedef std::shared_ptr<Image> ImageSPTR;

/**
 * @brief The AllocatedImage class
 */
class AllocatedImage : public Image
{
    static int GetBytesPerPixel(ImageFormatEnum format);
    static int CalculateStride(int width, ImageFormatEnum format);
    static void* Allocate(int width, int height, ImageFormatEnum format);

public:
    AllocatedImage(ImageFormatEnum format, int width, int height);
    virtual ~AllocatedImage();
};
typedef std::shared_ptr<AllocatedImage> AllocatedImageSPTR;
