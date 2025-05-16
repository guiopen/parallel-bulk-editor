#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <dirent.h>
#include <sys/stat.h>
#include "img_editing.h"

int is_image_file(const char *filename);
ImagePath *scan_directory(const char *input_dir, const char *output_dir, int *count);

#endif