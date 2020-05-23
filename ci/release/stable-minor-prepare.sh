#!/bin/bash

set -eu
set -o pipefail

function set_cmake_version {
    sed -i "s/\(set(PROJECT_VERSION \"\).*\")/\1${1}\")/g" CMakeLists.txt
}

# We start from master (that has the beta tag).
git checkout master

# Something like: disman@0.5XX.0-beta.0
LAST_TAG=$(git describe --abbrev=0)
CURRENT_BETA_VERSION=$(echo $LAST_TAG | sed -e 's,disman@\(\),\1,g')

echo "Current beta version is: $CURRENT_BETA_VERSION"

# We always release from stable branch.
BRANCH_NAME="Plasma/$(echo $CURRENT_BETA_VERSION | sed -e 's,^\w.\(\w\)\(\w*\).*,\1.\2,g')"
echo "Stable branch name: $BRANCH_NAME"
git checkout $BRANCH_NAME

# This updates version number from beta to its release
RELEASE_VERSION=$(semver -i minor $CURRENT_BETA_VERSION)

# The CMake project version is the same as the SemVer version.
CMAKE_VERSION=$RELEASE_VERSION

echo "Next SemVer version: '${RELEASE_VERSION}' Corresponding CMake project version: '${CMAKE_VERSION}'"

# This creates the changelog.
standard-version -t disman\@ --skip.commit true --skip.tag true --preMajor --release-as $RELEASE_VERSION

# Set CMake version.
set_cmake_version $CMAKE_VERSION

# Now we have all changes ready.
git add CMakeLists.txt CHANGELOG.md

# Commit and tag it.
git commit -m "build: create minor release ${RELEASE_VERSION}" -m "Update changelog and raise CMake project version to ${CMAKE_VERSION}."
git tag -a "disman@${RELEASE_VERSION}" -m "Create minor release ${RELEASE_VERSION}."

# Go back to master branch and update changelog.
git checkout master
git checkout $BRANCH_NAME CHANGELOG.md
git commit -m "docs: update changelog" -m "Update changelog from branch $BRANCH_NAME at minor release ${RELEASE_VERSION}."

echo "Changes applied. Check integrity of master and $BRANCH_NAME branches. Then issue 'git push --follow-tags' on both."
