# 측정 PC

**구성 1**(`runs.csv`의 `pc`=1). M0부터 지금까지의 모든 측정에 쓴 PC다. 값은 2026-11-09에 읽었고, 그 전에 드라이버나 OS를 갱신했는지는 모른다.
구성이 바뀌면 끝에 "구성 2" 절을 더하고 바뀐 항목만 적는다. 값을 읽는 명령은 [commands.md](../Guides/commands.md) "측정 PC 사양 읽기"에 있다.

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

## 3. Windows 설정

**3.1 전원 구성표**: 균형 조정.

**3.2 하드웨어 가속 GPU 스케줄링**: 레지스트리 값(`HwSchMode`)이 없다. 기본값으로 동작한다. 실제로 켜져 있는지는 확인하지 않았다.

**3.3 게임 모드**: 레지스트리 값(`AutoGameModeEnabled`)이 없다. 기본값으로 동작한다.

해상도, 화질, 측정 중 PC 상태는 [measurement.md](../Guides/measurement.md)를 따른다.
