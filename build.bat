javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java
javah -jni JLibTorrent

REM :TODO: add include paths for boost and friends.

cl -I"c:\program files\java\jdk1.6.0_11\include" -I"c:\program files\java\jdk1.6.0_11\include\win32" -I"." -MT -LD ./src/main/cpp/jnltorrentjnilib.cpp -Fejnilibtorrent.dll  