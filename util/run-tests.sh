#!/usr/bin/env bash

set -euo pipefail

module_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
nginx_source="$module_root/.ci/nginx"
test_output="$module_root/.ci/test-output"
build_jobs="${TEST_BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN)}"
mkdir -p "$test_output"
exec > >(tee "$test_output/run.log") 2>&1

configure_args=(
    --prefix="$nginx_source/test-prefix"
    --with-debug
    --with-http_ssl_module
    --with-http_v2_module
    --with-http_realip_module
    --with-http_sub_module
    --with-http_json_module
    --with-stream
    --with-stream_ssl_module
    --with-stream_realip_module
)

configure_args+=(--add-module="$module_root")

if [[ "${TEST_CORE_PATCH:-enabled}" == enabled ]]; then
    bash "$module_root/util/apply-nginx-patches.sh" "$nginx_source"
fi

cd "$nginx_source"
configure=./configure
[[ -f "$configure" ]] || configure=auto/configure
dynamic_args=("${configure_args[@]}")
dynamic_args[${#dynamic_args[@]}-1]="--add-dynamic-module=$module_root"
if sh "$configure" "${dynamic_args[@]}" > "$test_output/dynamic-rejected.log" 2>&1; then
    echo 'configure unexpectedly accepted a dynamic module'
    exit 1
fi
grep -F 'ngx_stream_extra_variables_module must be built with --add-module' "$test_output/dynamic-rejected.log"

stream_args=("${configure_args[@]}")
for index in "${!stream_args[@]}"; do
    if [[ "${stream_args[index]}" == --with-stream ]]; then
        stream_args[index]=--with-stream=dynamic
    fi
done
if sh "$configure" "${stream_args[@]}" > "$test_output/dynamic-stream-rejected.log" 2>&1; then
    echo 'configure unexpectedly accepted a dynamic Stream subsystem'
    exit 1
fi
grep -F 'requires a static Stream subsystem' "$test_output/dynamic-stream-rejected.log"

sh "$configure" "${configure_args[@]}"
make -j"$build_jobs"

export TEST_NGINX_UPSTREAM_TIMING=0
if grep -q '^#define NGX_STREAM_UPSTREAM_TIMING  *1$' src/stream/ngx_stream_upstream.h; then
    export TEST_NGINX_UPSTREAM_TIMING=1
fi

export TEST_NGINX_UPSTREAM_STATUS=0
if grep -q '^#define NGX_EXT  *1$' objs/ngx_auto_config.h; then
    export TEST_NGINX_UPSTREAM_STATUS=1
fi

export TEST_NGINX_BINARY="$nginx_source/objs/nginx"
export PERL5LIB="$module_root/.ci/nginx-tests/lib"
export TMPDIR="$(mktemp -d /tmp/nginx-module-test.XXXXXX)"
export TEST_NGINX_LEAVE=1
export TEST_NGINX_SERVROOT="$TMPDIR/servroot"
collect_logs() {
    find "$TMPDIR" -type f -name '*.log' -exec cp --parents {} "$test_output/" \;
}
trap collect_logs EXIT
if [[ "$(id -u)" == 0 ]]; then
    export TEST_NGINX_GLOBALS="${TEST_NGINX_GLOBALS:-user root;}"
fi
"$TEST_NGINX_BINARY" -V

cd "$module_root"
shopt -s nullglob
tests=()
for test_file in t/*.t; do
    tests+=("$test_file")
done
if (( ${#tests[@]} == 0 )); then
    echo "No tests selected" >&2
    exit 1
fi
prove -v "${tests[@]}" | tee "$test_output/tests.log"
grep -q '^Result: PASS$' "$test_output/tests.log"
