
REM FIXME: There is a bug in class generation with: javah -jni JLibTorrent

javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java

javah -jni JLibTorrent


cl /Od /D "WIN32" /D "_DEBUG" /D "_USRDLL" /D "_WINDLL" -I"c:\program files\java\jdk1.6.0_11\include" -I"c:\program files\java\jdk1.6.0_11\include\win32" -I"c:\boost" -I "../../libtorrent/libtorrent/include" -I "../../libtorrent/libtorrent/zlib" -I"." -LD /Gm /EHsc /RTC1 /MDd /Fo"Debug\\" /W3 /nologo /c /ZI /TP /errorReport:prompt ./src/main/cpp/jnltorrentjnilib.cpp -Fejnilibtorrent.dll  