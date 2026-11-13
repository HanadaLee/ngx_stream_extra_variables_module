
/*
 * Copyright (C) Jonathan Kolb
 * Copyright (C) Hanada
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <nginx.h>


#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_START_MSEC                       10
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECTED_MSEC                   11
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_MSEC                  12
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_END_MSEC                         13

#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECT_TIME                     20
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_TIME                  21
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_SESSION_TIME                     22

#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED                   30
#define NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_SENT                       31


static ngx_int_t ngx_stream_add_extra_variables(ngx_conf_t *cf);

static ngx_int_t ngx_stream_extra_variable_dollar(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_sec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_hextime(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_stream_extra_variable_connection_established_msec(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_session_created_msec(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_stream_extra_variable_hostname_uppercase(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_hostname_lowercase(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_time_rfc1123(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_stream_extra_variable_upstream_addr(
    ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_upstream_tries(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#if (NGX_RESTY_EXT)
static ngx_int_t ngx_stream_extra_variable_upstream_status(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#endif
#if (NGX_STREAM_UPSTREAM_TIMING)
static ngx_int_t ngx_stream_extra_variable_upstream_multi_msec(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_upstream_single_msec(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#endif
#if 0
static ngx_int_t ngx_stream_extra_variable_upstream_multi_time(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#endif
static ngx_int_t ngx_stream_extra_variable_upstream_single_time(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#if 0
static ngx_int_t ngx_stream_extra_variable_upstream_multi_length(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
#endif
static ngx_int_t ngx_stream_extra_variable_upstream_single_length(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_stream_extra_variable_upstream_total_length(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);


static ngx_stream_module_t  ngx_stream_extra_variables_module_ctx = {
    ngx_stream_add_extra_variables,             /* preconfiguration */
    NULL,                                       /* postconfiguration */

    NULL,                                       /* create main conf */
    NULL,                                       /* init main conf */

    NULL,                                       /* create server conf */
    NULL                                        /* merge server conf */
};


ngx_module_t  ngx_stream_extra_variables_module = {
    NGX_MODULE_V1,
    &ngx_stream_extra_variables_module_ctx,       /* module context */
    NULL,                                       /* module directives */
    NGX_STREAM_MODULE,                            /* module type */
    NULL,                                       /* init master */
    NULL,                                       /* init module */
    NULL,                                       /* init process */
    NULL,                                       /* init thread */
    NULL,                                       /* exit thread */
    NULL,                                       /* exit process */
    NULL,                                       /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_stream_variable_t  ngx_stream_extra_variables[] = {
    { ngx_string("dollar"), NULL,
      ngx_stream_extra_variable_dollar,
      0, 0, 0 },

    { ngx_string("sec"), NULL,
      ngx_stream_extra_variable_sec,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("hextime"), NULL,
      ngx_stream_extra_variable_hextime,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("connection_established_msec"), NULL,
      ngx_stream_extra_variable_connection_established_msec,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("session_created_msec"), NULL,
      ngx_stream_extra_variable_session_created_msec,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("hostname_uppercase"), NULL,
      ngx_stream_extra_variable_hostname_uppercase,
      0, 0, 0 },

    { ngx_string("hostname_lowercase"), NULL,
      ngx_stream_extra_variable_hostname_lowercase,
      0, 0, 0 },

    { ngx_string("time_rfc1123"), NULL,
      ngx_stream_extra_variable_time_rfc1123,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_last_addr"), NULL,
      ngx_stream_extra_variable_upstream_addr,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_tries"), NULL,
      ngx_stream_extra_variable_upstream_tries,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

#if (NGX_RESTY_EXT)

    { ngx_string("upstream_last_status"), NULL,
      ngx_stream_extra_variable_upstream_status,
      0, NGX_STREAM_VAR_NOCACHEABLE, 0 },

#endif

#if (NGX_STREAM_UPSTREAM_TIMING)

    { ngx_string("upstream_start_msec"), NULL,
      ngx_stream_extra_variable_upstream_multi_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_START_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_last_start_msec"), NULL,
      ngx_stream_extra_variable_upstream_single_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_START_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_connected_msec"), NULL,
      ngx_stream_extra_variable_upstream_multi_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECTED_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_last_connected_msec"), NULL,
      ngx_stream_extra_variable_upstream_single_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECTED_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_first_byte_msec"), NULL,
      ngx_stream_extra_variable_upstream_multi_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_last_first_byte_msec"), NULL,
      ngx_stream_extra_variable_upstream_single_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_end_msec"), NULL,
      ngx_stream_extra_variable_upstream_multi_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_END_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_last_end_msec"), NULL,
      ngx_stream_extra_variable_upstream_single_msec,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_END_MSEC,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

#if 0
    { ngx_string("upstream_connect_time"), NULL,
      ngx_stream_extra_variable_upstream_multi_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECT_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_string("upstream_last_connect_time"), NULL,
      ngx_stream_extra_variable_upstream_single_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECT_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

#if 0
    { ngx_string("upstream_first_byte_time"), NULL,
      ngx_stream_extra_variable_upstream_multi_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_string("upstream_last_first_byte_time"), NULL,
      ngx_stream_extra_variable_upstream_single_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

#if 0
    { ngx_string("upstream_session_time"), NULL,
      ngx_stream_extra_variable_upstream_multi_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_SESSION_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_string("upstream_last_session_time"), NULL,
      ngx_stream_extra_variable_upstream_single_time,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_SESSION_TIME,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

#if 0
    { ngx_string("upstream_bytes_received"), NULL,
      ngx_stream_extra_variable_upstream_multi_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_string("upstream_last_bytes_received"), NULL,
      ngx_stream_extra_variable_upstream_single_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

#if 0
    { ngx_string("upstream_bytes_sent"), NULL,
      ngx_stream_extra_variable_upstream_multi_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_SENT,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_string("upstream_last_bytes_sent"), NULL,
      ngx_stream_extra_variable_upstream_single_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_SENT,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_total_bytes_received"), NULL,
      ngx_stream_extra_variable_upstream_total_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_total_bytes_sent"), NULL,
      ngx_stream_extra_variable_upstream_total_length,
      NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_SENT,
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_int_t
ngx_stream_add_extra_variables(ngx_conf_t *cf)
{
    ngx_stream_variable_t  *var, *v;

    for (v = ngx_stream_extra_variables; v->name.len; v++) {
        var = ngx_stream_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_dollar(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    v->len = 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) "$";

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_sec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T", tp->sec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_hextime(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;
    uint32_t     timestamp;

    p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();
    timestamp = (uint32_t) tp->sec;

    v->len = ngx_sprintf(p, "%xi", timestamp) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_connection_established_msec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char          *p;
    ngx_time_t      *tp;
    ngx_msec_t       ms;

    p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    ms = (ngx_msec_t) (tp->sec * 1000 + tp->msec
                       + s->connection->start_time - ngx_current_msec);

    v->len = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_session_created_msec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char         *p;

    p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%T.%03M", s->start_sec, s->start_msec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_hostname_uppercase(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char *p, *dst;

    v->len = ngx_cycle->hostname.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(s->connection->pool, v->len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    dst = v->data;
    p = ngx_cycle->hostname.data;

    for (size_t i = 0; i < v->len; i++) {
        dst[i] = ngx_toupper(p[i]);
    }

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_hostname_lowercase(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char *p, *dst;

    v->len = ngx_cycle->hostname.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(s->connection->pool, v->len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    dst = v->data;
    p = ngx_cycle->hostname.data;

    for (size_t i = 0; i < v->len; i++) {
        dst[i] = ngx_tolower(p[i]);
    }

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_time_rfc1123(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(s->connection->pool, ngx_cached_http_time.len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(p, ngx_cached_http_time.data, ngx_cached_http_time.len);

    v->len = ngx_cached_http_time.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_upstream_addr(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_stream_upstream_state_t  *state;

    if (s->upstream && s->upstream->state && s->upstream->state->peer) {
        v->len = s->upstream->state->peer->len;
        v->data = ngx_pnalloc(s->connection->pool, v->len);
        if (v->data == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(v->data, s->upstream->state->peer->data, v->len);

        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

        return NGX_OK;
    }

    if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    state = s->upstream_states->elts;
    state = &state[s->upstream_states->nelts - 1];

    if (state->peer) {
        v->len = state->peer->len;
        v->data = ngx_pnalloc(s->connection->pool, v->len);

        if (v->data == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(v->data, state->peer->data, v->len);

    } else {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_upstream_tries(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char     *p;
    ngx_uint_t  tries;

    if (s->upstream_states && s->upstream_states->nelts > 0) {
        tries = s->upstream_states->nelts;

    } else if (s->upstream && s->upstream->state) {
        tries = 1;

    } else {
        v->not_found = 1;
        return NGX_OK;
    }

    p = ngx_pnalloc(s->connection->pool, NGX_INT_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;
    v->len = ngx_sprintf(p, "%ui", tries) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


#if (NGX_RESTY_EXT)

static ngx_int_t
ngx_stream_extra_variable_upstream_status(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_stream_upstream_state_t  *state;

    if (s->upstream && s->upstream->state && s->upstream->state->status) {
        state = s->upstream->state;

    } else {
        if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
            v->not_found = 1;
            return NGX_OK;
        }

        state = s->upstream_states->elts;
        state = &state[s->upstream_states->nelts - 1];
    }

    if (!state->status) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = ngx_pnalloc(s->connection->pool, NGX_INT_T_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(v->data, "%ui", state->status) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

#endif


#if (NGX_STREAM_UPSTREAM_TIMING)

static ngx_int_t
ngx_stream_extra_variable_upstream_multi_msec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char                       *p;
    ngx_time_t                   *tp;
    size_t                        len;
    ngx_uint_t                    i;
    ngx_msec_t                    ms;
    ngx_stream_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = s->upstream_states->nelts * (NGX_TIME_T_LEN + 4 + 2);

    p = ngx_pnalloc(s->connection->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = s->upstream_states->elts;

    tp = ngx_timeofday();

    for ( ;; ) {

        switch (data) {

        case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_START_MSEC:
            ms = state[i].start_msec;
            break;

        case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECTED_MSEC:
            if (state[i].connect_time == (ngx_msec_t) -1) {
                ms = (ngx_msec_t) -1;

            } else {
                ms = (ngx_msec_t) (state[i].start_msec + state[i].connect_time);
            }
            break;

        case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_MSEC:
            if (state[i].first_byte_time == (ngx_msec_t) -1) {
                ms = (ngx_msec_t) -1;

            } else {
                ms = state[i].start_msec + state[i].first_byte_time;
            }
            break;

        case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_END_MSEC:
            if (state[i].response_time == (ngx_msec_t) -1) {
                ms = (ngx_msec_t) -1;

            } else {
                ms = state[i].start_msec + state[i].response_time;
            }
            break;

        default:
            v->not_found = 1;
            return NGX_OK;
        }

        if (ms != (ngx_msec_t) -1) {
            ms = (ngx_msec_t)
                (tp->sec * 1000 + tp->msec + ms - ngx_current_msec);
            p = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000);

        } else {
            *p++ = '-';
        }

        if (++i == s->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == s->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_upstream_single_msec(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char                       *p;
    ngx_time_t                   *tp;
    ngx_msec_t                    ms;
    ngx_stream_upstream_state_t  *state;

    if (s->upstream && s->upstream->state) {
        state = s->upstream->state;

    } else {
        if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
            v->not_found = 1;
            return NGX_OK;
        }

        state = s->upstream_states->elts;
        state = &state[s->upstream_states->nelts - 1];
    }

    tp = ngx_timeofday();

    switch (data) {

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_START_MSEC:
        ms = state->start_msec;
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECTED_MSEC:
        if (state->connect_time == (ngx_msec_t) -1) {
            ms = (ngx_msec_t) -1;

        } else {
            ms = (ngx_msec_t) (state->start_msec + state->connect_time);
        }
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_MSEC:
        if (state->first_byte_time == (ngx_msec_t) -1) {
            ms = (ngx_msec_t) -1;

        } else {
            ms = (ngx_msec_t) (state->start_msec + state->first_byte_time);
        }
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_END_MSEC:
        if (state->response_time == (ngx_msec_t) -1) {
            ms = (ngx_msec_t) -1;

        } else {
            ms = (ngx_msec_t) (state->start_msec + state->response_time);
        }
        break;

    default:
        v->not_found = 1;
        return NGX_OK;
    }

    if (ms != (ngx_msec_t) -1) {
        ms = (ngx_msec_t)(tp->sec * 1000 + tp->msec + ms - ngx_current_msec);

        p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN + 4);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->data = p;
        p = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000);
        v->len = p - v->data;

    } else {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

#endif


#if 0
static ngx_int_t
ngx_stream_extra_variable_upstream_multi_time(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    return NGX_OK;
}
#endif


static ngx_int_t
ngx_stream_extra_variable_upstream_single_time(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char                       *p;
    ngx_msec_int_t                ms;
    ngx_stream_upstream_state_t  *state;

    if (s->upstream && s->upstream->state) {
        state = s->upstream->state;

    } else {
        if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
            v->not_found = 1;
            return NGX_OK;
        }

        state = s->upstream_states->elts;
        state = &state[s->upstream_states->nelts - 1];
    }

    switch (data) {

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_CONNECT_TIME:
        ms = state->connect_time;
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_FIRST_BYTE_TIME:
        ms = state->first_byte_time;
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_SESSION_TIME:
        ms = state->response_time;
        break;

    default:
        v->not_found = 1;
        return NGX_OK;
    }

    if (ms != -1) {
        ms = ngx_max(ms, 0);

        p = ngx_pnalloc(s->connection->pool, NGX_TIME_T_LEN + 4);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->data = p;
        p = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000);
        v->len = p - v->data;

    } else {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


#if 0
static ngx_int_t
ngx_stream_extra_variable_upstream_multi_length(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    return NGX_OK;
}
#endif


static ngx_int_t
ngx_stream_extra_variable_upstream_single_length(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_stream_upstream_state_t  *state;
    u_char                       *p;

    if (s->upstream && s->upstream->state) {
        state = s->upstream->state;

    } else {
        if (s->upstream_states == NULL || s->upstream_states->nelts == 0) {
            v->not_found = 1;
            return NGX_OK;
        }

        state = s->upstream_states->elts;
        state = &state[s->upstream_states->nelts - 1];
    }

    p = ngx_pnalloc(s->connection->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    switch (data) {

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED:
        p = ngx_sprintf(p, "%O", state->bytes_received);
        break;

    case NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_SENT:
        p = ngx_sprintf(p, "%O", state->bytes_sent);
        break;

    default:
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = p - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_stream_extra_variable_upstream_total_length(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char                       *p;
    ngx_uint_t                    i;
    off_t                         total;
    ngx_stream_upstream_state_t  *state;

    if (s->upstream_states && s->upstream_states->nelts > 0) {
        state = s->upstream_states->elts;
        total = 0;

        for (i = 0; i < s->upstream_states->nelts; i++) {
            if (data == NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED) {
                total += state[i].bytes_received;

            } else {
                total += state[i].bytes_sent;
            }
        }

    } else if (s->upstream && s->upstream->state) {
        state = s->upstream->state;

        if (data == NGX_STREAM_EXTRA_VARIABLE_UPSTREAM_BYTES_RECEIVED) {
            total = state->bytes_received;

        } else {
            total = state->bytes_sent;
        }

    } else {
        v->not_found = 1;
        return NGX_OK;
    }

    p = ngx_pnalloc(s->connection->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;
    v->len = ngx_sprintf(p, "%O", total) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}
