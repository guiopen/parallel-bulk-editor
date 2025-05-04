#ifndef IMG_EDITING_H
#define IMG_EDITING_H

#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef Pixel (*PixelTransformFunction)(Pixel);

int transform_image(const char *input_path, const char *output_path, PixelTransformFunction transform);
Pixel grayscale(Pixel pixel);
Pixel filter_red(Pixel pixel);
Pixel filter_green(Pixel pixel);
Pixel filter_blue(Pixel pixel);
Pixel invert(Pixel pixel);

#endif