# ReactESP Release Process

## Prerequisites

- [bump2version](https://pypi.org/project/bump2version/) (`pip install bump2version`)
- [Doxygen](https://www.doxygen.nl/)
- [Graphviz](https://graphviz.org/)
- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html)

## Steps

1. Create a new local branch `release_x.y.z` from `master`.

2. Bump the version with bumpversion. The version scheme uses an optional
   `-alpha` suffix, so bumping requires two steps:

       bumpversion minor   # 3.2.0 → 3.3.0-alpha
       bumpversion release # 3.3.0-alpha → 3.3.0

   Replace `minor` with `patch` or `major` as appropriate.

   Each command updates `VERSION`, `library.json`, `library.properties`, and
   `Doxyfile`, and creates a commit. Squash the two bump commits before
   proceeding:

       git reset --soft HEAD~2
       git commit -m "Bump version: x.y.z-1 → x.y.z"

3. Regenerate the Doxygen documentation:

       scripts/update_autogen.sh

   This removes the old docs, runs Doxygen, and commits the result.

4. Push the branch and create a PR against `master`. Verify everything looks OK.

5. Merge the PR.

6. Create a GitHub release at https://github.com/mairas/ReactESP/releases:
   - Tag: `vx.y.z`
   - Title: `Version x.y.z`
   - Description: list of changes since the previous release

7. Publish to the PlatformIO registry from a clean clone:

       cd /tmp
       git clone https://github.com/mairas/ReactESP.git ReactESP-release
       cd ReactESP-release
       git checkout vx.y.z
       pio pkg publish
       rm -rf /tmp/ReactESP-release
