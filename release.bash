#!/usr/bin/env bash

mvn install -Dmaven.test.skip=true
cd ../../lib
tar czvf jnl.tgz libjnltorrent.jnilib
aws -putp littleshoot jnl.tgz

cd -
