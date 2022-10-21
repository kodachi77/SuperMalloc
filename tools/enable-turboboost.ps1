# Run the following line if needed
# Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope CurrentUser

if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {  
    Write-Error "This script requires elevated permissions."
    Exit
}

$version = ([System.Environment]::OSVersion.Version)
if(($version.Major -gt 10) -or ($version.Major -eq 10 -and $version.Build -ge 17101)) {
    $plan_name = "ultimate performance"
    $plan_guid = "e9a42b02-d5df-448d-aa00-03f14749eb61"
} else {
    $plan_name = "high performance"
    $plan_guid = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"
}

$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object ElementName -IEQ $plan_name)
if($plan) {
    $plan.Activate()
} else {
    $duplicate = & powercfg /duplicatescheme $plan_guid
    $plan_guid = $duplicate.split()[3]
    & powercfg /setactive $plan_guid
}

$str = [string]::Format("CPU speed current/max: {0}/{1}", (Get-CimInstance CIM_Processor).CurrentClockSpeed, (Get-CimInstance CIM_Processor).CurrentClockSpeed)
Write-Output $str.replace("`n", "")

