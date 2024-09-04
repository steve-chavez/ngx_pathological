#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdbool.h>

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

static void final_handler(ngx_event_t *ev) {
    ngx_http_request_t *r = ev->data;

    ngx_http_send_header(r);
    ngx_http_output_filter(r, &(ngx_chain_t){.buf = &(ngx_buf_t){.last_buf = 1}});
    ngx_http_finalize_request(r, NGX_HTTP_OK);
}

static ngx_int_t pathological_handler(ngx_http_request_t *r) {
    u_char disconnect_qs[] = "disconnect";
    ngx_str_t disconnect_param;

    if (ngx_http_arg(r, disconnect_qs, sizeof(disconnect_qs) - 1, &disconnect_param) != NGX_OK) {
        disconnect_param = (ngx_str_t) ngx_string("false");
    }

    if (ngx_strncmp("true", disconnect_param.data, disconnect_param.len) == 0)
      return NGX_HTTP_CLOSE;
    else if (ngx_strncmp("false", disconnect_param.data, disconnect_param.len) != 0)
      return NGX_DECLINED;

    u_char status_qs[] = "status";
    ngx_str_t status_prm;

    // Get the query string parameter "status"
    if (ngx_http_arg(r, status_qs, sizeof(status_qs) - 1, &status_prm) != NGX_OK) {
      r->headers_out.status = NGX_HTTP_OK; // Default status if parameter is not present
    } else {
      // Convert the parameter to an integer
      ngx_int_t status_code = ngx_atoi(status_prm.data, status_prm.len);
      if (status_code == NGX_ERROR) {
          status_code = NGX_HTTP_BAD_REQUEST;  // Use 400 Bad Request if conversion fails
      }

      // Set the response status code
      r->headers_out.status = status_code;
    }

    u_char malhdr_qs[] = "malformed-header";
    ngx_str_t malhdr_prm;

    // Get the query string parameter "malformed-header"
    if (ngx_http_arg(r, malhdr_qs, sizeof(malhdr_qs) - 1, &malhdr_prm) == NGX_OK) {
      // compare the querystring to all cases
      if (ngx_strncmp(malhdr_prm.data, "spaces-in-header-name", malhdr_prm.len) == 0) {
          set_malformed_header(r, "Spaces In Header Name", "This header name contains spaces");

      } else if (ngx_strncmp(malhdr_prm.data, "trailing-whitespace", malhdr_prm.len) == 0) {
          set_malformed_header(r, "TrailingWhitespace ", "This header has trailing whitespace");

      } else if (ngx_strncmp(malhdr_prm.data, "header-injection", malhdr_prm.len) == 0) {
          set_malformed_header(r, "HeaderInjection\r\nInjected-Header", "This header contains an injection");

      } else if (ngx_strncmp(malhdr_prm.data, "non-printable-chars", malhdr_prm.len) == 0) {
          char header_value[] = "NonPrintableChars\x01\x02";
          set_malformed_header(r, "NonPrintableChars", header_value);

      } else if (ngx_strncmp(malhdr_prm.data, "missing-value", malhdr_prm.len) == 0) {
          set_malformed_header(r, "MissingValue", "");

      } else if (ngx_strncmp(malhdr_prm.data, "multiple-colons", malhdr_prm.len) == 0) {
          set_malformed_header(r, "MultipleColons", "Value1: Value2");

      } else if (ngx_strncmp(malhdr_prm.data, "invalid-encoding", malhdr_prm.len) == 0) {
          char header_value[] = "InvalidEncoding\xc3\x28";
          set_malformed_header(r, "InvalidEncoding", header_value);

      } else if (ngx_strncmp(malhdr_prm.data, "oversized-headers", malhdr_prm.len) == 0) {
          char header_value[9000];
          memset(header_value, 'X', 8999);
          header_value[8999] = '\0';
          set_malformed_header(r, "OversizedHeader", header_value);

      } else if (ngx_strncmp(malhdr_prm.data, "duplicate-headers", malhdr_prm.len) == 0) {
          set_malformed_header(r, "DuplicateHeader", "Value1");
          set_malformed_header(r, "DuplicateHeader", "Value2");

      } else if (ngx_strncmp(malhdr_prm.data, "invalid-characters", malhdr_prm.len) == 0) {
          set_malformed_header(r, "Invalid\xFF""Chars", "This header contains invalid characters");

      // TODO handle missing colon MissingColon[:] Value
      } else {
          return NGX_DECLINED;
      }
    }

    u_char delay_qs[] = "delay";
    ngx_str_t delay_param;

    // Get the query string parameter "delay"
    if (ngx_http_arg(r, delay_qs, sizeof(delay_qs) - 1, &delay_param) != NGX_OK) {
        delay_param.len = 0;
        delay_param.data = (u_char *)"0";
    }

    // Convert the parameter to an integer
    ngx_int_t delay = ngx_atoi(delay_param.data, delay_param.len);
    if (delay == NGX_ERROR) {
        delay = 0;
    }

    ngx_event_t *ev = ngx_pcalloc(r->pool, sizeof(ngx_event_t));
    if (ev == NULL) {
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ev->data = r;
    ev->handler = final_handler;
    ev->log = r->connection->log;

    ngx_add_timer(ev, delay * 1000);

    r->main->count++;  // Increase the main request counter

    // no body
    r->headers_out.content_length_n = 0;

    return NGX_DONE;
}

static char* ngx_pathological_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = pathological_handler;

    return NGX_CONF_OK;
}

ngx_module_t pathological_module = {
    NGX_MODULE_V1,
    &(ngx_http_module_t){0},
   (ngx_command_t[]){
      {
        .name = ngx_string("pathological"),
        .type = NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        .set = ngx_pathological_set,
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
