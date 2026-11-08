# Master Duel Switcher - Windows Uninstaller
$installDir = "$env:LOCALAPPDATA\MasterDuelSwitcher"
$desktopShortcut = "$([Environment]::GetFolderPath('Desktop'))\Master Duel Switcher.lnk"
$startMenuShortcut = "$([Environment]::GetFolderPath('Programs'))\Master Duel Switcher.lnk"

if (Test-Path $installDir) { Remove-Item -Recurse -Force $installDir }
if (Test-Path $desktopShortcut) { Remove-Item -Force $desktopShortcut }
if (Test-Path $startMenuShortcut) { Remove-Item -Force $startMenuShortcut }

Write-Host "✓ Master Duel Switcher removed successfully." -ForegroundColor Green
