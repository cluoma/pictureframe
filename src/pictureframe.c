//
//  pictureframe.c
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
#include <unistd.h>

#include "pictureframe.h"
#include "multipart_parser.h"
#include "db_interface.h"

void print_confirm_page(char *heading, char *message)
{
    if (heading == NULL) heading = "Oops! Something went wrong :(";
    if (message == NULL) message = "Try again. Maybe it will work next time?";

    printf("Content-Type: text/html\r\n\r\n");

    printf(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Image Uploader</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><link href=\"/css/bootstrap.css\" rel=\"stylesheet\"><link rel=\"shortcut icon\" href=\"/images/favicon.ico\"></head><body><div id=\"page\" class=\"page\"><div class=\"container\"><div class=\"row\"><div class=\"col-md-offset-3 col-md-6 col-sm-12 col-xs-12 text-center\"><h2>%s</h2><p class=\"big-para\">%s</p></div></div></div></div></body></html>",
        heading, message
    );
}

char *sstrstr(const char *haystack, const char *needle, size_t length)
{
    size_t needle_length = strlen(needle);
    size_t i;

    for (i = 0; i < length; i++)
    {
        if (i + needle_length > length)
        {
            return NULL;
        }

        if (strncmp(&haystack[i], needle, needle_length) == 0)
        {
            return (char *)&haystack[i];
        }
    }
    return NULL;
}

int check_request_method(const char *method)
{
    char *r_method = getenv("REQUEST_METHOD");
    if (r_method == NULL || strcmp(r_method, method) != 0)
    {
        return 0;
    }
    return 1;
}

int check_content_type(const char *mime)
{
    char *c_type = getenv("CONTENT_TYPE");
    if (c_type == NULL)
    {
        return 0;
    }
    char *t = strstr(c_type, mime);
    if (t == NULL || c_type != t)
    {
        return 0;
    }
    return 1;
}

long get_content_length()
{
    char *env_length = getenv("CONTENT_LENGTH");
    if (env_length == NULL)
    {
        return 0;
    }
    errno = 0;
    long length = strtol(env_length, NULL, 10);
    if (errno != 0)
    {
        return 0;
    }
    return length;
}

char *get_boundary()
{
    char *boundary = strstr(getenv("CONTENT_TYPE"), "boundary=");
    if (boundary != NULL)
    {
        boundary += strlen("boundary=");
        char *ret_str = calloc(1, strlen(boundary) + 3);

        if (ret_str == NULL) return NULL;

        strcat(ret_str, "--");
        strcat(ret_str, boundary);
        return ret_str;
    }
    return NULL;
}

int read_header_value(multipart_parser* p, const char *at, size_t length)
{
   struct form_data *form_data = (struct form_data *)multipart_parser_get_data(p);

   if (sstrstr(at, "name=\"name\"", length) != NULL)
   {
       form_data->form_element = NAME;
       form_data->chunk_data[NAME].mime = TEXT_PLAIN;
   }
   else if (sstrstr(at, "name=\"message\"", length) != NULL)
   {
       form_data->form_element = MESSAGE;
       form_data->chunk_data[MESSAGE].mime = TEXT_PLAIN;
   }
   else if (sstrstr(at, "name=\"file_source\"", length) != NULL)
   {
       form_data->form_element = PICTURE_DATA;
       form_data->chunk_data[PICTURE_DATA].mime = TEXT_PLAIN;
   }

   // Get MIME type from picture
   if (form_data->form_element == PICTURE_DATA && sstrstr(at, "image/png", length) != NULL)
   {
       form_data->chunk_data[PICTURE_DATA].mime = IMG_PNG;
   }
   else if (form_data->form_element == PICTURE_DATA && sstrstr(at, "image/jpeg", length) != NULL)
   {
       form_data->chunk_data[PICTURE_DATA].mime = IMG_JPG;
   }

   return 0;
}

int read_data_part(multipart_parser* p, const char *at, size_t length)
{
    if (length == 0) return 0;

    struct form_data *form_data = (struct form_data *)multipart_parser_get_data(p);

    char * cur_data = form_data->chunk_data[form_data->form_element].data;
    size_t cur_len = form_data->chunk_data[form_data->form_element].len;

    cur_data = realloc(cur_data, cur_len + length);

    if (cur_data == NULL)
    {
        form_data->status = NOT_OK;
        return 1;
    }

    memcpy(cur_data + cur_len, at, length);
    form_data->chunk_data[form_data->form_element].data = cur_data;
    form_data->chunk_data[form_data->form_element].len += length;

    return 0;
}

int read_multipart_end(multipart_parser* p)
{
    struct form_data *form_data = (struct form_data *)multipart_parser_get_data(p);

    // Data from picture was not acceptable
    if (form_data->chunk_data[PICTURE_DATA].data == NULL ||
        form_data->chunk_data[PICTURE_DATA].mime == TEXT_PLAIN)
    {
        form_data->status = NOT_OK;
        return 1;
    }

    // No message or name data, replace with empty string
    if (form_data->chunk_data[NAME].data == NULL)
    {
        form_data->chunk_data[NAME].data = calloc(1, 1);
    }
    if (form_data->chunk_data[MESSAGE].data == NULL)
    {
        form_data->chunk_data[MESSAGE].data = calloc(1, 1);
    }

    return 0;
}

void init_form_data(struct form_data * form_data)
{
    form_data->chunk_data[NAME].data = NULL;
    form_data->chunk_data[NAME].len = 0;
    form_data->chunk_data[NAME].mime = TEXT_PLAIN;
    form_data->chunk_data[MESSAGE].data = NULL;
    form_data->chunk_data[MESSAGE].len = 0;
    form_data->chunk_data[MESSAGE].mime = TEXT_PLAIN;
    form_data->chunk_data[PICTURE_DATA].data = NULL;
    form_data->chunk_data[PICTURE_DATA].len = 0;
    form_data->chunk_data[PICTURE_DATA].mime = TEXT_PLAIN;
    form_data->status = OK;
}

void free_form_data(struct form_data * form_data)
{
    for (int i = 0; i < 3; i++)
    {
        char *data = form_data->chunk_data[i].data;
        if (data != NULL)
            free(data);
    }
}

int main(int argc, const char * argv[]) {

    // We want a post method
    if (!check_request_method("POST"))
    {
        print_confirm_page(NULL, "Incorrect HTTP request method");
        goto fail_request;
    }

    // Make sure its multipart/form-data
    if (!check_content_type("multipart/form-data"))
    {
        print_confirm_page(NULL, "Incorrect mime-type (Picture format not supported)");
        goto fail_content;
    }

    // Get boundry
    char *boundary = get_boundary();
    if (boundary == NULL)
    {
        print_confirm_page(NULL, "Could not parse multipart boundary");
        goto fail_boundary;
    }

    // How many characters should we read in from STDIN?
    long length = get_content_length();
    if (length == 0)
    {
        print_confirm_page(NULL, "Invalid content length");
        goto fail_length;
    }

    // Allocate space and read in ENV QUERY
    char *input = malloc(length);
    if (input == NULL)
    {
        print_confirm_page(NULL, "Memory error");
        goto fail_input;
    }

    // Read in POST content data from STDIN
    size_t read_bytes = fread(input, 1, length, stdin);
    if (read_bytes < length)
    {
        print_confirm_page(NULL, "Post data does not match content length");
        goto fail_read;
    }

    // Struct to hold form data
    struct form_data form_data;
    init_form_data(&form_data);

    //printf("Content-Type: text/plain\r\n\r\n<pre>");
    //printf("%s", input);

    // Setup multipart parser callbacks
    multipart_parser_settings callbacks;
    memset(&callbacks, 0, sizeof(multipart_parser_settings));
    callbacks.on_header_value = read_header_value;
    callbacks.on_part_data = read_data_part;
    callbacks.on_body_end = read_multipart_end;

    multipart_parser* parser = multipart_parser_init(boundary, &callbacks);
    multipart_parser_set_data(parser, &form_data);
    multipart_parser_execute(parser, input, length);
    multipart_parser_free(parser);

    // Make sure everything went okay
    if (form_data.status == NOT_OK ||
        form_data.chunk_data[PICTURE_DATA].mime == TEXT_PLAIN)
    {
        print_confirm_page(NULL, "You probably forgot to attach a picture!");
        goto fail_parse;
    }

    // We don't need these anymore
    free(input);
    free(boundary);

    // Set filename for output, filename is just a timestamp
    int epoch = (int)time(NULL);
    char filename[20] = { 0 };
    if (form_data.chunk_data[PICTURE_DATA].mime == IMG_JPG)
    {
        sprintf(filename, "%d.%s", epoch, "jpg");
    }
    else if (form_data.chunk_data[PICTURE_DATA].mime == IMG_PNG)
    {
        sprintf(filename, "%d.%s", epoch, "png");
    }

    // Complete file path with file name
    char *file_path = malloc(strlen(PICTURE_FILE_PATH) + strlen(filename) + 1);
    sprintf(file_path, "%s%s", PICTURE_FILE_PATH, filename);

    // Open file using filepath and filename
    FILE *out = fopen(file_path, "wb+");
    if (out == NULL)
    {
        print_confirm_page(NULL, "Error writing picture to disk");
        goto fail_open_file;
    }
    free(file_path);

    // Write out data into file and free data
    size_t out_bytes = fwrite(form_data.chunk_data[PICTURE_DATA].data, 1,
                              form_data.chunk_data[PICTURE_DATA].len, out);
    if (out_bytes != form_data.chunk_data[PICTURE_DATA].len)
    {
        print_confirm_page(NULL, "Error writing picture to disk");
        goto fail_write_out;
    }
    fclose(out);

    if (!add_entry_to_db(epoch,
        form_data.chunk_data[NAME].data, form_data.chunk_data[NAME].len,
        form_data.chunk_data[MESSAGE].data, form_data.chunk_data[MESSAGE].len,
        filename, strlen(filename)))
    {
        print_confirm_page(NULL, "Database error");
        goto fail_db;
    }

    free_form_data(&form_data);

    print_confirm_page("Success! :D", "Thanks for the picture!");

    #ifdef NOTIFY_WAV
    fflush(stdout);
    if( fork() == 0 )
    {
        execlp("sudo", "sudo", "-u", "linaro", "/usr/local/bin/notifyme", NOTIFY_WAV, (char *)NULL);
    }
    #endif

    return 0; // Success

    // Exit points
fail_db:
fail_write_out:
fail_open_file:
    if (file_path != NULL) free(file_path);
fail_parse:
    free_form_data(&form_data);
fail_read:
fail_input:
    if (input != NULL) free(input);
fail_length:
fail_boundary:
    if (boundary != NULL) free(boundary);
fail_content:
fail_request:
    return 1;
}
