#!/bin/bash
# Print one release's section from CHANGELOG.rst, for use as the annotated tag
# message. The release workflow then publishes that tag message as the release
# body, so all three carry the same text without it being written three times.
set -euo pipefail

cd "$(dirname "$0")/.."

version="${1:?usage: changelog-section.sh VERSION}"

section="$(awk -v ver="${version}" '
    /^\*+$/ {
        if (state == 2) { exit }            # rule opening the next section
        if (state == 1) { state = 2; next } # rule closing our heading
        rule = 1; next
    }
    rule && $1 == ver { state = 1; rule = 0; next }
    { rule = 0 }
    state == 2 { print }
' CHANGELOG.rst)"

if [ -z "${section}" ]; then
    echo "no section for ${version} in CHANGELOG.rst" >&2
    exit 1
fi

# Trim the blank lines the rst layout leaves at either end
printf '%s\n' "${section}" | sed '/./,$!d' | tac | sed '/./,$!d' | tac
