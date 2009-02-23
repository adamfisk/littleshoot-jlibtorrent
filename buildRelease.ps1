./buildBase.ps1 /rebuild Release

if (!$?)
{
    Write-Output "Could not build LT Release build.  Exiting" 
    exit 1
}
