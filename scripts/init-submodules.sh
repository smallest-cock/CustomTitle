#!/bin/bash
set -e

git submodule init

# Shallow clone for json
git submodule update --depth=1 external/json

# Configure branch-tracked submodules
setup_branch_submodule() {
    local path=$1
    local branch=$2
    git submodule set-branch --branch "$branch" "$path"
    git submodule update --remote "$path"
    cd "$path"
    git checkout "$branch"
    git pull
    cd ../..
}

setup_branch_submodule external/RLSDK main
setup_branch_submodule external/ModUtils main
setup_branch_submodule external/BMSDK master
setup_branch_submodule external/BakkesmodPluginTemplate master

git submodule status

# Linux-only: vendored submodules assume a case-insensitive filesystem
# and rely on a couple of MSVC-only implicit casts. Fix both up so the
# tree builds cleanly on a case-sensitive filesystem with clang-cl.
if [[ "$(uname -s)" == "Linux" ]]; then
    echo "==> Fixing BMSDK include casing..."
    python3 scripts/fix_include_casing.py external/BMSDK

    echo "==> Patching imgui_impl_dx11.cpp narrowing casts..."
    PATCH="$(pwd)/external/patches/plugin-template-imgui-fix.patch"

    if git -C external/BakkesmodPluginTemplate apply --reverse --check "$PATCH" &>/dev/null; then
        echo "    already applied, skipping."
    elif git -C external/BakkesmodPluginTemplate apply --check "$PATCH" &>/dev/null; then
        git -C external/BakkesmodPluginTemplate apply "$PATCH"
        echo "    applied patch"
    else
        echo "    ERROR: patch does not apply cleanly." >&2
        exit 1
    fi
fi
