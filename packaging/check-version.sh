#!/bin/bash
# Fail when packaging metadata has drifted from the version in CMakeLists.txt.
# dpkg and rpm each take the package version from their own file, so a bump that
# misses one produces a package whose version contradicts the program inside it.
set -euo pipefail

cd "$(dirname "$0")/.."

version="$(grep -Po 'keyleds VERSION \K[0-9.]+' CMakeLists.txt)"
deb="$(head -n1 debian/changelog | grep -Po '\(\K[0-9][^)]*')"
rpm="$(grep -Po '^Version:\s*\K\S+' keyleds.spec)"

status=0
if [ "${version}" != "${deb%-*}" ]; then
    echo "debian/changelog says ${deb}, CMakeLists.txt says ${version}" >&2
    status=1
fi
if [ "${version}" != "${rpm}" ]; then
    echo "keyleds.spec says ${rpm}, CMakeLists.txt says ${version}" >&2
    status=1
fi
if [ "${status}" != 0 ]; then
    echo "run packaging/sync-version.sh" >&2
    exit 1
fi
