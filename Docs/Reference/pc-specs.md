# 측정 PC

**구성 1** (`runs.csv`의 `pc`=1). M0부터 지금까지의 모든 측정에 쓴 PC다. 값은 2026-11-09에 읽었다.
구성이 바뀌면 이 문서 끝에 "구성 2" 절을 더하고 바뀐 항목만 적는다.

## 1. 하드웨어

**1.1 CPU**: AMD Ryzen 9 5950X. 16코어 32스레드, 기본 클럭 3.4GHz.

**1.2 GPU**: NVIDIA GeForce RTX 4090, 24GB.

**1.3 RAM**: 64GB DDR4. 32GB 모듈 2장(F4-3200C16-32GVK).

> **주의**: 2666MT/s로 동작한다. 모듈 정격 3200보다 낮다. XMP를 켜면 구성이 바뀐다.

**1.4 저장 장치**: 엔진과 프로젝트는 G: 드라이브에 있다. Solidigm P44 Pro 2TB, NVMe SSD.

**1.5 모니터**: Philips 436M6VBP, 3840×2160 60Hz.

## 2. 소프트웨어

**2.1 OS**: Windows 11 Pro 26H2, 빌드 10.0.26300.9818.

**2.2 GPU 드라이버**: NVIDIA 621.14(2026-10-27). Windows 장치 관리자에는 32.0.16.2114로 나온다.

**2.3 엔진**: UE 5.8.4 소스 빌드, `G:\Epic Games\UE_5.8.4`.
git `f45729c0b`(2026-10-20), BranchName `UE5`, CompatibleChangelist 55116800(`Engine/Build/Build.version`).

## 3. 측정할 때의 상태

**3.1 실행 방식**: Test 패키지, 2560×1440 창 모드.

**3.2 화질**: 스케일러빌리티 Epic(3). VSync 끔, 프레임 제한 없음. 나머지 고정값은 [measurement.md](../Guides/measurement.md) "렌더 고정"을 따른다.

**3.3 전원 구성표**: 균형 조정.

**3.4 하드웨어 가속 GPU 스케줄링**: 레지스트리 값(`HwSchMode`)이 없다. 기본값으로 동작한다. 실제로 켜져 있는지는 확인하지 않았다.

**3.5 게임 모드**: 레지스트리 값(`AutoGameModeEnabled`)이 없다. 기본값으로 동작한다.

> **주의**: 창 모드라서 Windows 데스크톱 합성을 거칠 수 있다. 이 수치를 전체 화면에서 잰 수치와 바로 비교하지 않는다.

> **주의**: 측정 중에 모니터를 끄거나 PC를 오래 비워 두면 모든 조건이 느려진다. [measurement.md](../Guides/measurement.md) "측정 중 PC 상태"를 지킨다.

## 4. 기록의 한계

- M0\~M3 사이에 드라이버나 OS를 갱신했는지는 모른다. 2026-11-09 이전 값을 남기지 않았다.
- OS 빌드 번호는 M0\~M3 CSV 메타데이터의 `[os]`와 같다.
- M3b부터는 `runs.csv`의 `build` 열이 실행마다 드라이버와 빌드를 남긴다.

## 5. 값을 다시 읽는 법

PowerShell에서 읽는다.

- CPU, RAM, GPU: `Get-CimInstance Win32_Processor`, `Win32_PhysicalMemory`, `Win32_VideoController`
- GPU 드라이버: `nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv`
- 저장 장치: `Get-Partition -DriveLetter G | Get-Disk`
- 모니터 모델: `Get-CimInstance -Namespace root\wmi WmiMonitorID`
- 전원 구성표: `powercfg /getactivescheme`
- 엔진 커밋: `git -C "G:\Epic Games\UE_5.8.4" rev-parse --short HEAD`, `git -C "G:\Epic Games\UE_5.8.4" log -1 --format=%cs`
