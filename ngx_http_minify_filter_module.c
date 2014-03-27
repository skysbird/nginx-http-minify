
/*
 * Copyright (C) skysbird
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_jsmin.h"
#include "ngx_cssmin.h"

typedef struct {
    ngx_flag_t           enable;
    ngx_hash_t           types;
    ngx_array_t         *types_keys;
} ngx_http_minify_conf_t;


static ngx_str_t  ngx_http_minify_default_types[] = {
    ngx_string("application/x-javascript"),
    ngx_string("text/css"),
    ngx_null_string
};


static ngx_command_t  ngx_http_minify_filter_commands[] = {

    { ngx_string("minify"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_minify_conf_t, enable),
      NULL },

    { ngx_string("minify_types"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_types_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_minify_conf_t, types_keys),
      &ngx_http_minify_default_types[0] },

      ngx_null_command
};


static ngx_int_t ngx_http_minify_filter_init(ngx_conf_t *cf);
static void *ngx_http_minify_create_conf(ngx_conf_t *cf);
static char *ngx_http_minify_merge_conf(ngx_conf_t *cf, void *parent, 
    void *child);


static ngx_http_module_t  ngx_http_minify_filter_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_minify_filter_init,             /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_minify_create_conf,             /* create location configuration */
    ngx_http_minify_merge_conf               /* merge location configuration */
};


ngx_module_t  ngx_http_minify_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_minify_filter_module_ctx,      /* module context */
    ngx_http_minify_filter_commands,         /* module directives */
    NGX_HTTP_MODULE,                         /* module type */
    NULL,                                    /* init master */
    NULL,                                    /* init module */
    NULL,                                    /* init process */
    NULL,                                    /* init thread */
    NULL,                                    /* exit thread */
    NULL,                                    /* exit process */
    NULL,                                    /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;
static ngx_int_t ngx_http_minify_buf(ngx_buf_t *buf,ngx_http_request_t *r,
    ngx_open_file_info_t *of);


static ngx_int_t
ngx_http_minify_header_filter(ngx_http_request_t *r)
{
    ngx_http_minify_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(r, ngx_http_minify_filter_module);

    if (!conf->enable
        || (r->headers_out.status != NGX_HTTP_OK
            && r->headers_out.status != NGX_HTTP_FORBIDDEN
            && r->headers_out.status != NGX_HTTP_NOT_FOUND)
        || (r->headers_out.content_encoding
            && r->headers_out.content_encoding->value.len)
        || ngx_http_test_content_type(r, &conf->types) == NULL
        || r->header_only)
    {
        return ngx_http_next_header_filter(r);
    }

    ngx_http_clear_content_length(r);
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_minify_buf_in_memory(ngx_buf_t *buf,ngx_http_request_t *r);

static ngx_int_t
ngx_http_minify_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_buf_t                  *b;
    ngx_int_t                   rc;
    ngx_str_t                  *filename;
    ngx_uint_t                  level;
    ngx_chain_t                *cl;
    ngx_open_file_info_t        of;
    ngx_http_minify_conf_t     *conf;
    ngx_http_core_loc_conf_t   *ccf;

    ccf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    conf = ngx_http_get_module_loc_conf(r,ngx_http_minify_filter_module);
    if (!conf->enable){
        return ngx_http_next_body_filter(r,in);
    }

    if(ngx_http_test_content_type(r, &conf->types) == NULL){
        return ngx_http_next_body_filter(r,in);
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http minify filter");

    for (cl = in; cl; cl = cl->next) {
        b = cl->buf;

        if (cl->buf->in_file){
            ngx_memzero(&of, sizeof(ngx_open_file_info_t));

            of.read_ahead = ccf->read_ahead;
            of.directio = ccf->directio;
            of.valid = ccf->open_file_cache_valid;
            of.min_uses = ccf->open_file_cache_min_uses;
            of.errors = ccf->open_file_cache_errors;
            of.events = ccf->open_file_cache_events;

            filename = &cl->buf->file->name;
            if (ngx_open_cached_file(ccf->open_file_cache, filename, &of, r->pool)
                    != NGX_OK)
                {
                    switch (of.err) {

                    case 0:
                        return NGX_HTTP_INTERNAL_SERVER_ERROR;

                    case NGX_ENOENT:
                    case NGX_ENOTDIR:
                    case NGX_ENAMETOOLONG:

                        level = NGX_LOG_ERR;
                        rc = NGX_HTTP_NOT_FOUND;
                        break;

                    case NGX_EACCES:

                        level = NGX_LOG_ERR;
                        rc = NGX_HTTP_FORBIDDEN;
                        break;

                    default:

                        level = NGX_LOG_CRIT;
                        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
                        break;
                    }

                    if (rc != NGX_HTTP_NOT_FOUND || ccf->log_not_found) {
                        ngx_log_error(level, r->connection->log, of.err,
                                      "%s \"%V\" failed", of.failed, filename);
                    }


                    return rc;
                }

            if (!of.is_file) {
                ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno,
                              "\"%V\" is not a regular file", filename);
                return NGX_HTTP_NOT_FOUND;
            }

            if (of.size == 0) {
                continue;
            }

            ngx_http_minify_buf(b,r,&of);    

        } else {

           if (b->pos == NULL) {
                continue;
           }

           ngx_http_minify_buf_in_memory(b,r); 
        }

    }

    return ngx_http_next_body_filter(r,in);

}

static ngx_int_t 
ngx_http_minify_buf_in_memory(ngx_buf_t *buf,ngx_http_request_t *r)
{
    ngx_buf_t   *b = NULL, *dst = NULL, *min_dst = NULL;
    ngx_int_t size;

    size = buf->end - buf->start;

    dst = buf;
    dst->end[0] = 0;
    
    b = ngx_calloc_buf(r->pool); 
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    b->start = ngx_palloc(r->pool, size);
    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;
    
    min_dst = b;
    if (ngx_strcmp(r->headers_out.content_type.data,
                   ngx_http_minify_default_types[0].data) 
        == 0)
    {
        jsmin(dst,min_dst);

    } else if (ngx_strcmp(r->headers_out.content_type.data, 
                          ngx_http_minify_default_types[1].data) 
               == 0)
    {
        cssmin(dst,min_dst);

    } else {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf->start = min_dst->start;     
    buf->pos = min_dst->pos;
    buf->last = min_dst->last;
    buf->end = min_dst->end;
    buf->memory = 1;
    buf->in_file = 0;
    

    return NGX_OK;

}

static ngx_int_t 
ngx_http_minify_buf(ngx_buf_t *buf,ngx_http_request_t *r,
                    ngx_open_file_info_t *of)
{
    ngx_buf_t   *b = NULL, *dst = NULL, *min_dst = NULL;
    ngx_int_t    size;
    ngx_file_t  *src_file ;
    ssize_t n;

    src_file =  ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    if (src_file == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    src_file->fd = of->fd;
    src_file->name = buf->file->name;
    src_file->log = r->connection->log;
    src_file->directio = of->is_directio;

    size = of->size;

    b = ngx_calloc_buf(r->pool); 
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->start = ngx_palloc(r->pool, size+1);
    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size ;
    b->temporary = 1;

    dst = b;

    #if (NGX_HAVE_FILE_AIO)
    
         ngx_output_chain_ctx_t       *ctx;
         ctx = ngx_http_get_module_ctx(r, ngx_http_minify_filter_module);
         if (ctx->aio_handler) {
             n = ngx_file_aio_read(src_file, dst->pos, (size_t) size, 0, 
                                   ctx->pool);

             if (n == NGX_AGAIN) {
             ctx->aio_handler(ctx, src_file);
             return NGX_AGAIN;
             }
    
         } else {
             n = ngx_read_file(src_file, dst->pos, (size_t) size, 0);
         }
    #else
    
        ngx_read_file(src_file, dst->pos, (size_t) size, 0);
    #endif

    dst->end[0] = 0;
    
    b = ngx_calloc_buf(r->pool); 
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    b->start = ngx_palloc(r->pool, size);
    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;
    
    min_dst = b;
    if (ngx_strcmp(r->headers_out.content_type.data,
                   ngx_http_minify_default_types[0].data) 
        == 0)
    {
        jsmin(dst,min_dst);

    } else if (ngx_strcmp(r->headers_out.content_type.data, 
                          ngx_http_minify_default_types[1].data) 
               == 0)
    {
        cssmin(dst,min_dst);

    } else {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf->start = min_dst->start;     
    buf->pos = min_dst->pos;
    buf->last = min_dst->last;
    buf->end = min_dst->end;
    buf->memory = 1;
    buf->in_file = 0;
    

    return NGX_OK;

}

static void *
ngx_http_minify_create_conf(ngx_conf_t *cf)
{
    ngx_http_minify_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_minify_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->bufs.num = 0;
     *     conf->types = { NULL };
     *     conf->types_keys = NULL;
     */

    conf->enable = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_minify_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_minify_conf_t *prev = parent;
    ngx_http_minify_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    if (ngx_http_merge_types(cf, &conf->types_keys, &conf->types,
                             &prev->types_keys, &prev->types,
                             ngx_http_minify_default_types)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_minify_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_minify_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_minify_body_filter;

    return NGX_OK;
}



