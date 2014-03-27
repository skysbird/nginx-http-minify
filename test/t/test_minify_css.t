use lib 'lib';
use Shell;
use Test::Nginx::Socket;
plan tests => blocks() * 2;
run_tests();


__DATA__

=== TEST 0:0 jsmin with sendfile on
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

sendfile on;
--- config
    minify on;
--- user_files
>>> a.css
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}
--- request
    GET /a.css
--- response_body eval 
"\@charset \"utf-8\";*{outline: 0;padding: 0;margin: 0;border: 0;}body{font-size: 12px;font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;text-align:center;margin:0 auto;background-color: #E8E7E7;position:relative;} " 


=== TEST 0:1 jsmin without sendfile
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

--- config
    minify on;
--- user_files
>>> a.css
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}
--- request
    GET /a.css
--- response_body eval 

"\@charset \"utf-8\";*{outline: 0;padding: 0;margin: 0;border: 0;}body{font-size: 12px;font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;text-align:center;margin:0 auto;background-color: #E8E7E7;position:relative;} " 


=== TEST 0:2 jsmin with gzipon without sendfile 
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

--- config
    minify on;
    gzip on;
--- user_files
>>> a.css
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}
--- request
    GET /a.css
--- response_body eval 
"\@charset \"utf-8\";*{outline: 0;padding: 0;margin: 0;border: 0;}body{font-size: 12px;font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;text-align:center;margin:0 auto;background-color: #E8E7E7;position:relative;} " 

=== TEST 0:3 jsmin without minify 
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

--- config
    gzip on;
--- user_files
>>> a.css
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}
--- request
    GET /a.css
--- response_body
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}

=== TEST 0:4 jsmin with sendfile on and gzip on
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

sendfile on;
--- config
    minify on;
    gzip on;
--- user_files
>>> a.css
@charset "utf-8";
/* CSS Document */
* {
    outline: 0;
    padding: 0;
    margin: 0;
    border: 0;
}

body {
    font-size: 12px;
    font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;
    text-align:center;
    margin:0 auto;
    background-color: #E8E7E7;
    position:relative;
}
--- request
    GET /a.css
--- response_body eval
"\@charset \"utf-8\";*{outline: 0;padding: 0;margin: 0;border: 0;}body{font-size: 12px;font-family: '宋体' ,Arial, Helvetica, Garuda, sans-serif;text-align:center;margin:0 auto;background-color: #E8E7E7;position:relative;} " 

=== TEST 0:5 jsmin with sendfile on and empty file
--- http_config
types {
    text/html                             html htm shtml;
    text/css                              css;
    application/x-javascript              js;
}

sendfile on;
--- config
    minify on;
    gzip on;
--- user_files
>>> a.css
--- request
    GET /a.css
--- response_body: 


