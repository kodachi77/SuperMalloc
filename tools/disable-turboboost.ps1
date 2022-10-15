if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {  
    Write-Error "This script requires elevated permissions."
    Exit
}

$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object ElementName -iMatch "balanced")
$plan.Activate()