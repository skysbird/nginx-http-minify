/* jsmin.c
   2013-02-25

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ngx_core.h>

static int   theA;
static int   theB;
static int   theLookahead = EOF;
static int   theX = EOF;
static int   theY = EOF;

int ngx_getc(ngx_buf_t *in){
    if (in->pos > in->end){
       return EOF;
    }
    u_char c = in->pos[0];
    ++in->pos;
    return c;
}

void ngx_putc(u_char c,ngx_buf_t *out){
    if(out->pos<=out->end){
 	out->pos[0] = c;
	++out->pos;
    }
}
static void
error(char* s)
{
    fputs("JSMIN Error: ", stderr);
    fputs(s, stderr);
    fputc('\n', stderr);
    exit(1);
}

/* isAlphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/

static int
isAlphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
        c > 126);
}


/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

static int
get(ngx_buf_t *in)
{
    int c = theLookahead;
    theLookahead = EOF;
    if (c == EOF) {
        c = ngx_getc(in);
    }
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}


/* peek -- get the next character without getting it.
*/

static int
peek(ngx_buf_t *in)
{
    theLookahead = get(in);
    return theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/

static int
next(ngx_buf_t *in)
{
    int c = get(in);
    if  (c == '/') {
        switch (peek(in)) {
        case '/':
            for (;;) {
                c = get(in);
                if (c <= '\n') {
                    break;
                }
            }
            break;
        case '*':
            get(in);
            while (c != ' ') {
                switch (get(in)) {
                case '*':
                    if (peek(in) == '/') {
                        get(in);
                        c = ' ';
                    }
                    break;
                case EOF:
                    error("Unterminated comment.");
                }
            }
            break;
        }
    }
    theY = theX;
    theX = c;
    return c;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

static void
action(int d,ngx_buf_t *in,ngx_buf_t *out)
{
    switch (d) {
    case 1:
        ngx_putc(theA, out);
        if (
            (theY == '\n' || theY == ' ') &&
            (theA == '+' || theA == '-' || theA == '*' || theA == '/') &&
            (theB == '+' || theB == '-' || theB == '*' || theB == '/')
        ) {
            ngx_putc(theY, out);
        }
    case 2:
        theA = theB;
        if (theA == '\'' || theA == '"' || theA == '`') {
            for (;;) {
                ngx_putc(theA, out);
                theA = get(in);
                if (theA == theB) {
                    break;
                }
                if (theA == '\\') {
                    ngx_putc(theA, out);
                    theA = get(in);
                }
                if (theA == EOF) {
                    error("Unterminated string literal.");
                }
            }
        }
    case 3:
        theB = next(in);
        if (theB == '/' && (
            theA == '(' || theA == ',' || theA == '=' || theA == ':' ||
            theA == '[' || theA == '!' || theA == '&' || theA == '|' ||
            theA == '?' || theA == '+' || theA == '-' || theA == '~' ||
            theA == '*' || theA == '/' || theA == '\n'
        )) {
            ngx_putc(theA, out);
            if (theA == '/' || theA == '*') {
                ngx_putc(' ', out);
            }
            ngx_putc(theB, out);
            for (;;) {
                theA = get(in);
                if (theA == '[') {
                    for (;;) {
                        ngx_putc(theA, out);
                        theA = get(in);
                        if (theA == ']') {
                            break;
                        }
                        if (theA == '\\') {
                            ngx_putc(theA, out);
                            theA = get(in);
                        }
                        if (theA == EOF) {
                            error("Unterminated set in Regular Expression literal.");
                        }
                    }
                } else if (theA == '/') {
                    switch (peek(in)) {
                    case '/':
                    case '*':
                        error("Unterminated set in Regular Expression literal.");
                    }
                    break;
                } else if (theA =='\\') {
                    ngx_putc(theA, out);
                    theA = get(in);
                }
                if (theA == EOF) {
                    error("Unterminated Regular Expression literal.");
                }
                ngx_putc(theA, out);
            }
            theB = next(in);
        }
    }
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

void
jsmin(ngx_buf_t *in,ngx_buf_t *out)
{
    if (peek(in) == 0xEF) {
        get(in);
        get(in);
        get(in);
    }
    theA = '\n';
    action(3,in,out);
    while (theA != EOF) {
        switch (theA) {
        case ' ':
            action(isAlphanum(theB) ? 1 : 2,in,out);
            break;
        case '\n':
            switch (theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
            case '!':
            case '~':
                action(1,in,out);
                break;
            case ' ':
                action(3,in,out);
                break;
            default:
                action(isAlphanum(theB) ? 1 : 2,in,out);
            }
            break;
        default:
            switch (theB) {
            case ' ':
                action(isAlphanum(theA) ? 1 : 3,in,out);
                break;
            case '\n':
                switch (theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                case '`':
                    action(1,in,out);
                    break;
                default:
                    action(isAlphanum(theA) ? 1 : 3,in,out);
                }
                break;
            default:
                action(1,in,out);
                break;
            }
        }
    }
    out->end = out->pos;
    out->last = out->pos;
    out->pos = out->start;
}


/* main -- Output any command line arguments as comments
        and then minify the input.
*/
/*extern int
main(int argc, char* argv[])
{
    int i;
    for (i = 1; i < argc; i += 1) {
        fprintf(out, "// %s\n", argv[i]);
    }
    jsmin();
    return 0;
}*/

/*void test2(){
	printf("shit\n");
	jsmin();
}*/
