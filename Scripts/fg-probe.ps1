# 포그라운드 창 프로브(진단용). 2초마다 현재 포그라운드 창의 시각·PID·프로세스 이름·창 제목을 Saved/Lab/fg-probe.log에 덧붙인다.
# 측정 PC에 손대지 않는다(읽기만). 멈추려면 Saved/Lab/fg-probe.stop 파일을 만든다(또는 -Seconds). 로그는 git 밖.
param([int]$IntervalSec = 2, [int]$Seconds = 0)
$ErrorActionPreference = "Continue"
$Root = Split-Path $PSScriptRoot -Parent
$Dir = "$Root\Saved\Lab"; New-Item -ItemType Directory -Force -Path $Dir | Out-Null
$Log = "$Dir\fg-probe.log"; $Stop = "$Dir\fg-probe.stop"
Remove-Item -Force -ErrorAction SilentlyContinue $Stop
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class FgWin {
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
}
"@
function Add-Log($Text) {
    try {
        $fs = New-Object System.IO.FileStream($Log, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::ReadWrite)
        $w = New-Object System.IO.StreamWriter($fs, (New-Object System.Text.UTF8Encoding($false)))
        $w.WriteLine($Text); $w.Close()
    } catch { }
}
$T0 = Get-Date
Add-Log ("# fg-probe start {0:s} interval={1}s" -f $T0, $IntervalSec)
while ($true) {
    $h = [FgWin]::GetForegroundWindow()
    $procId = [uint32]0
    [void][FgWin]::GetWindowThreadProcessId($h, [ref]$procId)
    $name = try { (Get-Process -Id $procId -ErrorAction Stop).ProcessName } catch { "?" }
    $sb = New-Object System.Text.StringBuilder 256
    [void][FgWin]::GetWindowText($h, $sb, 256)
    Add-Log ("{0:s}`t{1}`t{2}`t{3}" -f (Get-Date), $procId, $name, $sb.ToString())
    if (Test-Path $Stop) { break }
    if ($Seconds -gt 0 -and ((Get-Date) - $T0).TotalSeconds -ge $Seconds) { break }
    Start-Sleep -Seconds $IntervalSec
}
Add-Log ("# fg-probe end {0:s}" -f (Get-Date))
