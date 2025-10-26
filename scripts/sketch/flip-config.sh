target="$1"

if [ "$(basename "$PWD")" != "microbox" ]; then
    echo "This script should be executed from the root of the repository."
    exit 1
fi

if [ -z "$target" ]; then
    echo "Usage: $0 <target>"
    echo "target should be either 'minima' or 'r4wifi'."
    exit 1
fi

if [ "$target" != "minima" ] && [ "$target" != "r4wifi" ]; then
    echo "Invalid target: $target"
    echo "target should be either 'minima' or 'r4wifi'."
    exit 1
fi

set -x # Enables command echo-ing
if [ "$target" == "minima" ]; then
  cp scripts/sketch/sketch-minima.yaml sketch.yaml
else
  cp scripts/sketch/sketch-unor4wifi.yaml sketch.yaml
fi
set -x
