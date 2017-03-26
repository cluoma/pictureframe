//
//  pictureframe.h
//  pictureframe
//
//  Created by Colin Luoma on 2017-03-18
//  Copyright (c) 2017 Colin Luoma. All rights reserved.
//

#include <stdlib.h>
#include "multipart_parser.h"

#ifndef PICTURE_FILE_PATH
    #define PICTURE_FILE_PATH "/home/colin/"
#endif
#ifndef SQLITE_DB_FILE
    #define SQLITE_DB_FILE "/home/colin/pictureframe.db"
#endif
#ifndef PASSWORD
    #define PASSWORD "password"
#endif

struct multipart_chunk_data {
    char *data;
    size_t len;

    int mime;
};

struct form_data {
    struct multipart_chunk_data chunk_data[3];
    int form_element;

    int status;
};

enum {
    NAME,
    MESSAGE,
    PICTURE_DATA
};

enum {
    OK,
    NOT_OK
};

enum {
    IMG_JPG,
    IMG_PNG,
    TEXT_PLAIN
};

// Prints confirmation HTML to stdout
void print_confirm_page(char *heading, char *message);

// Naive implementation of a fixed length strstr
char *sstrstr(const char *haystack, const char *needle, size_t length);

// Make sure the request method and content type match
int check_request_method(const char *method);
int check_content_type(const char *mime);

// Returns 0 if content length is missing
long get_content_length();

// returns the multipart boundary string, caller must free when finished
char *get_boundary();

// Multipart parser callback functions
int read_header_value(multipart_parser* p, const char *at, size_t length);
int read_data_part(multipart_parser* p, const char *at, size_t length);
int read_multipart_end(multipart_parser* p);

// Init and free form data
void init_form_data(struct form_data * form_data);
void free_form_data(struct form_data * form_data);
