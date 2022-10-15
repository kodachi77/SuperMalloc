# Processor performance boost mode
# Values: Disabled, Enabled, Aggressive(*), Efficient Enabled, Efficient Aggressive, Aggressive At Guaranteed, Efficient Aggressive At Guaranteed
# powercfg -attributes 54533251-82be-4824-96c1-47b60b740d00 be337238-0d82-4146-a960-4f3749d470c7 -attrib_hide

# Maximum processor frequency
# Current Value=4200; Min Value=0; Max Value=4294967295 (in MHz)
# powercfg -attributes 54533251-82be-4824-96c1-47b60b740d00 75b0ae3f-bce0-45a7-8c89-c9611c25e100 -attrib_hide

# powercfg /setacvalueindex 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c 54533251-82be-4824-96c1-47b60b740d00 75b0ae3f-bce0-45a7-8c89-c9611c25e100 4200

# Processor performance boost policy
# Current Value=100; Min Value=0; Max Value=100 (in %)
# powercfg -attributes 54533251-82be-4824-96c1-47b60b740d00 45bcc044-d885-43e2-8605-ee0ec6e96b59 -attrib_hide


# Minimum processor state
# Current Value=100; Min Value=0; Max Value=100 (in %)
# powercfg -attributes 54533251-82be-4824-96c1-47b60b740d00 893dee8e-2bef-41e0-89c6-b55d0929964c -attrib_hide

# Maximum processor state
# Current Value=100; Min Value=0; Max Value=100 (in %)
# powercfg -attributes 54533251-82be-4824-96c1-47b60b740d00 bc5038f7-23e0-4960-96da-33abaf5935ec -attrib_hide

$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object ElementName -IEQ "high performance")
$plan.Activate()

$str = [string]::Format("CPU speed current/max: {0}/{1}", (Get-CimInstance CIM_Processor).CurrentClockSpeed, (Get-CimInstance CIM_Processor).CurrentClockSpeed)
Write-Output $str.replace("`n", "")

