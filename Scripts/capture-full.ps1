# 총괄 묶음 촬영(Docs/Worklog/Q5.md "2단위", plan.md "HUD와 촬영"). 측정이 끝난 뒤 따로 돈다. 사다리 8단계 x 목표 총수 x P1을 HUD 켜고 띄워 ffmpeg(gdigrab)로 게임 창의 한 구간을 녹화해 GIF로 만든다.
# 라벨: <Batch>shot-<단계>-<변형>-N<N>-P1-r<Run>. 접두사가 <Batch>-가 아니라 summarize.py·spikes.py·equivalence-check.py의 글롭에 잡히지 않는다. 측정 CSV는 건드리지 않는다.
# 결과는 git 밖 Saved/Capture/<Batch>/<라벨>.gif이고 경로·크기·시각은 Measurements/<Batch>/captures.csv에 적는다(경로만 커밋). 한 단계가 실패해도 다음으로 넘어가고, 종료 코드는 늘 0이며 끝에 실패 수를 출력한다.
# 녹화 구간: 게임 창이 뜬 뒤 -RecordDelaySec초 기다렸다가 -Seconds초 녹화한다(기본 24초 뒤 10초: 워밍업 15초 + 시작 지연 여유). 게임은 -LabDuration으로 측정 구간을 줄이고
# -LabShotAt(기본 45초)로 끝에 스크린샷 한 장을 찍어 ablation-check.py --shot 대조를 통과시킨다(LabHud 1, LabShotAt > 0). 녹화 부하로 HUD 숫자가 측정 실행보다 높을 수 있다(글 캡션).
# 사다리 단계는 Scripts/full-conditions.json의 group ladder 조건이다(이름을 박지 않는다).
param(
    [string]$Batch = "Full1",
    [int]$TargetN = 10000,
    [int]$Run = 1,
    [int]$Seconds = 10,
    [int]$RecordDelaySec = 24,
    [int]$GameDurationSec = 40,
    [int]$ShotAt = 45,
    [int]$Width = 960,
    [int]$Fps = 15,
    # 사다리 번호 목록(예: 8 또는 1,8). 비우면 8단계 전부
    [int[]]$Steps = @(),
    [switch]$DryRun
)
$env:LAB_NO_ENGINE = "1"
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = "Continue"
$PSNativeCommandUseErrorActionPreference = $false

$Doc = Get-Content "$PSScriptRoot\full-conditions.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Ladder = @($Doc.conditions | Where-Object { $_.group -eq "ladder" } | Sort-Object { [int]$_.ladder })
if ($Steps.Count -gt 0) { $Ladder = @($Ladder | Where-Object { $Steps -contains [int]$_.ladder }) }
$Exe = "$ProjectDir\$(($Doc.exe) -replace '/', '\')"
$PkgRoot = "$ProjectDir\Packaged\Test\Windows\HordeOptLab"
$PkgCsv = "$PkgRoot\Saved\Profiling\CSV"
$OutCsv = "$ProjectDir\Saved\Profiling\CSV"
$CapDir = "$ProjectDir\Saved\Capture\$Batch"
$Index = "$ProjectDir\Measurements\$Batch\captures.csv"
$FfCmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
$Ffmpeg = if ($FfCmd) { $FfCmd.Source } else { "ffmpeg" }

function Get-Label($c) { return "${Batch}shot-$($c.label_step)-$($c.label_variant)-N$TargetN-P1-r$Run" }
function Get-GameArgs($c, $label) {
    $a = @($Doc.common_args) + @($c.args) + @("-LabN=$TargetN", "-LabPreset=1", "-LabLabel=$label", "-LabHud", "-LabShotAt=$ShotAt", "-LabDuration=$GameDurationSec")
    return $a
}
# Start-Process -ArgumentList는 배열 원소를 따옴표로 감싸 주지 않으므로 공백이 든 경로·창 제목은 직접 감싼다
function Q($s) { return "`"$s`"" }
# 1패스 캡처(거의 무손실 mkv) -> 2패스 팔레트 GIF
function Get-CaptureArgs($title, $mkv) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-f", "gdigrab", "-framerate", "$Fps", "-draw_mouse", "0", "-t", "$Seconds", "-i", (Q "title=$title"), "-c:v", "libx264", "-qp", "0", "-preset", "ultrafast", "-pix_fmt", "yuv444p", (Q $mkv))
}
function Get-PaletteArgs($mkv, $pal) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-i", (Q $mkv), "-vf", "fps=$Fps,scale=${Width}:-1:flags=lanczos,palettegen=stats_mode=diff", (Q $pal))
}
function Get-GifArgs($mkv, $pal, $gif) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-i", (Q $mkv), "-i", (Q $pal), "-lavfi", "fps=$Fps,scale=${Width}:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5", (Q $gif))
}

$PerRun = 30 + $GameDurationSec + 15 + 2 + 25   # 시작 + 워밍업 + 측정 + 플러시 + 인코딩 여유
if ($DryRun) {
    Write-Host "DRYRUN capture batch=$Batch N=$TargetN runs=$($Ladder.Count) record=${Seconds}s after window+${RecordDelaySec}s, game -LabDuration=$GameDurationSec -LabShotAt=$ShotAt, gif ${Width}px ${Fps}fps"
    Write-Host "ffmpeg: $Ffmpeg (found=$([bool]$FfCmd)); output dir: $CapDir"
    Write-Host ("est={0:N1} min ({1}s per run)" -f ($Ladder.Count * $PerRun / 60.0), $PerRun)
    foreach ($c in $Ladder) {
        $l = Get-Label $c
        Write-Host "  $l"
        Write-Host "    game: `"$Exe`" $((Get-GameArgs $c $l) -join ' ')"
    }
    $c0 = $Ladder[0]; $l0 = Get-Label $c0
    Write-Host "  ffmpeg capture: `"$Ffmpeg`" $((Get-CaptureArgs '<window title>' "$CapDir\$l0.mkv") -join ' ')"
    Write-Host "  ffmpeg palette: `"$Ffmpeg`" $((Get-PaletteArgs "$CapDir\$l0.mkv" "$CapDir\$l0.png") -join ' ')"
    Write-Host "  ffmpeg gif:     `"$Ffmpeg`" $((Get-GifArgs "$CapDir\$l0.mkv" "$CapDir\$l0.png" "$CapDir\$l0.gif") -join ' ')"
    Write-Host "  check:          py -3 -I Scripts/ablation-check.py <csv> --cond <name> --shot"
    exit 0
}

if (-not (Test-Path $Exe)) { Write-Host "FAIL: no Test package at $Exe."; exit 1 }
if (-not $FfCmd) { Write-Host "FAIL: ffmpeg not found (where ffmpeg)."; exit 1 }
$Running = @(Get-Process UnrealEditor, HordeOptLab-Win64-Test, chrome -ErrorAction SilentlyContinue)
if ($Running.Count -gt 0) { Write-Host "FAIL: editor, game or chrome is up ($($Running.ProcessName -join ', '))."; exit 1 }

if (-not ('FgWin' -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class FgWin {
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@
}
function Get-FgPid { $p = [uint32]0; [void][FgWin]::GetWindowThreadProcessId([FgWin]::GetForegroundWindow(), [ref]$p); return [int]$p }
function Set-GameForeground($Proc) {
    $Deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $Deadline) {
        $Proc.Refresh()
        if ($Proc.HasExited) { return "focus=exited" }
        if ($Proc.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }
    if ($Proc.MainWindowHandle -eq [IntPtr]::Zero) { return "focus=NOWINDOW" }
    $H = $Proc.MainWindowHandle
    [void][FgWin]::SetForegroundWindow($H); Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=ok" }
    try { [void](New-Object -ComObject WScript.Shell).AppActivate($Proc.Id) } catch { }
    Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=appactivate" }
    [FgWin]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [FgWin]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
    [void][FgWin]::SetForegroundWindow($H); Start-Sleep -Milliseconds 300
    if ((Get-FgPid) -eq $Proc.Id) { return "focus=altkey" }
    return "focus=FAILED"
}

New-Item -ItemType Directory -Force -Path $CapDir, "$ProjectDir\Measurements\$Batch" | Out-Null
if (-not (Test-Path $Index)) { Set-Content -Path $Index -Value "time,label,status,gif,size_kb,record_sec,note" -Encoding UTF8 }

function Capture-One($c) {
    $label = Get-Label $c
    $gif = "$CapDir\$label.gif"
    $mkv = "$CapDir\$label.mkv"; $pal = "$CapDir\$label.palette.png"
    $res = @{ Ok = $false; Note = "" }
    $Proc = $null
    try {
        if ((Test-Path $gif) -or (Test-Path "$OutCsv\$label.csv")) { $res.Note = "label already used (gif or csv exists). Use -Run."; return $res }
        Remove-Item "$PkgCsv\$label.csv", "$PkgRoot\Saved\Lab\$label.summary.txt" -ErrorAction SilentlyContinue
        Write-Host "SHOT $label $(Get-Date -Format 'HH:mm:ss')"
        $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $c $label) -PassThru
        $focus = Set-GameForeground $Proc
        Write-Host $focus
        if ($focus -notmatch "^focus=(ok|appactivate|altkey)$") { $res.Note = $focus; return $res }
        $Proc.Refresh()
        $title = $Proc.MainWindowTitle
        if (-not $title) { $res.Note = "empty window title"; return $res }
        Start-Sleep -Seconds $RecordDelaySec
        if ($Proc.HasExited) { $res.Note = "game exited before recording"; return $res }
        $errLog = "$CapDir\$label.ffmpeg.log"
        $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-CaptureArgs $title $mkv) -Wait -PassThru -NoNewWindow -RedirectStandardError $errLog
        if ($p.ExitCode -ne 0 -or -not (Test-Path $mkv)) { $res.Note = "ffmpeg capture exit $($p.ExitCode) (see $errLog)"; return $res }
        $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-PaletteArgs $mkv $pal) -Wait -PassThru -NoNewWindow -RedirectStandardError "$errLog.pal"
        if ($p.ExitCode -ne 0) { $res.Note = "ffmpeg palette exit $($p.ExitCode)"; return $res }
        $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-GifArgs $mkv $pal $gif) -Wait -PassThru -NoNewWindow -RedirectStandardError "$errLog.gif"
        if ($p.ExitCode -ne 0 -or -not (Test-Path $gif)) { $res.Note = "ffmpeg gif exit $($p.ExitCode)"; return $res }
        Remove-Item $mkv, $pal -ErrorAction SilentlyContinue
        $res.Ok = $true
        # 게임이 스스로 끝나길 기다린다(제한 시간 뒤 종료). 그다음 CSV를 가져와 --shot 대조
        if (-not $Proc.WaitForExit(($GameDurationSec + 120) * 1000)) { Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue; $res.Note = "game did not exit, killed; "; }
        if (Test-Path "$PkgCsv\$label.csv") {
            $out = (& py -3 -I "$ProjectDir\Scripts\ablation-check.py" "$PkgCsv\$label.csv" --cond $c.name --shot 2>&1 | Out-String)
            if ($LASTEXITCODE -ne 0) { $res.Note += "metadata check failed: " + ([regex]::Match($out, "CHECK [^\r\n]*FAIL[^\r\n]*")).Value }
            Move-Item "$PkgCsv\$label.csv" "$OutCsv\$label.csv" -Force
        } else { $res.Note += "no csv from the shot run" }
        return $res
    } catch {
        $res.Note = "exception: $($_.Exception.Message)"
        return $res
    } finally {
        if ($Proc -and -not $Proc.HasExited) { Stop-Process -Id $Proc.Id -Force -ErrorAction SilentlyContinue }
    }
}

$fail = 0
foreach ($c in $Ladder) {
    $label = Get-Label $c
    $r = Capture-One $c
    $gif = "$CapDir\$label.gif"
    $size = if (Test-Path $gif) { [math]::Round((Get-Item $gif).Length / 1KB) } else { "" }
    $note = ($r.Note -replace "[\r\n,]+", " ").Trim()
    Add-Content -Path $Index -Encoding UTF8 -Value ("{0},{1},{2},{3},{4},{5},{6}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $label, $(if ($r.Ok) { "OK" } else { "FAIL" }), $(if ($r.Ok) { $gif } else { "" }), $size, $Seconds, $note)
    if (-not $r.Ok) { $fail++; Write-Host "FAIL $label : $($r.Note)" } else { Write-Host "OK $label -> $gif ($size KB) $note" }
}
Write-Host "CAPTURE DONE $Batch shots=$($Ladder.Count) failed=$fail (index: $Index)"
exit 0
