if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {  
    Write-Error "This script requires elevated permissions."
    Exit
}

$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object IsActive -eq True)
$status = [string]::Format("Turboboost is {0}.", $(if($plan.ElementName -ieq "ultimate performance" -or $plan.ElementName -ieq "high performance") {"ON"} else {"OFF"}))
Write-Output $status
