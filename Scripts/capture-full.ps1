# 총괄 묶음 촬영(Docs/Worklog/Q5.md "2단위", plan.md "HUD와 촬영"). 측정이 끝난 뒤 따로 돈다. 사다리 8단계 x 목표 총수 x P1을 HUD 켜고 띄워 ffmpeg(gdigrab)로 게임 창의 한 구간을 녹화해 움직이는 WebP로 만든다(-Format gif면 GIF).
# 라벨: <Batch>shot-<단계>-<변형>-N<N>-P1-r<Run>. 접두사가 <Batch>-가 아니라 summarize.py·spikes.py·equivalence-check.py의 글롭에 잡히지 않는다. 측정 CSV는 건드리지 않는다.
# 결과는 git 밖 Saved/Capture/<Batch>/<라벨>.webp(또는 .gif)이고 경로·크기·시각은 Measurements/<Batch>/captures.csv에 적는다(경로만 커밋). 한 단계가 실패해도 다음으로 넘어가고, 종료 코드는 늘 0이며 끝에 실패 수를 출력한다.
# 영역: 창 제목으로 찍는 gdigrab(title=)은 DPI 가상화를 받는 ffmpeg가 창 크기를 논리 픽셀(배율 150%면 2/3)로 읽고 화면 DC에서는 물리 픽셀을 복사해 왼쪽 위 일부만 담는다(Pre1).
# 그래서 ffmpeg를 DPI 인식 상태로 띄우고(__COMPAT_LAYER=HighDpiAware) 게임 창 클라이언트 영역의 물리 화면 좌표를 구해 -f gdigrab -i desktop -offset_x/-offset_y/-video_size로 그 영역만 찍는다. 영역이 가상 화면 밖으로 넘치면 실패다.
# 녹화 구간: 게임 창이 뜬 뒤 -RecordDelaySec초 기다렸다가 -Seconds초 녹화한다(기본 24초 뒤 8초: 워밍업 15초 + 시작 지연 여유). 게임은 -LabDuration으로 측정 구간을 줄이고
# -LabShotAt(기본 45초)로 끝에 스크린샷 한 장을 찍어 ablation-check.py --shot 대조를 통과시킨다(LabHud 1, LabShotAt > 0). 녹화 부하로 HUD 숫자가 측정 실행보다 높을 수 있다(글 캡션).
# 화면 왼쪽 위 HUD에 엔진의 "CsvProfiler frame: N" 화면 메시지가 겹치므로 -ExecCmds=DisableAllScreenMessages로 끈다(UnrealEngine.cpp:13786 bShowScreenMessages, 5951 명령. csv 캡처는 --shot 대조에 필요해 그대로 켠다. Docs/Reference/engine-notes.md).
# 사다리 단계는 Scripts/full-conditions.json의 group ladder 조건이다(이름을 박지 않는다).
param(
    [string]$Batch = "Full1",
    [int]$TargetN = 10000,
    [int]$Run = 1,
    [int]$Seconds = 8,
    [int]$RecordDelaySec = 24,
    [int]$GameDurationSec = 40,
    [int]$ShotAt = 45,
    [int]$Width = 960,
    [int]$Fps = 15,
    # 움직이는 WebP libwebp_anim -q:v(0~100, 클수록 큰 파일·좋은 화질)
    [int]$Quality = 60,
    # webp(기본) 또는 gif(팔레트 2패스)
    [ValidateSet("webp","gif")][string]$Format = "webp",
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
    $a = @($Doc.common_args) + @($c.args) + @("-LabN=$TargetN", "-LabPreset=1", "-LabLabel=$label", "-LabHud", "-LabShotAt=$ShotAt", "-LabDuration=$GameDurationSec", "-ExecCmds=DisableAllScreenMessages")
    return $a
}
# Start-Process -ArgumentList는 배열 원소를 따옴표로 감싸 주지 않으므로 공백이 든 경로·창 제목은 직접 감싼다
function Q($s) { return "`"$s`"" }
# 1패스 캡처(거의 무손실 mkv) -> 움직이는 WebP(기본) 또는 2패스 팔레트 GIF
function Get-CaptureArgs($rect, $mkv) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-f", "gdigrab", "-framerate", "$Fps", "-draw_mouse", "0", "-offset_x", "$($rect.X)", "-offset_y", "$($rect.Y)", "-video_size", "$($rect.W)x$($rect.H)", "-t", "$Seconds", "-i", "desktop", "-c:v", "libx264", "-qp", "0", "-preset", "ultrafast", "-pix_fmt", "yuv444p", (Q $mkv))
}
function Get-WebpArgs($mkv, $webp) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-i", (Q $mkv), "-vf", "fps=$Fps,scale=${Width}:-1", "-c:v", "libwebp_anim", "-q:v", "$Quality", "-loop", "0", (Q $webp))
}
function Get-PaletteArgs($mkv, $pal) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-i", (Q $mkv), "-vf", "fps=$Fps,scale=${Width}:-1:flags=lanczos,palettegen=stats_mode=diff", (Q $pal))
}
function Get-GifArgs($mkv, $pal, $gif) {
    return @("-y", "-hide_banner", "-loglevel", "warning", "-i", (Q $mkv), "-i", (Q $pal), "-lavfi", "fps=$Fps,scale=${Width}:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5", (Q $gif))
}

$PerRun = 30 + $GameDurationSec + 15 + 2 + 25   # 시작 + 워밍업 + 측정 + 플러시 + 인코딩 여유
if ($DryRun) {
    Write-Host "DRYRUN capture batch=$Batch N=$TargetN runs=$($Ladder.Count) record=${Seconds}s after window+${RecordDelaySec}s, game -LabDuration=$GameDurationSec -LabShotAt=$ShotAt, ${Format} ${Width}px ${Fps}fps q=$Quality"
    Write-Host "ffmpeg: $Ffmpeg (found=$([bool]$FfCmd)); output dir: $CapDir"
    Write-Host ("est={0:N1} min ({1}s per run)" -f ($Ladder.Count * $PerRun / 60.0), $PerRun)
    foreach ($c in $Ladder) {
        $l = Get-Label $c
        Write-Host "  $l"
        Write-Host "    game: `"$Exe`" $((Get-GameArgs $c $l) -join ' ')"
    }
    $c0 = $Ladder[0]; $l0 = Get-Label $c0
    Write-Host "  ffmpeg capture: `"$Ffmpeg`" $((Get-CaptureArgs @{ X = '<x>'; Y = '<y>'; W = '<w>'; H = '<h>' } "$CapDir\$l0.mkv") -join ' ')"
    if ($Format -eq "webp") {
        Write-Host "  ffmpeg webp:    `"$Ffmpeg`" $((Get-WebpArgs "$CapDir\$l0.mkv" "$CapDir\$l0.webp") -join ' ')"
    } else {
        Write-Host "  ffmpeg palette: `"$Ffmpeg`" $((Get-PaletteArgs "$CapDir\$l0.mkv" "$CapDir\$l0.png") -join ' ')"
        Write-Host "  ffmpeg gif:     `"$Ffmpeg`" $((Get-GifArgs "$CapDir\$l0.mkv" "$CapDir\$l0.png" "$CapDir\$l0.gif") -join ' ')"
    }
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
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int i);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@
}
# 이 PowerShell 프로세스를 DPI 인식으로 바꾼다(창 좌표가 물리 픽셀로 나온다). Add-Type 직후 한 번
[void][FgWin]::SetProcessDPIAware()
# 게임 창 클라이언트 영역의 물리 화면 좌표. 가상 화면(모든 모니터)에 다 들어오지 않으면 $null과 이유를 돌려준다
function Get-ClientRegion($H) {
    $r = New-Object FgWin+RECT
    $pt = New-Object FgWin+POINT
    if (-not [FgWin]::GetClientRect($H, [ref]$r)) { return @{ Err = "GetClientRect failed" } }
    if (-not [FgWin]::ClientToScreen($H, [ref]$pt)) { return @{ Err = "ClientToScreen failed" } }
    $reg = @{ X = $pt.X; Y = $pt.Y; W = $r.Right - $r.Left; H = $r.Bottom - $r.Top }
    $vx = [FgWin]::GetSystemMetrics(76); $vy = [FgWin]::GetSystemMetrics(77); $vw = [FgWin]::GetSystemMetrics(78); $vh = [FgWin]::GetSystemMetrics(79)
    if ($reg.W -le 0 -or $reg.H -le 0) { return @{ Err = "empty client rect $($reg.W)x$($reg.H)" } }
    if ($reg.X -lt $vx -or $reg.Y -lt $vy -or ($reg.X + $reg.W) -gt ($vx + $vw) -or ($reg.Y + $reg.H) -gt ($vy + $vh)) {
        return @{ Err = "window region $($reg.W)x$($reg.H)@$($reg.X),$($reg.Y) is outside the screen ${vw}x${vh}@${vx},${vy}" }
    }
    return $reg
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
if (-not (Test-Path $Index)) { Set-Content -Path $Index -Value "time,label,status,file,size_kb,record_sec,note" -Encoding UTF8 }

function Capture-One($c) {
    $label = Get-Label $c
    $gif = "$CapDir\$label.$Format"
    $mkv = "$CapDir\$label.mkv"; $pal = "$CapDir\$label.palette.png"
    $res = @{ Ok = $false; Note = "" }
    $Proc = $null
    try {
        if ((Test-Path $gif) -or (Test-Path "$OutCsv\$label.csv")) { $res.Note = "label already used (output or csv exists). Use -Run."; return $res }
        Remove-Item "$PkgCsv\$label.csv", "$PkgRoot\Saved\Lab\$label.summary.txt" -ErrorAction SilentlyContinue
        Write-Host "SHOT $label $(Get-Date -Format 'HH:mm:ss')"
        $Proc = Start-Process -FilePath $Exe -ArgumentList (Get-GameArgs $c $label) -PassThru
        $focus = Set-GameForeground $Proc
        Write-Host $focus
        if ($focus -notmatch "^focus=(ok|appactivate|altkey)$") { $res.Note = $focus; return $res }
        $Proc.Refresh()
        Start-Sleep -Seconds $RecordDelaySec
        if ($Proc.HasExited) { $res.Note = "game exited before recording"; return $res }
        $Proc.Refresh()
        $reg = Get-ClientRegion $Proc.MainWindowHandle
        if ($reg.Err) { $res.Note = $reg.Err; return $res }
        Write-Host "region $($reg.W)x$($reg.H) at $($reg.X),$($reg.Y)"
        $errLog = "$CapDir\$label.ffmpeg.log"
        $env:__COMPAT_LAYER = "HighDpiAware"
        try { $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-CaptureArgs $reg $mkv) -Wait -PassThru -NoNewWindow -RedirectStandardError $errLog }
        finally { Remove-Item Env:\__COMPAT_LAYER -ErrorAction SilentlyContinue }
        if ($p.ExitCode -ne 0 -or -not (Test-Path $mkv)) { $res.Note = "ffmpeg capture exit $($p.ExitCode) (see $errLog)"; return $res }
        $res.Note = "region $($reg.W)x$($reg.H)@$($reg.X),$($reg.Y); "
        if ($Format -eq "webp") {
            $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-WebpArgs $mkv $gif) -Wait -PassThru -NoNewWindow -RedirectStandardError "$errLog.enc"
            if ($p.ExitCode -ne 0 -or -not (Test-Path $gif)) { $res.Note = "ffmpeg webp exit $($p.ExitCode)"; return $res }
        } else {
            $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-PaletteArgs $mkv $pal) -Wait -PassThru -NoNewWindow -RedirectStandardError "$errLog.pal"
            if ($p.ExitCode -ne 0) { $res.Note = "ffmpeg palette exit $($p.ExitCode)"; return $res }
            $p = Start-Process -FilePath $Ffmpeg -ArgumentList (Get-GifArgs $mkv $pal $gif) -Wait -PassThru -NoNewWindow -RedirectStandardError "$errLog.enc"
            if ($p.ExitCode -ne 0 -or -not (Test-Path $gif)) { $res.Note = "ffmpeg gif exit $($p.ExitCode)"; return $res }
        }
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
    $gif = "$CapDir\$label.$Format"
    $size = if (Test-Path $gif) { [math]::Round((Get-Item $gif).Length / 1KB) } else { "" }
    $note = ($r.Note -replace "[\r\n,]+", " ").Trim()
    Add-Content -Path $Index -Encoding UTF8 -Value ("{0},{1},{2},{3},{4},{5},{6}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $label, $(if ($r.Ok) { "OK" } else { "FAIL" }), $(if ($r.Ok) { $gif } else { "" }), $size, $Seconds, $note)
    if (-not $r.Ok) { $fail++; Write-Host "FAIL $label : $($r.Note)" } else { Write-Host "OK $label -> $gif ($size KB) $note" }
}
Write-Host "CAPTURE DONE $Batch shots=$($Ladder.Count) failed=$fail (index: $Index)"
exit 0
