# Master Duel Switcher - 1-Click Windows Installer
$ErrorActionPreference = "Stop"

$repo = "bebekgalaksi/MasterDuel-Switcher"
$installDir = "$env:LOCALAPPDATA\MasterDuelSwitcher"
$zipUrl = "https://github.com/$repo/releases/latest/download/MasterDuelSwitcher-Windows-x64.zip"
$zipPath = "$env:TEMP\MasterDuelSwitcher.zip"

Write-Host "Downloading Master Duel Switcher..." -ForegroundColor Cyan
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $zipUrl -OutFile $zipPath

Write-Host "Installing to $installDir..." -ForegroundColor Cyan
if (Test-Path $installDir) {
    Remove-Item -Recurse -Force $installDir
}
New-Item -ItemType Directory -Force -Path $installDir | Out-Null

Expand-Archive -Path $zipPath -DestinationPath "$env:TEMP\md_extract" -Force
Copy-Item -Path "$env:TEMP\md_extract\MasterDuelSwitcher-Windows-x64\*" -Destination $installDir -Recurse -Force
Remove-Item -Recurse -Force "$env:TEMP\md_extract", $zipPath

# Create Desktop Shortcut
$desktop = [Environment]::GetFolderPath("Desktop")
$wscript = New-Object -ComObject WScript.Shell
$shortcut = $wscript.CreateShortcut("$desktop\Master Duel Switcher.lnk")
$shortcut.TargetPath = "$installDir\MasterDuelSwitcher.exe"
$shortcut.WorkingDirectory = $installDir
$shortcut.Description = "Master Duel Account Switcher"
$shortcut.Save()

# Create Start Menu Shortcut
$startMenu = [Environment]::GetFolderPath("Programs")
$shortcutSM = $wscript.CreateShortcut("$startMenu\Master Duel Switcher.lnk")
$shortcutSM.TargetPath = "$installDir\MasterDuelSwitcher.exe"
$shortcutSM.WorkingDirectory = $installDir
$shortcutSM.Description = "Master Duel Account Switcher"
$shortcutSM.Save()

Write-Host ""
Write-Host "✓ Installation complete! Shortcut created on your Desktop." -ForegroundColor Green
Write-Host "Launch 'Master Duel Switcher' anytime to switch accounts." -ForegroundColor White
