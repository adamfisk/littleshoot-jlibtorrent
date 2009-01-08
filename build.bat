
javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java

javah -verbose -classpath ./src/main/java/ -force -d build/headers/ org.lastbamboo.jni.JLibTorrent

cl -I "c:\program files\java\jdk1.6.0_11\include" -I"c:\program files\java\jdk1.6.0_11\include\win32" -I"c:\boost" -I "../../libtorrent/libtorrent/include" -I "../../libtorrent/libtorrent/zlib" -I "./build/headers" -I"." /LD /MT /EHsc /W1  /TP ./src/main/cpp/jnltorrentjnilib.cpp /Fe"jlibtorrent.dll"
