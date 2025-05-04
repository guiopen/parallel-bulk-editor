#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "img_editing.h"

void print_usage()
{
    printf("Usage: ./editor <input_folder> <edit_type>\n"
           "Edit types: grayscale, red, green, blue, invert\n");
}

PixelTransformFunction get_transform_function(const char *edit_type)
{
    if (strcmp(edit_type, "grayscale") == 0)
        return grayscale;
    if (strcmp(edit_type, "red") == 0)
        return filter_red;
    if (strcmp(edit_type, "green") == 0)
        return filter_green;
    if (strcmp(edit_type, "blue") == 0)
        return filter_blue;
    if (strcmp(edit_type, "invert") == 0)
        return invert;
    return NULL;
}

int is_image_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext)
        return 0;
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0);
}

int process_directory(const char *input_dir, const char *output_dir, PixelTransformFunction transform)
{
    DIR *dir = opendir(input_dir);
    if (!dir)
        return 0;

    struct dirent *entry;
    int processed = 0;
    char input_path[512], output_path[512];

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && is_image_file(entry->d_name))
        {
            snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, entry->d_name);
            snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, entry->d_name);
            if (transform_image(input_path, output_path, transform))
                processed++;
        }
    }

    closedir(dir);
    return processed;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        print_usage();
        return 1;
    }

    const char *input_dir = argv[1];
    const char *edit_type = argv[2];
    char output_dir[256];

    PixelTransformFunction transform = get_transform_function(edit_type);
    if (!transform)
    {
        print_usage();
        return 1;
    }

    snprintf(output_dir, sizeof(output_dir), "%s_%s", input_dir, edit_type);
    if (mkdir(output_dir, 0777) != 0)
    {
        printf("Error creating output directory\n");
        return 1;
    }

    int processed = process_directory(input_dir, output_dir, transform);
    printf("Processed %d images\n", processed);

    return 0;
}