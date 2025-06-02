#!/bin/bash
# Initialize all submodules
git submodule init

# Shallow clone for json
git submodule update --depth=1 external/json

# Function to handle branch-tracked submodules
setup_branch_submodule() {
  local submodule_path=$1
  local branch=$2
  
  echo "Configuring $submodule_path to track $branch"
  git submodule set-branch --branch "$branch" "$submodule_path"
  git submodule update --remote "$submodule_path"
  
  # Verify branch is checked out
  (cd "$submodule_path" && git checkout "$branch" && git pull)
}

# Configure branch-tracked submodules
setup_branch_submodule external/RLSDK main
setup_branch_submodule external/ModUtils main
setup_branch_submodule external/BMSDK master
setup_branch_submodule external/BakkesmodPluginTemplate master

# Final verification
echo "Submodule status:"
git submodule status