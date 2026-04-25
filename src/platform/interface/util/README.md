# Utils

This module contains implementations of the common operations on the things
defined in `interface/`.

The idea is that if the interface defines some struct
(e.g. `Color` enumeration), we can have a utility that allows for converting this
color directly to string. This operation can be performed on the `Color` interface
alone and so it makes sense to put it here.
