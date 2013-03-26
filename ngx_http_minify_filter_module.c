#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>



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
static char *ngx_http_minify_merge_conf(ngx_conf_t *cf, void *parent, void *child);



static ngx_http_module_t  ngx_http_minify_filter_module_ctx = {
    NULL,           /* preconfiguration */
    ngx_http_minify_filter_init,             /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_minify_create_conf,             /* create location configuration */
    ngx_http_minify_merge_conf               /* merge location configuration */
};


ngx_module_t  ngx_http_minify_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_minify_filter_module_ctx,      /* module context */
    ngx_http_minify_filter_commands,         /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

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


    return ngx_http_next_header_filter(r);
}


static ngx_int_t
ngx_http_minify_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{

//    ngx_buf_t             *b;
    ngx_chain_t           *cl;
    ngx_http_minify_conf_t  *conf;
  
    conf = ngx_http_get_module_loc_conf(r,ngx_http_minify_filter_module);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http minify filter");
    if(ngx_http_test_content_type(r, &conf->types) == NULL){
        return ngx_http_next_body_filter(r,in);
    }

    cl = in; 
    while (cl) {
        cl = cl->next;
    }

    return ngx_http_next_body_filter(r,in);

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



