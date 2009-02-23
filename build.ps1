./buildBase.ps1 /build Debug

if (!$?)
{
    Write-Output "Could not build Debug release.  Exiting" 
    exit 1
}
