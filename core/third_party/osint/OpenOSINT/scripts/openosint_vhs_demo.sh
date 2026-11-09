#!/usr/bin/env bash
# VHS demo driver — streams fixture data line-by-line for a natural terminal effect.
# Called via alias in openosint.tape; never runs live tools so output is reproducible.
# Usage: openosint_vhs_demo.sh username johndoe99
#        openosint_vhs_demo.sh email target@example.com

FIXTURE_DIR="$(cd "$(dirname "$0")/../assets" && pwd)"

stream_file() {
  while IFS= read -r line; do
    echo "$line"
    sleep 0.045
  done < "$1"
}

case "$1" in
  username) stream_file "$FIXTURE_DIR/fixture_username.txt" ;;
  email)    stream_file "$FIXTURE_DIR/fixture_email.txt" ;;
  *)        echo "usage: openosint username|email <target>"; exit 1 ;;
esac
