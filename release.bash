#!/usr/bin/env bash

cd ../lib
tar czvf jnl.tgz libjnltorrent.jnilib
aws -putp littleshoot jnl.tgz

cd -
