//
//  pictureframe_viewer.c
//  pictureframe
//
//  Created by Colin Luoma on 2017-03-18
//  Copyright (c) 2017 Colin Luoma. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <libexif/exif-data.h>

#include "pictureframe.h"
#include "db_interface.h"
#include "base64.h"

void print_view_page(char *base64img, int orientation, char *mime_type,
                     char *message, char *name)
{
    int degrees;
    switch (orientation)
    {
        case 1:
            degrees = 0;
            break;
        case 3:
            degrees = 180;
            break;
        case 6:
            degrees = 90;
            break;
        case 8:
            degrees = 270;
            break;
        default:
            degrees = 0;
            break;
    }

    printf("Content-Type: text/html\r\n\r\n");

    printf("<!DOCTYPE html><html><style>#onboard{-webkit-transform: rotate(%ddeg);-moz-transform: rotate(%ddeg);-ms-transform: rotate(%ddeg);-o-transform: rotate(%ddeg);transform: rotate(%ddeg);z-index: -1;position: absolute;top: 0;bottom: 0;left: 0;right: 0;max-width: 100%%;max-height: 100%%;margin: auto;overflow: auto;background: url(data:image/jpeg;base64,%s);background-repeat: no-repeat;background-position: center;background-size:contain;}h2{color: white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;}h4{color: white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;}</style><head></head><body bgcolor=\"black\"><div id=\"onboard\"></div><h2 id=\"topleft\" color=\"white\">%s<br></h2><h4 id=\"topleft\"> -%s<br></h4></body></html>",
    degrees,degrees,degrees,degrees,degrees,
    base64img, message, name);
}

int get_orientation(char *filename)
{
    // Append filename to filepath
    char *filepath = calloc(1, strlen(PICTURE_FILE_PATH) + strlen(filename) + 1);
    sprintf(filepath, "%s%s", PICTURE_FILE_PATH, filename);

    // Extract image orientation from EXIF data
    int orientation = 0;
    ExifData *exif_data = exif_data_new_from_file(filepath);
    if (exif_data)
    {
        ExifByteOrder byte_order = exif_data_get_byte_order(exif_data);
        ExifEntry *exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);

        if (exif_entry)
            orientation = exif_get_short(exif_entry->data, byte_order);

        exif_data_free(exif_data);
    }
    free(filepath);
    return orientation;
}

char *get_base64_img(char *filename)
{
    // Append filename to filepath
    char *filepath = calloc(1, strlen(PICTURE_FILE_PATH) + strlen(filename) + 1);
    sprintf(filepath, "%s%s", PICTURE_FILE_PATH, filename);

    // Load image into buffer
    FILE *f = fopen(filepath, "r");
    if (f == NULL)
    {
        free(filepath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);

    char *image_data = malloc(fsize + 1);
    size_t l = fread(image_data, 1, fsize, f);
    if (l < fsize)
    {
        free(filepath);
        free(image_data);
        fclose(f);
        return NULL;
    }
    fclose(f);
    image_data[fsize] = 0;

    int len;
    char *base64image = base64(image_data, fsize, &len);
    if (base64image == NULL)
    {
        return NULL;
    }

    free(filepath);
    free(image_data);

    return base64image;
}

int main(int argc, const char * argv[])
{

    // Check the password
    char *qs = getenv("QUERY_STRING");
    if (qs == NULL)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("No query string\n");
        return 0;
    }
    char *pass = strstr(qs, "password=");
    if (pass == NULL)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("No password\n");
        return 0;
    }
    pass += strlen("password=");
    if (strcmp(pass, PASSWORD) != 0)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Incorrect password or query string messed up.\n");
        return 0;
    }

    struct picture_data *picture_data = get_random_entry();

    if (picture_data == NULL)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Could not extract info from DB.\n");
        return 0;
    }

    char *mime;
    if (strstr(picture_data->filename, ".jpg") != NULL)
    {
        mime = "image/jpeg";
    }
    else if (strstr(picture_data->filename, ".png") != NULL)
    {
        mime = "image/png";
    }
    else
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Picture filename incorrect\n");
        return 0;
    }

    int orientation = get_orientation(picture_data->filename);
    if (orientation == 0) {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Could not get orientation from EXIF\n");
        return 0;
    }

    char *base64image = get_base64_img(picture_data->filename);
    if (base64image == NULL)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Error reading picture from disk\n");
        return 0;
    }

    print_view_page(base64image, orientation, mime, picture_data->message, picture_data->name);

    free(picture_data->filename);
    free(picture_data->message);
    free(picture_data->name);
    free(picture_data);
    free(base64image);

    return 0;
}
