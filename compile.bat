
cl -I "c:\program files\java\jdk1.6.0_11\include" -I"c:\program files\java\jdk1.6.0_11\include\win32" -I"c:\boost" -I "../../libtorrent/libtorrent/include" -I "../../libtorrent/libtorrent/zlib" -I "./build/Release/Headers" -I"." /LD /MT /EHsc /W1  /TP ./src/main/cpp/jnltorrentjnilib.cpp /Fe"jlibtorrent.dll"
