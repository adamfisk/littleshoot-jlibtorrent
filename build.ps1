#javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java
cmd /c "mvn.bat install -Dmaven.test.skip=true"

if (!$?) {
    Write-Output "Could not build Java file. Is javac on your path? Exiting." 
    exit 1
}

echo "Building Java header file"
javah -verbose -classpath ./target/classes -force -d src/main/cpp org.lastbamboo.jni.JLibTorrent

#$buildArg = $args[0]
#$buildConfig = $args[1]
$LS_BUILD_CONFIG=[Environment]::GetEnvironmentVariable("LS_BUILD_CONFIG", "User")
$LS_VS_COMMAND=[Environment]::GetEnvironmentVariable("LS_VS_COMMAND", "User")

#devenv jlibtorrent.vcproj/jlibtorrent.sln jlibtorrent.vcproj/jlibtorrent.vcproj $LS_VS_COMMAND $LS_BUILD_CONFIG


#devenv jlibtorrent.vcproj/jlibtorrent.sln $LS_VS_COMMAND $LS_BUILD_CONFIG

# Note only release builds seem to work!!!!
devenv jlibtorrent.vcproj/jlibtorrent.sln $LS_VS_COMMAND Release
if (!$?) {
    Write-Output "Could not build JNI project for LibTorrent. Exiting" 
    exit 1
}

cp ./jlibtorrent.vcproj/Release/jnltorrent.dll ../../lib/jnltorrent.dll

if (!$?) {
    Write-Output "Could not copy dll. Exiting" 
    exit 1
}