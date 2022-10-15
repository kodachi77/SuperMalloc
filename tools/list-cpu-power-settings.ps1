if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {  
    Write-Error "This script requires elevated permissions."
    Exit
}

$settings = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerSetting)
$settings_in_subgroup = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerSettingInSubgroup)
$possible_values = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerSettingDefinitionPossibleValue)
$range_data = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerSettingDefinitionRangeData)
$settings_data_idx = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerSettingDataIndex)

$plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object ElementName -IEQ "ultimate performance")
if (-not $plan) {
    Write-Warning "Cannot find plan named 'Ultimate Performance'. Using active plan."
    $plan = (Get-WmiObject -Namespace root\cimv2\power -Class Win32_PowerPlan | Where-Object IsActive -EQ True)
}
$plan_id = ($plan.InstanceID | Select-String -Pattern "{.*}$").Matches.Value
$str = "Microsoft:PowerSettingDataIndex\\" + $plan_id
$plan_settings_data_idx = ($settings_data_idx | Where-Object InstanceID -Match $str)

$SUB_PROCESSOR = (powercfg /aliases | Select-String -Pattern "([0-9a-z`-]+)\s+SUB_PROCESSOR").Matches.Groups[1].Value

$script_output = ""

$settings_in_subgroup | Where-Object GroupComponent -Match "{$SUB_PROCESSOR}" | foreach-object {
    $guid = ($_.PartComponent | Select-String -Pattern "{(.*)}`"$").Matches.Groups[1].Value
    $ps = ($settings | Where-Object InstanceID -Match $guid)
    
    $value_elems = $possible_values | Where-Object InstanceID -Match $guid
    $r = $range_data | Where-Object InstanceID -Match $guid
    
    $setting_value = ($plan_settings_data_idx | Where-Object InstanceID -Match "\\AC\\{$guid}").SettingIndexValue
    $value_desc = ""
    if ($value_elems -ne $null)
    {
        $tmp = $value_elems.ElementName | ForEach-Object { $_ }
        $tmp[$setting_value] += "(*)"
        $value_desc = [string]::Format("# Values: {0}", ($tmp -join ", "))
    }
    
    $prefix = ""
    $range_desc = ""
    if ($r -ne $null)
    {
        $valmin = $r | Where-Object ElementName -eq "ValueMin"
        $valmax = $r | Where-Object ElementName -eq "ValueMax"
        
        $prefix = $(if ($setting_value -lt $valmax.SettingValue -or $setting_value -gt $valmax.SettingValue) { "" }
            else { "# " })
        
        $range_desc = [string]::Format("# Current Value={0}; Min Value={1}; Max Value={2} (in {3})", $setting_value, $(if ($valmin -ne $null) { $valmin.SettingValue }
                else { "" }), $(if ($valmax -ne $null) { $valmax.SettingValue }
                else { "" }), $r[0].Description)
    }
    
    
    $desc = [string]::Format("# {0}", $ps.ElementName)
    $powercfg = [string]::Format("{0}powercfg -attributes {1} {2} -attrib_hide", $prefix, $SUB_PROCESSOR, $guid)
    
    $script_output += $desc + "`n"
    if ($range_desc) { $script_output += $range_desc + "`n" }
    if ($value_desc) { $script_output += $value_desc + "`n" }
    $script_output += $powercfg + "`n`n"
}

$script_output | Out-File -FilePath "unhide-cpu-power-settings.ps1" -Encoding ascii -NoNewline
