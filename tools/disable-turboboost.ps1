$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object ElementName -iMatch "balanced")
$plan.Activate()