#!/bin/bash
# Build the RPM package in a container and verify it installs on a clean
# system. Called by `make rpm` and by CI.
set -euo pipefail

release="${RPM_RELEASE:-41}"
outdir="${OUT_DIR:-dist}"
image="keyleds-rpm-${release}"

cd "$(dirname "$0")/.."

packaging/check-version.sh

# Streaming the context avoids shipping build trees and .git into the image
tar --exclude=./build --exclude=./dist --exclude=./.git -cf - . \
    | podman build --build-arg "RELEASE=${release}" -f packaging/Containerfile.rpm \
                   -t "${image}" -

mkdir -p "${outdir}"
rm -f "${outdir}"/*.rpm
cid="$(podman create "${image}")"
trap 'podman rm -f "${cid}" >/dev/null' EXIT
podman cp "${cid}:/rpms/." "${outdir}/"

ls -1 "${outdir}"/*.rpm
