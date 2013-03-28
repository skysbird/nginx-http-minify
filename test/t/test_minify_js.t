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
>>> a.js
alert('a');
alert('b');
alert('c');
--- request
    GET /a.js
--- response_body eval
"\x{0a}alert('a');alert('b');alert('c');"




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
>>> a.js
alert('a');
alert('b');
alert('c');
--- request
    GET /a.js
--- response_body eval
"\x{0a}alert('a');alert('b');alert('c');"


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
>>> a.js
alert('a');
alert('b');
alert('c');
--- request
    GET /a.js
--- response_body eval
"\x{0a}alert('a');alert('b');alert('c');"


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
>>> a.js
alert('a');
alert('a');
alert('a');
--- request
    GET /a.js
--- response_body
alert('a');
alert('a');
alert('a');

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
>>> a.js
alert('a');
alert('b');
alert('c');
--- request
    GET /a.js
--- response_body eval
"\x{0a}alert('a');alert('b');alert('c');"

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
>>> a.js
--- request
    GET /a.js
--- response_body: 

 






