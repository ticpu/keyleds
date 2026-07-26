#!/bin/bash
# Propagate the version declared in CMakeLists.txt into the packaging metadata.
# rpm and dpkg each insist on carrying their own copy; this is what keeps them
# from drifting, and the build scripts refuse to run when they have.
set -euo pipefail

cd "$(dirname "$0")/.."

version="$(grep -Po 'keyleds VERSION \K[0-9.]+' CMakeLists.txt)"
revision="${1:-1}"

# rpm holds a plain field, so it can simply be overwritten
sed -i "s/^Version:.*/Version: ${version}/" keyleds.spec

# dpkg takes the version from the newest changelog entry, so a release needs a
# new stanza rather than an edit. The whitespace here is load-bearing: one space
# either side of the dashes, two before the date.
if head -n1 debian/changelog | grep -q "(${version}-"; then
    echo "debian/changelog already has an entry for ${version}"
else
    {
        printf '%s (%s-%s) unstable; urgency=medium\n\n' keyleds "${version}" "${revision}"
        printf '  * New upstream release; see CHANGELOG.rst.\n\n'
        printf ' -- %s <%s>  %s\n\n' \
            "$(git config user.name)" "$(git config user.email)" "$(date -R)"
        cat debian/changelog
    } > debian/changelog.new
    mv debian/changelog.new debian/changelog
fi

grep -n '^Version:' keyleds.spec
head -n1 debian/changelog
