#!/bin/bash

# This script is useful for finding large files in the project and identifying
# possible candidates for modularization.

echo "C++ source / header files excluding external libs and font definitons"
git ls-files | grep -e .cpp -e .hpp | grep -v src/lib | grep -v src/fonts | xargs wc -l


