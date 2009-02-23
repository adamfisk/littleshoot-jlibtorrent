javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java

javah -verbose -classpath ./src/main/java/ -force -d build/headers/ org.lastbamboo.jni.JLibTorrent

$buildArg = $args[0]
$buildConfig = $args[1]
devenv jlibtorrent.vcproj/jlibtorrent.sln jlibtorrent.vcproj/jlibtorrent.vcproj $buildArg $buildConfig

cp ./jlibtorrent.vcproj/Debug/jlibtorrent.dll ../../lib/jnltorrent.dll