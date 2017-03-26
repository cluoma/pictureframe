PICTURE_FILE_PATH=\"/home/colin/\"
SQLITE_DB_FILE=\"/home/colin/pictureframe.db\"
PASSWORD=\"password\"

all: pictureframe pictureframe_viewer

pictureframe:
	gcc -o pictureframe.cgi -std=c99 -DPICTURE_FILE_PATH=$(PICTURE_FILE_PATH) -DSQLITE_DB_FILE=$(SQLITE_DB_FILE) -Wall -O3 -D_GNU_SOURCE src/pictureframe.c src/multipart_parser.c src/db_interface.c -lsqlite3

pictureframe_viewer:
	gcc -o pictureframe_viewer.cgi -std=c99 -DPICTURE_FILE_PATH=$(PICTURE_FILE_PATH) -DSQLITE_DB_FILE=$(SQLITE_DB_FILE) -DPASSWORD=$(PASSWORD) -Wall -O3 src/pictureframe_viewer.c src/db_interface.c -lsqlite3 -lexif

install:
	mv pictureframe.cgi cgi-bin/
	mv pictureframe_viewer.cgi cgi-bin/
