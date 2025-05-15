#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "file_utils.h"

int is_image_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext)
        return 0;
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0);
}

ImagePath *scan_directory(const char *input_dir, const char *output_dir, int *count)
{
    DIR *dir = opendir(input_dir);
    if (!dir)
        return NULL;

    struct dirent *entry;
    *count = 0;
    while ((entry = readdir(dir)))
    {
        if (entry->d_type == DT_REG && is_image_file(entry->d_name))
        {
            (*count)++;
        }
    }

    ImagePath *paths = malloc(*count * sizeof(ImagePath));
    if (!paths)
    {
        closedir(dir);
        return NULL;
    }

    rewinddir(dir);
    int i = 0;
    while ((entry = readdir(dir)) && i < *count)
    {
        if (entry->d_type == DT_REG && is_image_file(entry->d_name))
        {
            snprintf(paths[i].input_path, sizeof(paths[i].input_path),
                     "%s/%s", input_dir, entry->d_name);
            snprintf(paths[i].output_path, sizeof(paths[i].output_path),
                     "%s/%s", output_dir, entry->d_name);
            i++;
        }
    }

    closedir(dir);
    return paths;
}