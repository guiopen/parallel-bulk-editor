#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "img_editing.h"

// Bibliotecas
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
 * Função principal de transformação de imagem
 */
int transform_image(const char *input_path, const char *output_path, PixelTransformFunction transform)
{
    int width, height, channels;
    // Carrega a imagem do disco
    unsigned char *img = stbi_load(input_path, &width, &height, &channels, 3);
    if (!img)
        return 0;

    // Aplica a transformação pixel a pixel
    for (int i = 0; i < width * height; i++)
    {
        Pixel input_pixel = {img[i * 3], img[i * 3 + 1], img[i * 3 + 2]};
        Pixel output_pixel = transform(input_pixel);
        img[i * 3] = output_pixel.r;
        img[i * 3 + 1] = output_pixel.g;
        img[i * 3 + 2] = output_pixel.b;
    }

    // Salva a imagem transformada
    int success = stbi_write_jpg(output_path, width, height, 3, img, 100);
    stbi_image_free(img);
    return success;
}

// Converte um pixel para escala de cinza: Red 21%, Green 72%, Blue 7%
Pixel grayscale(Pixel pixel)
{
    uint8_t gray = (uint8_t)(0.21f * pixel.r + 0.72f * pixel.g + 0.07f * pixel.b);
    return (Pixel){gray, gray, gray};
}

// Mantém apenas o canal vermelho, zerando os outros
Pixel filter_red(Pixel pixel)
{
    return (Pixel){pixel.r, 0, 0};
}

// Mantém apenas o canal verde, zerando os outros
Pixel filter_green(Pixel pixel)
{
    return (Pixel){0, pixel.g, 0};
}

// Mantém apenas o canal azul, zerando os outros
Pixel filter_blue(Pixel pixel)
{
    return (Pixel){0, 0, pixel.b};
}

// Inverte as cores, subtraindo cada componente de 255
Pixel invert(Pixel pixel)
{
    return (Pixel){255 - pixel.r, 255 - pixel.g, 255 - pixel.b};
}