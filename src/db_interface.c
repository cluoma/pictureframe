//
//  db_interface.c
//  pictureframe
//
//  Created by Colin Luoma on 2017-03-18
//  Copyright © 2017 Colin Luoma. All rights reserved.
//

#include "pictureframe.h"
#include "db_interface.h"

// Opens database
sqlite3 *open_database()
{
    sqlite3 *db;

    int rc = sqlite3_open(SQLITE_DB_FILE, &db);

    if( rc )
    {
        // Couldn't open
        sqlite3_close(db);
        return NULL;
    }
    else
    {
        // All good
        return db;
    }
}

// Attempts to create the appropriate SQLITE3 db table
int create_table(sqlite3 *db)
{
    sqlite3_stmt *results;

    char *sql = "CREATE TABLE IF NOT EXISTS `pictures` ( \
 	`id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE, \
 	`time`	NUMERIC NOT NULL, \
    `shown` NUMERIC NOT NULL, \
 	`name`	TEXT, \
 	`message`	TEXT, \
 	`filename`	TEXT);";

    // Prepare statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &results, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
    }

    // Execute statement and finalize
    sqlite3_step(results);
    sqlite3_finalize(results);

    return 1;
}

int reset_shown(sqlite3 *db)
{
    sqlite3_stmt *results;

    char *sql = "UPDATE pictures \
                 SET shown = 0;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &results, 0);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(results);
        return 0;
    }

    sqlite3_step(results);
    sqlite3_finalize(results);

    return 1;
}

int set_shown(sqlite3 *db, int id)
{
    sqlite3_stmt *results;

    char sql[60];
    sprintf(sql, "UPDATE pictures SET shown = 1 WHERE id = %d;", id);

    int rc = sqlite3_prepare_v2(db, sql, -1, &results, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
        sqlite3_finalize(results);
    }

    sqlite3_step(results);
    sqlite3_finalize(results);

    return 1;
}

int add_entry_to_db(int time,
                    const char*name, size_t name_len,
                    const char*message, size_t message_len,
                    const char*filename, size_t filename_len) {

    sqlite3_stmt *results;

    // Open db connection
    sqlite3 *db = open_database();
    if (db == NULL)
    {
       return 0;
    }

    // We couldn't create the table
    if (!create_table(db))
    {
        sqlite3_close(db);
        return 0;
    }

    char *sql = "INSERT INTO pictures (name, shown, message, filename, time) VALUES(?, 0, ?, ?, ?)";
    // Prepare statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &results, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_text(results, 1, name, (int)name_len, SQLITE_TRANSIENT);
    sqlite3_bind_text(results, 2, message, (int)message_len, SQLITE_TRANSIENT);
    sqlite3_bind_text(results, 3, filename, (int)filename_len, SQLITE_TRANSIENT);
    sqlite3_bind_int(results, 4, time);

    sqlite3_step(results);
    sqlite3_finalize(results);

    sqlite3_close(db);

    return 1;
}

struct picture_data *get_random_entry()
{
    struct picture_data *picture_data = NULL;
    sqlite3_stmt *results;

    sqlite3 *db = open_database();
    if (db == NULL)
    {
        return NULL;
    }

    char *sql = "SELECT t1.`id`, t1.message, t1.name, t1.filename \
                FROM (\
                SELECT id, message, name, filename \
                FROM pictures \
                WHERE shown = 0 \
                ORDER BY time DESC LIMIT 20) t1 \
                ORDER BY RANDOM() LIMIT 1";

    int rc = sqlite3_prepare_v2(db, sql, -1, &results, 0);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(results);
        sqlite3_close(db);
        return NULL;
    }


    int step = sqlite3_step(results);
    if(step == SQLITE_ROW) // Get first result
    {
        picture_data = malloc(sizeof(struct picture_data));

        int id                  = sqlite3_column_int(results, 0);
        const char *message     = (char *) sqlite3_column_text(results, 1);
        int message_len         = sqlite3_column_bytes(results, 1);
        const char *name        = (char *) sqlite3_column_text(results, 2);
        int name_len            = sqlite3_column_bytes(results, 2);
        const char *filename    = (char *) sqlite3_column_text(results, 3);
        int filename_len        = sqlite3_column_bytes(results, 3);


        // Message
        picture_data->message = calloc(1, message_len + 1);
        memcpy(picture_data->message, message, message_len);
        // Name
        picture_data->name = calloc(1, name_len + 1);
        memcpy(picture_data->name, name, name_len);
        // Filename
        picture_data->filename = calloc(1, filename_len + 1);
        memcpy(picture_data->filename, filename, filename_len);

        sqlite3_finalize(results);
        set_shown(db, id);
    }
    else if (step == SQLITE_DONE) // No results
    {
        sqlite3_finalize(results);
        reset_shown(db);
        sqlite3_close(db);
        return get_random_entry();
    }

    sqlite3_close(db);

    return picture_data;
}
