#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

// Configuration structure
typedef struct {
    ngx_msec_t delay; // delay in milliseconds
} ngx_stream_post_handshake_delay_srv_conf_t;

// Context structure
typedef struct {
    ngx_event_t delay_event;
    ngx_uint_t delayed; // Flag indicating whether the delay has been applied
} ngx_stream_post_handshake_delay_ctx_t;

// Function prototypes
static void *ngx_stream_post_handshake_delay_create_srv_conf(ngx_conf_t *cf);
static char *ngx_stream_post_handshake_delay_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_stream_post_handshake_delay_handler(ngx_stream_session_t *s);
static void ngx_stream_post_handshake_delay_timeout_handler(ngx_event_t *ev);
static ngx_int_t ngx_stream_post_handshake_delay_init(ngx_conf_t *cf);

// Module directives
static ngx_command_t ngx_stream_post_handshake_delay_commands[] = {
    {
        ngx_string("post_handshake_delay"),
        NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_msec_slot,
        NGX_STREAM_SRV_CONF_OFFSET,
        offsetof(ngx_stream_post_handshake_delay_srv_conf_t, delay),
        NULL
    },
    ngx_null_command
};

// Module context
static ngx_stream_module_t ngx_stream_post_handshake_delay_module_ctx = {
    NULL,                                      // preconfiguration
    ngx_stream_post_handshake_delay_init,      // postconfiguration
    NULL,                                      // create main configuration
    NULL,                                      // init main configuration
    ngx_stream_post_handshake_delay_create_srv_conf,  // create server configuration
    ngx_stream_post_handshake_delay_merge_srv_conf    // merge server configuration
};

// Module definition
ngx_module_t pathological_module = {
    NGX_MODULE_V1,
    &ngx_stream_post_handshake_delay_module_ctx,      // module context
    ngx_stream_post_handshake_delay_commands,         // module directives
    NGX_STREAM_MODULE,                                // module type
    NULL,                                             // init master
    NULL,                                             // init module
    NULL,                                             // init process
    NULL,                                             // init thread
    NULL,                                             // exit thread
    NULL,                                             // exit process
    NULL,                                             // exit master
    NGX_MODULE_V1_PADDING
};

// Create server configuration
static void *
ngx_stream_post_handshake_delay_create_srv_conf(ngx_conf_t *cf)
{
    ngx_stream_post_handshake_delay_srv_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_stream_post_handshake_delay_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->delay = NGX_CONF_UNSET_MSEC;

    return conf;
}

// Merge server configuration
static char *
ngx_stream_post_handshake_delay_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_stream_post_handshake_delay_srv_conf_t *prev = parent;
    ngx_stream_post_handshake_delay_srv_conf_t *conf = child;

    ngx_conf_merge_msec_value(conf->delay, prev->delay, 0);

    return NGX_CONF_OK;
}

// Handler function
static ngx_int_t
ngx_stream_post_handshake_delay_handler(ngx_stream_session_t *s)
{
    ngx_stream_post_handshake_delay_srv_conf_t *conf;
    conf = ngx_stream_get_module_srv_conf(s, pathological_module);

    if (conf->delay > 0) {
        // Get or create context
        ngx_stream_post_handshake_delay_ctx_t *ctx;
        ctx = ngx_stream_get_module_ctx(s, pathological_module);

        if (ctx == NULL) {
            // Allocate and set context
            ctx = ngx_pcalloc(s->connection->pool, sizeof(ngx_stream_post_handshake_delay_ctx_t));
            if (ctx == NULL) {
                return NGX_ERROR;
            }

            ngx_stream_set_ctx(s, ctx, pathological_module);
        }

        if (!ctx->delayed) {
            ngx_log_debug1(NGX_LOG_DEBUG_STREAM, s->connection->log, 0,
                           "post_handshake_delay: delaying for %M ms", conf->delay);

            ctx->delayed = 1;

            // Initialize the delay event
            ngx_event_t *ev = &ctx->delay_event;
            ev->handler = ngx_stream_post_handshake_delay_timeout_handler;
            ev->data = s;
            ev->log = s->connection->log;
            ev->cancelable = 1;

            // Add timer
            ngx_add_timer(ev, conf->delay);

            // Suspend processing
            return NGX_DONE;
        }
    }

    return NGX_DECLINED; // Continue processing
}

// Timeout handler
static void
ngx_stream_post_handshake_delay_timeout_handler(ngx_event_t *ev)
{
    ngx_stream_session_t *s = ev->data;
    ngx_connection_t *c = s->connection;

    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "post_handshake_delay: delay expired, resuming session");

    // Continue processing the session
    ngx_stream_core_run_phases(s);
}

// Module initialization function
static ngx_int_t
ngx_stream_post_handshake_delay_init(ngx_conf_t *cf)
{
    ngx_stream_core_main_conf_t  *cmcf;
    ngx_stream_handler_pt        *h;

    cmcf = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_STREAM_PREREAD_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_stream_post_handshake_delay_handler;

    return NGX_OK;
}
