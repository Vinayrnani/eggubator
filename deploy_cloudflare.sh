#!/bin/bash
# Usage: ./deploy_cloudflare.sh <cloudflare_url>

if [ -z "$1" ]; then
    echo "Usage: $0 <cloudflare_url>"
    exit 1
fi

URL=$1
# Remove trailing slash if present
URL=${URL%/}

echo "Deploying firmware to $URL/update ..."
curl -X POST -F "image=@build/eggubator.ino.bin" "$URL/update"
