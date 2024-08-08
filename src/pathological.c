#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static void set_malformed_header(ngx_http_request_t *r, const char *header_name, const char *header_value) {
    ngx_table_elt_t *h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return;
    }

    h->hash = 1;
    h->key.len = strlen(header_name);
    h->key.data = (u_char *) header_name;
    h->value.len = strlen(header_value);
    h->value.data = (u_char *) header_value;
}

static ngx_int_t ngx_http_malformed_headers_handler(ngx_http_request_t *r) {
    u_char qs_param[] = "malformed-header";

    // if the qs param is not present, decline
    ngx_str_t value;
    if (ngx_http_arg(r, qs_param, sizeof(qs_param) - 1, &value) != NGX_OK) {
        return NGX_DECLINED;
    }

    // compare the querystring to all cases
    if (ngx_strncmp(value.data, "spaces-in-header-name", value.len) == 0) {
        set_malformed_header(r, "Spaces In Header Name", "This header name contains spaces");

    } else if (ngx_strncmp(value.data, "trailing-whitespace", value.len) == 0) {
        set_malformed_header(r, "TrailingWhitespace ", "This header has trailing whitespace");

    } else if (ngx_strncmp(value.data, "header-injection", value.len) == 0) {
        set_malformed_header(r, "HeaderInjection\r\nInjected-Header", "This header contains an injection");

    } else if (ngx_strncmp(value.data, "non-printable-chars", value.len) == 0) {
        char header_value[] = "NonPrintableChars\x01\x02";
        set_malformed_header(r, "NonPrintableChars", header_value);

    } else if (ngx_strncmp(value.data, "missing-value", value.len) == 0) {
        set_malformed_header(r, "MissingValue", "");

    } else if (ngx_strncmp(value.data, "multiple-colons", value.len) == 0) {
        set_malformed_header(r, "MultipleColons", "Value1: Value2");

    } else if (ngx_strncmp(value.data, "invalid-encoding", value.len) == 0) {
        char header_value[] = "InvalidEncoding\xc3\x28";
        set_malformed_header(r, "InvalidEncoding", header_value);

    } else if (ngx_strncmp(value.data, "oversized-headers", value.len) == 0) {
        char header_value[9000];
        memset(header_value, 'X', 8999);
        header_value[8999] = '\0';
        set_malformed_header(r, "OversizedHeader", header_value);

    } else if (ngx_strncmp(value.data, "duplicate-headers", value.len) == 0) {
        set_malformed_header(r, "DuplicateHeader", "Value1");
        set_malformed_header(r, "DuplicateHeader", "Value2");

    } else if (ngx_strncmp(value.data, "invalid-characters", value.len) == 0) {
        set_malformed_header(r, "Invalid\xFF""Chars", "This header contains invalid characters");

    // TODO handle missing colon MissingColon[:] Value
    } else {
        return NGX_DECLINED;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = 0;

    ngx_http_send_header(r);

    return ngx_http_output_filter(r, &(ngx_chain_t){.buf = &(ngx_buf_t){.last_buf = 1}});
}

static char* ngx_http_malformed_headers(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_malformed_headers_handler;

    return NGX_CONF_OK;
}

ngx_module_t pathological_module = {
    NGX_MODULE_V1,
    &(ngx_http_module_t){0},
   (ngx_command_t[]){
      {
        .name = ngx_string("pathological"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        .set = ngx_http_malformed_headers,
      },
      ngx_null_command
    },
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};
