#!/bin/bash

# generate-hashes.sh - Simple vibe coded script to generate SHA256 hashes for files
# Usage: ./scripts/generate-hashes.sh "*.dll,*.zip"

set -e

echo "🔍 Generating SHA256 hashes for verification..."
echo ""

IFS=',' read -ra PATTERNS <<< "$1"

for pattern in "${PATTERNS[@]}"; do
  pattern=$(echo "$pattern" | xargs)  # Trim whitespace
  
  # Use shell globbing instead of find
  echo "📁 Files matching $pattern :"
  
  # Expand the pattern (shell handles wildcards)
  for file in $pattern; do
    if [[ -f "$file" ]]; then
      filename=$(basename "$file")
      hash=$(sha256sum "$file" | cut -d' ' -f1)
      
      echo ""
      echo "╔═════════════════════════════════════════════════════════════════════════╗"
      echo "║                               FILE HASH                                 ║"
      echo "╠═════════════════════════════════════════════════════════════════════════╣"
      echo "║ File: $filename"
    #   echo "║ Path: $file"
      echo "║ sha256:$hash"
      echo "╚═════════════════════════════════════════════════════════════════════════╝"
      echo ""
      
      safe_name=$(echo "$filename" | sed 's/[^a-zA-Z0-9_]/_/g' | tr '[:upper:]' '[:lower:]' | sed 's/\./_/g')
      echo "${safe_name}_sha256=$hash" >> "$GITHUB_OUTPUT"
    fi
  done
done

# Print summary
echo "📊 Hash Generation Complete!"