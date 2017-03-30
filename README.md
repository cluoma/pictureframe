# pictureframe

Upload images and view them.

`pictureframe` consists of 3 files: `submit.html`, `pictureframe.cgi`, and `pictureframe_viewer.cgi`.

`submit.html` is a simple form page to upload an image (mandatory) as well as an optional message and name from submitter.

`pictureframe.cgi` receives the form data from `submit.html` in the multipart encoded POST request. The users name, message, and the name of the picture are stored in an SQLite3 database at the location supplied by the user.

`pictureframe_viewer.cgi` queries the SQLite3 database for a picture that hasn't been displayed yet and is shown in a fullscreen browser view. The user must supply the correct password in the URI query string to access the pictures.

### Installation
Ensure that sqlite3 and libexif -dev libraries are installed on your OS. Twitter's bootstrap CSS should also be installed in your webservers directory. If not, copy the `bootstrap.css` file in this repo to `/css/` in your webservers root directory.
```
git clone https://github.com/cluoma/pictureframe
cd pictureframe

# Edit variables in Makefile as appropriate
# Ensure that your webserver has read/write access to supplied directories
nano Makefile

make
cp submit.html <webserver docroot>/
cp pictureframe.cgi <webserver docroot>/cgi-bin/
cp pictureframe_viewer.cgi <webserver docroot>/cgi-bin/
```

### Dependencies/Libraries
 - [sqlite3](https://www.sqlite.org/)
 - [libexif](http://libexif.sourceforge.net/)
 - [multipart-parser-c](https://github.com/iafonov/multipart-parser-c)
 - [base64](https://github.com/superwills/NibbleAndAHalf)
