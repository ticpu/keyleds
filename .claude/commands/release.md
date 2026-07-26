Cut a release of keyleds. Never start this process without explicit instruction — "commit the fix" is not a release request.

The version is written only in the top-level `CMakeLists.txt`. `debian/changelog` and `keyleds.spec` carry their own copies, both written by a script; never edit either by hand.

If $ARGUMENTS names a version or bump level (patch/minor/major), use it; otherwise ask which bump is wanted before touching anything.

1. Preflight: on `main`, latest CI green (`gh run list --branch main --limit 1`), and the working tree clean apart from `CHANGELOG.rst` if $ARGUMENTS says the section was written ahead of time.
2. Edit the version in `project (keyleds VERSION X.Y.Z ...)` in `CMakeLists.txt`.
3. `packaging/sync-version.sh` — writes the `keyleds.spec` field and a new `debian/changelog` stanza.
4. Add the release section to `CHANGELOG.rst`, or check the existing one if it was written ahead of time. Features, fixes and behaviour changes for someone not following development; no commit lists or hashes. Describe the change relative to the **previous release**, not to intermediate branch state — a bug introduced and fixed since the last tag never existed as far as the reader is concerned. This text is the only copy: the tag message and the GitHub release body are both taken from it.
5. `cmake -S . -B build -DWITH_TESTS=ON && cmake --build build -j$(nproc) && ctest --test-dir build --output-on-failure`
6. `packaging/build-deb.sh` and `packaging/build-rpm.sh` — both must produce packages at the new version. They refuse to run if step 2 and step 3 disagree.
7. Commit as `release: vX.Y.Z`, staging `CMakeLists.txt`, `keyleds.spec`, `debian/changelog` and `CHANGELOG.rst` explicitly.
8. `git push`, then WAIT for CI to pass on main (`gh run watch`).
9. `git tag -as vX.Y.Z -m "$(packaging/changelog-section.sh X.Y.Z)"` — the tag message is the `CHANGELOG.rst` section, and the release workflow publishes it as the release body. Never retype it.
10. `git push --tags`, then WAIT for the Release workflow to finish (`gh run watch`). It refuses to publish if the tag and `CMakeLists.txt` disagree.
11. Update the AUR package. AUR commits get no Co-Authored-By trailer.
