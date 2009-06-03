#!/bin/sh

export LD_LIBRARY_PATH=./_build_/default/src/libanidb/

./_build_/default/src/anidbfs/anidbfs-update fusetest testuser /storage/ch01/anime/hdtv
