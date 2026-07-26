#!/bin/bash
# Build the Debian package in a container and verify it installs on a clean
# system. Called by `make deb` and by CI.
set -euo pipefail

suite="${DEB_SUITE:-trixie}"
outdir="${OUT_DIR:-dist}"
image="keyleds-deb-${suite}"

cd "$(dirname "$0")/.."

packaging/check-version.sh

# Streaming the context avoids shipping build trees and .git into the image
tar --exclude=./build --exclude=./dist --exclude=./.git -cf - . \
    | podman build --build-arg "SUITE=${suite}" -f packaging/Containerfile.deb \
                   -t "${image}" -

mkdir -p "${outdir}"
rm -f "${outdir}"/*.deb
cid="$(podman create "${image}")"
trap 'podman rm -f "${cid}" >/dev/null' EXIT
podman cp "${cid}:/debs/." "${outdir}/"

ls -1 "${outdir}"/*.deb
