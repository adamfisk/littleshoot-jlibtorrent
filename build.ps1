javac ./src/main/java/org/lastbamboo/jni/JLibTorrent.java

#if (!$?)
#{
#    Write-Output "Could not build Java file.  Exiting" 
#    exit 1
#}

echo "Building Java header file"
javah -verbose -classpath ./src/main/java/ -force -d build/headers/ org.lastbamboo.jni.JLibTorrent

#$buildArg = $args[0]
#$buildConfig = $args[1]
$LS_BUILD_CONFIG=[Environment]::GetEnvironmentVariable("LS_BUILD_CONFIG", "User")
$LS_VS_COMMAND=[Environment]::GetEnvironmentVariable("LS_VS_COMMAND", "User")
devenv jlibtorrent.vcproj/jlibtorrent.sln jlibtorrent.vcproj/jlibtorrent.vcproj $LS_VS_COMMAND $LS_BUILD_CONFIG
if (!$?)
{
    Write-Output "Could not build JNI project for LibTorrent. Exiting" 
    exit 1
}

cp ./jlibtorrent.vcproj/$LS_BUILD_CONFIG/jnltorrent.dll ../../lib/jnltorrent.dll

if (!$?)
{
    Write-Output "Could not copy dll. Exiting" 
    exit 1
}