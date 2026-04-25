target="$1"

if [ "$(basename "$PWD")" != "microbox" ]; then
    echo "This script should be executed from the root of the repository."
    exit 1
fi

if [ -z "$target" ]; then
    echo "Usage: $0 <target>"
    echo "target should be either 'minima', 'r4wifi', or 'esp32'."
    exit 1
fi

if [ "$target" != "esp32" ] && [ "$target" != "minima" ] && [ "$target" != "r4wifi" ]; then
    echo "Invalid target: $target"
    echo "target should be either 'minima', 'r4wifi', or 'esp32'."
    exit 1
fi

if [ "$target" == "minima" ]; then
set -x
  cp scripts/sketch/sketch-minima.yaml sketch.yaml
set +x
elif [ "$target" == "r4wifi" ]; then
set -x
  cp scripts/sketch/sketch-unor4wifi.yaml sketch.yaml
set +x
else
set -x
  cp scripts/sketch/sketch-esp32.yaml sketch.yaml
set +x
fi
