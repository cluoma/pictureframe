//
//  db_interface.h
//  pictureframe
//
//  Created by Colin Luoma on 2017-03-18
//  Copyright © 2017 Colin Luoma. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

struct picture_data {
    char *message;
    char *name;
    char *filename;
};

// Adds data to database
// Also creates the database if it doesn't exist
int add_entry_to_db(int time,
                    const char*name, size_t name_len,
                    const char*message, size_t message_len,
                    const char*filename, size_t filename_len);

// Returns a database entry for a random picture to display
// Returns NULL on failure
struct picture_data *get_random_entry();
