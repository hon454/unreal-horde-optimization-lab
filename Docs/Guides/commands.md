# 명령과 측정 절차

STATUS.md "명령"에는 매번 쓰는 것만 둔다. 여기에는 스크립트 사양, 인자, 측정 절차를 둔다.

## 스크립트 사양

모든 스크립트는 `Scripts/common.ps1`을 불러 엔진 경로를 얻는다. 엔진 경로는 저장소에 적지 않고 `.uproject`의 `EngineAssociation`을 레지스트리(`HKCU\Software\Epic Games\Unreal Engine\Builds`)에서 찾는다(DS 실험실 방식).

| 스크립트 | 하는 일 | 거부 조건 |
| --- | --- | --- |
| `build.ps1` | `HordeOptLabEditor Win64 Development` 빌드 | `UnrealEditor` 또는 `HordeOptLab` 프로세스가 떠 있으면 |
| `run-dev.ps1 -N -Preset -Seed [-Variant] [-Small]` | Development 에디터 빌드를 `-game`으로 실행. 개발·동작 확인·오버레이 스크린샷용. 수치는 쓰지 않는다 | |
| `package.ps1 [-Config Test]` | `RunUAT BuildCookRun` → `Packaged/<Config>/`. 끝나면 소요 시간을 출력 | 다른 실행이 떠 있으면 |
| `run-bench.ps1 -Milestone <M> -N <수> -Preset <P1\|P2> [-Variant] [-Seed] [-Runs 3] [-Tag x] [-Trace]` | Test 패키지 실행 파일을 `-LabBench` 인자로 띄우고 종료를 기다린다. 라벨은 `<Milestone>-<Variant>-N<수>-<Preset><Tag>`로 만든다. 실행마다 `Saved/Profiling/CSV/<라벨>-rN.csv`를 확인하고, 없으면 실패 | 같은 라벨의 CSV가 있으면. 다른 실행이 떠 있으면 |
| `summarize.py --milestone M<N>` | `Saved/Profiling/CSV/`의 라벨을 읽어 `Measurements/M<N>/runs.csv`에 행을 더하고 `summary.csv`를 다시 쓴다 | |
| `plot.py` | `Measurements/*/summary.csv`로 `Docs/figures/`의 그래프를 다시 그린다: 60fps 최대 N(`n-at-60fps.svg`와 `.csv`), N 스케일링(`n-scaling-P1.svg`) | |
| `summarize.py --milestone M<N> [--rebuild]` (M3b) | 열 추가: `frame_p95_ms`, `bottleneck`(프레임·Game·Render·GPU p50 중 최댓값 이름), `build`(CSV 메타데이터 `LabBuild`·`LabCVars`·`LabDriver`, 없으면 빈칸). `--rebuild`는 기존 행도 원본 CSV로 다시 계산한다. 태그 `aa*` 라벨은 `<변형>-aa` 조건으로 따로 요약, `trace*`는 summary에서 뺀다 | |
| `spikes.py <라벨 접두사 또는 CSV...> [--factor 2] [--verbose] [--with-r0]` (M3b) | 묶음의 주기적 스파이크 검사: 중앙값 2배 초과 프레임의 사건 간격 중앙값과 ±10% 비율, 실행의 주기성 판정, 묶음 요약. 예: `py -3 -I Scripts/spikes.py M3b-` | 대상 없으면 |
| `run-m3b.ps1 [-Phase all\|e3b\|matrix\|aa\|trace\|…] [-NoBase] [-Tag x] [-DryRun]` (M3b) | 묶음 1 러너. E3b (8) 24회를 실행 단위로 섞어 먼저, 전수(M0\~M3, 30k 포함), 묶음 끝 A/A 3회, 20k P1 트레이스 병렬 1 + 직렬 1 순서. `-DryRun`은 목록과 예상 시간만 출력 | 다른 실행이 떠 있으면(run-bench.ps1) |
| `run-bench.ps1 ... [-RunIndex i] [-Trace] [-ExtraArgs "..."]` (M3b) | `-RunIndex`는 r<i> 하나만(실행 단위 섞기), `-Trace`는 `Saved/Traces/<라벨>.utrace`(`-trace=cpu,gpu,frame,counters,mass`), `-ExtraArgs`는 게임 인자 덧붙임 | |
| `noise-table.py <라벨 접두사...>` (M3b) | 접두사별 실행의 프레임 p50·p95, Game·Render p50을 표로 내고 중앙값·최소\~최대·표준편차를 붙인다. 잡음 진단용. 예: `py -3 -I Scripts/noise-table.py M3b-m3mass-N20000-P1aff- M3b-m3mass-N20000-P1noaff-` | |
## 벤치마크 러너 인자(게임 측)

`-LabBench` 가 있으면 다음 순서로 동작하고 끝나면 종료한다.

1. 렌더 CVar 고정(measurement.md "렌더 고정")과 `-LabN= -LabPreset= -LabSeed= -LabVariant=` 적용. 모든 값을 로그 `lab_config`에 한 줄로 남긴다.
2. 스포너가 정상 상태 분포로 N개 배치. 카메라를 프리셋에 놓는다.
3. 15초 워밍업. 그 사이 N 유지 확인(사망·재생성이 동작하는지).
4. `csvprofile start`(압축 끔, 라벨을 파일 이름으로). 60초.
5. `csvprofile stop`. 프레임 수, N, 전환 수 등 요약을 로그 `lab_summary`에 남긴다. 조건이 깨졌으면(N 불일치 등) 종료 코드 1.
6. `-Trace`면 2\~5 동안 `-trace=cpu,gpu,frame,counters,mass`로 트레이스를 남긴다. 트레이스 실행의 수치는 비교에 쓰지 않는다(이벤트 기록 비용).

## 빼 보기 스위치(Q3)

빼 보기 표([plan.md](../plan.md) "품질 계획")의 행은 "최종 구성 변형 + 인자 하나"다. 최종 구성은 Q4부터 `q4final`(`-LabStage=M5`, `q2crowd` + 액터 상한 1,000·전환 거리 79m, 아래 "근거리 품질 인자(Q4)")이고 아홉 행 모두 `q4final` 위에서 대조를 통과했다(Worklog/Q4.md). 새 인자 다섯(`-LabHydrate`·`-LabSimLod`·`-LabIskm`·`-LabFixedBounds`·`-LabFarCull`)은 0일 때만 뜻이 있고(끄기 전용), 나머지 넷은 기존 인자다. 경위와 설계 선택은 [Worklog/Q3.md](../Worklog/Q3.md).

| 행 | 빼는 것 → 대신 | 인자 | 메타데이터에서 0이 되는 키 |
| --- | --- | --- | --- |
| final | 없음 | 없음 | 없음(`LabTech*` 아홉 모두 1) |
| hydrate | 하이드레이션 → 전 구간 ISKM(액터 0) | `-LabHydrate=0` | `LabTechHydrate` |
| budget | 애니 예산 → 예산 없는 액터(`USkeletalMeshComponent`, 매 프레임 애니) | `-LabScreenSigBudget=0` | `LabTechAnimBudget`, `LabBudget` |
| fix | 전환 대책 다섯 → 모두 끔 | `-LabHydrateFixMask=0` | `LabTechHydrateFix`, `LabHydrateFixMask` |
| pool | 엔티티 풀 → 매번 생성·파괴 | `-LabEntityPool=0` | `LabTechPool`, `LabPool` |
| simlod | Simulation LOD → 엔진 이동·매 프레임 로직을 전 개체에 | `-LabSimLod=0` | `LabTechSimLod`(`LabSimLod`는 `-`) |
| iskm | ISKM → VAT ISM(고정 바운드는 넣을 컴포넌트가 없다) | `-LabIskm=0` | `LabTechIskm`, `LabIskmBounds` |
| bounds | 고정 바운드 → 기본 바운드 | `-LabFixedBounds=0` | `LabTechFixedBounds`, `LabIskmBounds` |
| farcull | 200m 밖 지우기 → 맵 끝까지 그리기(배경 무리는 그대로 켬) | `-LabFarCull=0` | `LabTechFarCull` |
| crowdfx | Niagara 배경 무리 → 빈 수평선 | `-LabCrowdFx=0` | `LabTechCrowdFx`, `LabCrowdFxActive` |

```bash
# Test 패키지(Q5 뒤, run-bench.ps1의 ValidateSet에 q2crowd가 들어가기 전에는 -Variant m5bounds -ExtraArgs "-LabCrowdFx=1 <인자>")
powershell -ExecutionPolicy Bypass -File Scripts/run-bench.ps1 -Milestone M5 -Variant m5bounds -N 10000 -Preset P1 -Types 2 -Runs 1 -Tag hydrate -ExtraArgs "-LabCrowdFx=1 -LabHydrate=0"
# Development 작은 실행 + 메타데이터 대조(라벨에 행 이름을 넣으면 ablation-check.py가 기대값을 고른다)
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant q2crowd -Extra "-LabHydrate=0 -LabBench -LabLabel=Q3dev-hydrate-N100-P1-r1"
py -3 -I Scripts/ablation-check.py Saved/Profiling/CSV/Q3dev-hydrate-N100-P1-r1.csv
```

- `lab_config` 끝의 `tech=hyd:1,bud:1,fix:1,pool:1,sim:1,iskm:1,bnd:1,far:1,fx:1`이 아홉 기법의 설정이다. 단계가 그 기법을 아직 갖지 않으면 0이다(M4 `m4mass`의 Simulation LOD는 틱 간격만이라 `tick`, 전환 대책 일부만 켜면 `m<마스크>`).
- 실제 적용값은 `lab_applied`(개발 빌드 로그, 벤치마크는 측정 시작 때, 개발 실행은 끝날 때)와 CSV 메타데이터 `LabStage`·`LabBudget`(할당기가 도는가)·`LabPool`·`LabIskmBounds`(고정 바운드가 들어간 ISKM 컴포넌트 수, 2종이면 2)·`LabHydrateFixMask`다.
- `-LabFarM`(그리는 거리)은 표현 Off와 Simulation LOD Off, 배경 무리 안쪽 반경을 함께 옮기므로 farcull 행에 쓰지 않는다. `-LabFarCull=0`은 표현 Off만 맵 반경의 두 배(1,000m)로 옮긴다.

## 전환 점수(Q3)

`Scripts/transition-score.py`는 60fps 고정 간격 연속 캡처와 그 실행의 개발 로그로 전환 사건마다 화면 튐을 잰다. ISKM과 VAT, 전환 대책 켬·끔은 같은 시드·같은 인자로 찍고 표현만 바꾼다. 캡처는 `Saved/Screenshots/LabBurst/`에 쌓이므로 실행마다 다른 폴더로 옮긴다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 10000 -Preset P1 -Duration 21 -ShotAt 10 -Stage M5 -Variant q2crowd -Extra "-LabShotBurst=600 -benchmark -fps=60"
py -3 Scripts/transition-score.py <캡처 폴더> Saved/Logs/LabDev-q2crowd-N10000-P1.log --event both
```

`--event`는 `hydrate`(인스턴스 → 액터, 기본), `dehydrate`(액터 → 인스턴스), `both`. VAT 표현은 `-Extra`에 `-LabIskm=0`, 대책 끔은 `-LabHydrateFixMask=0`을 더한다.

## 근거리 품질 인자(Q4)

최종 구성 변형은 `q4final`(= `q2crowd` + 액터 상한 1,000 + 화면 안 전환 거리 79m, `-LabStage=M5`)이다. 경위는 [Worklog/Q4.md](../Worklog/Q4.md).

| 인자 | 뜻 | 기본 |
| --- | --- | --- |
| `-LabActorCap=<개수>` | 액터 상한(표현 LOD High의 `LODMaxCount`, 두 종류 합계) | 500, `q4final` 1,000 |
| `-LabHydrateM=<미터>` | 화면 안에서 액터로 바뀌는 카메라 거리(1\~79m). 화면 밖 15m와 80·200m 밴드는 그대로 | 30, `m3hyd15` 15, `m3hyd50` 50, `q4final` 79 |

CSV 메타데이터 `LabActorCap`·`LabHydrateM`, 스탯 `Lab/ViewActors`(화면 안 보이는 액터)·`Lab/ViewActors30`·`Lab/ViewInst30`(화면 안 카메라 30m 안 액터·인스턴스)·`Lab/ViewInstHyd`(전환 거리 안 인스턴스). `summarize.py`가 `view_actors_*`·`cover30_*`·`coverhyd_mean` 열로 낸다.

```bash
# Q4 묶음 러너(run-bench.ps1을 조건마다 부른다. "N:프리셋:상한:거리:횟수:태그앞:태그뒤"를 ;로 잇는다)
powershell -ExecutionPolicy Bypass -File Scripts/run-q4.ps1 -Jobs "10000:P1:1000:79:3::;10000:P2:1000:79:3::" -FirstWithR0
py -3 -I Scripts/summarize.py --milestone Q4
# 빠른 회전 뒤 채워지는 시간(개발 빌드 참고, -LabCamTurn -LabFrameLog 로그)
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 10000 -Preset P1 -Stage M5 -Variant q4final -Duration 71 -Extra "-LabCamTurn -LabFrameLog -benchmark -fps=60"
py -3 -I Scripts/turn-fill.py Saved/Logs/LabDev-q4final-N10000-P1.log
```

## 측정 절차

1. 측정 전에 잴 조건, 라벨, 걸리는 시간을 채팅에 알리고 답을 받는다.
2. `package.ps1`로 그 커밋의 Test 패키지를 만든다. 패키지 폴더에 커밋 해시를 남긴다.
3. 묶음의 맨 첫 실행 1회만 버린다(`run-bench.ps1`은 기본으로 r0을 돌린다. 첫 조건 뒤로는 `-SkipR0`나 `-RunIndex`를 준다). 그 뒤 조건별 3회.
4. 실행마다 `Saved/Lab/<라벨>.summary.txt`와 CSV 메타데이터를 확인한다(Test 패키지는 로그를 쓰지 않는다. 개발 빌드는 로그의 `lab_config`·`lab_summary`). N이 유지되지 않았거나 프레임 수가 비정상이면 그 실행을 버리고 라벨을 바꿔 다시 잰다.
5. `summarize.py` → `runs.csv`, `summary.csv`. 차이 판정은 measurement.md.
6. STATUS.md "측정 결과"에 묶음을 적는다. 마일스톤이 끝나면 Worklog로 옮긴다.

## 스크린샷·영상 실행

- 오버레이 스크린샷(`stat unit`, `a.Budget.Debug.Enabled 1`, 밴드 색 디버그 뷰)은 `run-dev.ps1`로 Development에서 찍는다. 그 실행의 수치는 쓰지 않는다.
- Test 패키지 스크린샷은 `stat unit`만 가능하다.
- 영상은 수치 실행에서 찍지 않는다. 라벨 `visualN`으로 따로 돌린다.

## 측정 PC 사양 읽기

[pc-specs.md](../Reference/pc-specs.md)의 값은 PowerShell에서 아래 명령으로 읽었다. 구성이 바뀌면 같은 명령으로 다시 읽는다.

- CPU, RAM, GPU: `Get-CimInstance Win32_Processor`, `Win32_PhysicalMemory`, `Win32_VideoController`
- GPU 드라이버: `nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv`
- 저장 장치: `Get-Partition -DriveLetter G | Get-Disk`
- 모니터 모델: `Get-CimInstance -Namespace root\wmi WmiMonitorID`
- 전원 구성표: `powercfg /getactivescheme`
- GPU 스케줄링, 게임 모드: `HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers`의 `HwSchMode`, `HKCU:\Software\Microsoft\GameBar`의 `AutoGameModeEnabled`(없으면 기본값)
- 엔진 커밋: `git -C "G:\Epic Games\UE_5.8.4" rev-parse --short HEAD`, `git -C "G:\Epic Games\UE_5.8.4" log -1 --format=%cs`

## Unreal MCP

- 서버는 에디터가 뜨면 자동으로 열린다: `Config/DefaultEditorPerProjectUserSettings.ini`의 `bAutoStartServer=True`, `ServerPortNumber=8001`(DS 실험실의 기본 8000과 겹치지 않게). 수동은 콘솔 `ModelContextProtocol.StartServer`.
- 클라이언트 설정: 프로젝트 루트 `.mcp.json`(gitignore)에 `"unreal-mcp": {"type": "http", "url": "http://127.0.0.1:8001/mcp"}`. 새 체크아웃이면 에디터 콘솔 `ModelContextProtocol.GenerateClientConfig ClaudeCode`로 다시 만든다. Claude Code는 에디터를 띄운 뒤 시작해야 툴이 잡힌다.
- 2026-11-06 확인: `-nullrhi`로 띄운 에디터가 약 15초 뒤 `initialize`에 응답했다.
- 툴 호출은 게임 스레드에서 직렬. 겹쳐 부르지 않는다. 대량 변경 전후 저장. 새 `UFUNCTION`은 에디터 재시작.
- Claude Code는 `claude --add-dir ..\unreal-horde-references`로 시작한다(`.claude/settings.json`의 `additionalDirectories`에도 있다).

## 에셋 마이그레이션

Fab 원본 팩은 `../unreal-horde-references/AssetHost/`(런처 5.8 엔진 프로젝트)에 받고, 쓰는 것만 `Scripts/migrate-assets.py`로 옮긴다. AssetHost를 연 엔진(런처 `G:\Epic Games\UE_5.8`)의 커맨드렛으로 돌린다. `LAB_MIGRATE_DRY=1`이면 의존성 목록과 크기만 출력한다.

```bash
LAB_MIGRATE_SET=minion LAB_MIGRATE_DRY=0 LAB_MIGRATE_DEST="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Content" "G:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-references/AssetHost/AssetHost.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/migrate-assets.py" -unattended -nosplash -nullrhi -stdout
```

2026-11-06 실행: 51개 패키지, 원본 기준 282.7MB(옮긴 뒤 `Content/ParagonMinions/` 244MB). 소프트 참조로 Dusk 메시와 Super 텍스처 일부가 함께 온다.

`siege` 세트(`LAB_MIGRATE_SET=siege`)는 M3b 2종의 대형(시즈 미니언) 메시·피직스·애니 셋이다. 2026-11-09 실행: 의존성 36개 중 새 패키지 11개(원본 기준 56.5MB), 나머지는 이미 있어 건너뜀(`EAssetMigrationConflict::Skip`). 종료 코드 1이 나면 [troubleshooting.md](troubleshooting.md)의 AssetHost 줄을 본다.

`orbfx` 세트(`LAB_MIGRATE_SET=orbfx`)는 오브의 타워·구체·이펙트다. 2026-11-06에 AssetHost 커맨드렛이 Python 플러그인을 올리기 전에 로그 없이 종료 코드 1로 끝나는 일이 있었다. 이때 `MI_Mesh_Overlay_TeslaCoil`과 부모 `M_Mesh_Overlay`는 같은 `/Game` 경로로 `.uasset`을 직접 복사했다. 의존성(MPC, `MF_ExposureCompensation`, 노이즈 텍스처)은 이미 옮겨져 있었다.

## 파생 에셋과 맵

- `Scripts/make-static-minion.py`: `static` 변형의 `/Game/Lab/Derived/SM_LabMinion`을 만든다(gitignore). 커맨드렛에 `-AllowCommandletRendering`이 필요하다. 새 체크아웃에서는 에셋 이전 뒤 한 번 돌린다.
- `Scripts/bake-vat.py`: M2 VAT(걷기·피격·죽기)를 Bone·Vertex 모드로 굽고 정적 메시·데이터 에셋·텍스처·머티리얼을 `/Game/Lab/VAT/Baked/`(gitignore)에 만든다. 실행은 `make-static-minion.py`와 같다(`-AllowCommandletRendering`). 새 체크아웃에서 M2 이상을 돌리기 전에 한 번 돌린다.
- `Scripts/create-iskm.py`: M5 ISKM 에셋(종류마다 `bOptimizeForInstancing`을 켠 스켈레탈 메시 복제본, ASTPD, ISKM 머티리얼)을 `/Game/Lab/ISKM/Baked/`(gitignore)에 만든다. 실행은 `make-static-minion.py`와 같다(`-AllowCommandletRendering`). `LAB_UNIT`을 비우면 두 종류, `small`/`large`면 하나. `LAB_ISKM_VERIFY=1`이면 만들지 않고 저장된 에셋을 새 프로세스에서 읽어 확인만 한다. 새 체크아웃에서 M5를 돌리기 전에 한 번 돌린다. 클립 순서와 커스텀 데이터 번호는 [Worklog/M5.md](../Worklog/M5.md) "에셋 세션".
- `Scripts/create-bench-map.py`: `L_Bench`를 엔진 Basic 템플릿으로 다시 만든다(바닥을 입자 바깥 반경 1,000m + 20m(반쪽 길이 1,020m)로 키우고 VolumetricCloud 삭제). 기존 맵이 있으면 `.umap` 파일을 먼저 지운다(스크립트의 `delete_asset`이 커맨드렛에서 듣지 않았다).

```bash
"G:/Epic Games/UE_5.8.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/HordeOptLab.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/make-static-minion.py" -unattended -nosplash -AllowCommandletRendering -stdout
```

## 벤치마크 실행(구현됨)

- `package.ps1`이 `Packaged/Test/Windows/`를 만들고 `Packaged/Test/commit.txt`에 커밋 해시(작업 트리가 더러우면 `(dirty)`)를 남긴다. 쿡은 `L_Bench`와, 코드가 경로로 부르는 `/Game/ParagonMinions`·`/Game/NiagaraExamples`·`/Game/Lab` 폴더다(`Config/DefaultGame.ini`).
- `run-bench.ps1 -Milestone M0 -N <n> -Preset <P1|P2> -Variant <v> [-Runs 3] [-Tag x] [-ResX -ResY]`: r0 + r1\~rRuns. 실행마다 CSV를 패키지 `Saved/Profiling/CSV/`에서 프로젝트 `Saved/Profiling/CSV/`로 옮기고 `Saved/Lab/<라벨>.summary.txt`를 출력한다. 해상도 기본값은 2560x1440 창이다(measurement.md "확정값").
- `run-e1.ps1 [-Tag x]`: M1 E1 (a) 묶음(mass N {1k, 2k, 5k, 20k, 50k} x P1·P2 x 3회, 첫 조건만 r0). M1부터 변형 이름은 라벨용 `mass`다. 개발 실행은 `run-dev.ps1 -Extra "-LabStage=M1"`.
- `run-e2.ps1 [-Tag x] [-OnlySub]`: M2 E2 묶음. (a) N {1k, 5k, 20k, 50k} x P1·P2 x 3회로 M1(`noanim`, `run-bench.ps1 -Stage M1`)과 M2(`mass`)를 번갈아, (b)(c)(d) 변형(`vatvert`, `vatcull`, `vatcullvis`, `vatdie0`, `vatdie1k`)은 5k P1 x 3회. 약 90분. (d)는 다시 설계해 `run-bench.ps1 -Milestone M2 -Variant kill0|kill1k -N 5000`으로 쟀다.
- `run-bench.ps1 -Stage <M>`: 결과 폴더·라벨은 `-Milestone`, 실행 단계만 바꾼다(같은 묶음에서 이전 단계를 잴 때).
- `summarize.py`는 `py -3`(3.9)로 돌린다. pandas 없이 표준 라이브러리만 쓴다.

## Unreal MCP를 HTTP로 직접 부르기

이 세션처럼 MCP 클라이언트가 붙어 있지 않을 때는 에디터를 `-nullrhi`로 띄운 뒤 JSON-RPC로 부른다. `initialize` 응답 헤더의 `Mcp-Session-Id`를 이후 요청에 붙인다.

```bash
curl -s -X POST http://127.0.0.1:8001/mcp -H "Content-Type: application/json" -H "Accept: application/json, text/event-stream" -H "Mcp-Session-Id: <id>" -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"list_toolsets","arguments":{}}}'
```
