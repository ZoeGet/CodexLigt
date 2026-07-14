param(
  [string]$Python = "",
  [string]$MonitorArgs = "",
  [string]$WorkDir = ""
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($WorkDir)) {
  $WorkDir = Split-Path -Parent $scriptDir
}

$monitorScript = Join-Path $scriptDir "codex_light_monitor.py"
$logDir = Join-Path $scriptDir "logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$stdoutLog = Join-Path $logDir "codex_light_monitor.out.log"
$stderrLog = Join-Path $logDir "codex_light_monitor.err.log"

function Resolve-PythonPath {
  param([string]$Requested)

  if (-not [string]::IsNullOrWhiteSpace($Requested)) {
    return $Requested
  }

  $pythonCmd = Get-Command python -ErrorAction SilentlyContinue
  if ($pythonCmd -and $pythonCmd.Source) {
    $pythonw = Join-Path (Split-Path -Parent $pythonCmd.Source) "pythonw.exe"
    if (Test-Path $pythonw) {
      return $pythonw
    }
    return $pythonCmd.Source
  }

  $pythonwCmd = Get-Command pythonw -ErrorAction SilentlyContinue
  if ($pythonwCmd -and $pythonwCmd.Source) {
    return $pythonwCmd.Source
  }

  throw "Python was not found. Install Python or pass -Python C:\path\to\pythonw.exe."
}

function Split-Args {
  param([string]$Text)

  if ([string]::IsNullOrWhiteSpace($Text)) {
    return @()
  }

  $matches = [regex]::Matches($Text, '"([^"\\]*(?:\\.[^"\\]*)*)"|(\S+)')
  $items = New-Object System.Collections.Generic.List[string]
  foreach ($match in $matches) {
    if ($match.Groups[1].Success) {
      $items.Add($match.Groups[1].Value.Replace('\"', '"'))
    } else {
      $items.Add($match.Groups[2].Value)
    }
  }
  return $items.ToArray()
}

$pythonPath = Resolve-PythonPath -Requested $Python
$monitorProcess = $null

function Start-Monitor {
  if ($script:monitorProcess -and -not $script:monitorProcess.HasExited) {
    return
  }

  $args = New-Object System.Collections.Generic.List[string]
  $args.Add($monitorScript)
  foreach ($arg in (Split-Args -Text $MonitorArgs)) {
    $args.Add($arg)
  }

  $script:monitorProcess = Start-Process `
    -FilePath $pythonPath `
    -ArgumentList $args.ToArray() `
    -WorkingDirectory $WorkDir `
    -WindowStyle Hidden `
    -RedirectStandardOutput $stdoutLog `
    -RedirectStandardError $stderrLog `
    -PassThru
}

function Stop-Monitor {
  if ($script:monitorProcess -and -not $script:monitorProcess.HasExited) {
    try {
      $script:monitorProcess.Kill()
      $script:monitorProcess.WaitForExit(3000) | Out-Null
    } catch {
      # Process may already be gone.
    }
  }
  $script:monitorProcess = $null
}

function Restart-Monitor {
  Stop-Monitor
  Start-Monitor
}

$notifyIcon = New-Object System.Windows.Forms.NotifyIcon
$notifyIcon.Icon = [System.Drawing.SystemIcons]::Application
$notifyIcon.Text = "CodexLight Bridge"
$notifyIcon.Visible = $true

$menu = New-Object System.Windows.Forms.ContextMenuStrip

$statusItem = New-Object System.Windows.Forms.ToolStripMenuItem
$statusItem.Text = "CodexLight Bridge running"
$statusItem.Enabled = $false
[void]$menu.Items.Add($statusItem)

$openLogItem = New-Object System.Windows.Forms.ToolStripMenuItem
$openLogItem.Text = "Open log folder"
$openLogItem.Add_Click({ Start-Process explorer.exe $logDir })
[void]$menu.Items.Add($openLogItem)

$restartItem = New-Object System.Windows.Forms.ToolStripMenuItem
$restartItem.Text = "Restart monitor"
$restartItem.Add_Click({ Restart-Monitor })
[void]$menu.Items.Add($restartItem)

[void]$menu.Items.Add((New-Object System.Windows.Forms.ToolStripSeparator))

$exitItem = New-Object System.Windows.Forms.ToolStripMenuItem
$exitItem.Text = "Exit"
$exitItem.Add_Click({
  Stop-Monitor
  $notifyIcon.Visible = $false
  $notifyIcon.Dispose()
  [System.Windows.Forms.Application]::Exit()
})
[void]$menu.Items.Add($exitItem)

$notifyIcon.ContextMenuStrip = $menu
$notifyIcon.Add_DoubleClick({ Start-Process explorer.exe $logDir })

Start-Monitor

[System.Windows.Forms.Application]::Run()

