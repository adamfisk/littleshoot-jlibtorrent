
javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java

javah -verbose -classpath ./src/main/java/ -force -d build/headers/ org.lastbamboo.jni.JLibTorrent

devenv jlibtorrent.sln jlibtorrent.vcproj /build Debug