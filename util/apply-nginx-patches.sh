#!/usr/bin/env bash

set -euo pipefail

nginx_source="$(cd "${1:?Usage: apply-nginx-patches.sh <nginx-source>}" && pwd)"
module_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

while read -r feature header patch_file version; do
    version="${version:-1}"
    if grep -q "^#define $feature  *$version$" "$nginx_source/$header"; then
        continue
    fi

    if grep -q "^#define $feature " "$nginx_source/$header"; then
        echo "Incompatible $feature; apply patches to a fresh nginx source tree" >&2
        exit 1
    fi

    patch --batch --forward --fuzz=0 -d "$nginx_source" -p1 \
        < "$module_root/patches/$patch_file"
done < "$module_root/patches/series"
