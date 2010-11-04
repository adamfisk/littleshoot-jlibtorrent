#!/usr/bin/env bash

function die()
{
  echo $*
  exit 1
}

mvn install -Dmaven.test.skip=true || die "Could not build JNI lib"
cd ../../lib || die "Could not cd"
rm jnl.tgz 
tar czvf jnl.tgz libjnltorrent.jnilib || die "Could not tgz"
aws -putp littleshoot jnl.tgz || die "Could no upload tgz"

cd -
