# ReactESP Release Process

## Prerequisites

- [bump2version](https://pypi.org/project/bump2version/) (`pip install bump2version`)
- [Doxygen](https://www.doxygen.nl/)
- [Graphviz](https://graphviz.org/)
- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html)

## Steps

1. Create a new local branch `release_x.y.z` from `master`.

2. Bump the version with bumpversion:

       bumpversion minor   # for minor release (x.Y.z)
       bumpversion patch   # for patch release (x.y.Z)
       bumpversion major   # for major release (X.y.z)

   This updates `VERSION`, `library.json`, `library.properties`, and `Doxyfile`,
   and creates a commit automatically.

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
