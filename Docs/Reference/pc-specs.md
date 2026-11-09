# 측정 PC

M0 첫날에 채운다. 바뀌면 날짜와 함께 행을 더한다. 모든 `runs.csv` 행은 이 표의 어느 행에서 잰 것인지 `pc` 열로 가리킨다.

| 날짜 | CPU | GPU, 드라이버 | RAM | OS | 엔진 | 해상도·`sg.*` |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-11-09 | AMD Ryzen 9 5950X (16코어 32스레드, 기본 3.4GHz) | NVIDIA GeForce RTX 4090 24GB, 드라이버 32.0.16.2114(NVIDIA 621.14, 2026-10-27) | 64GB (DDR4, 32GB 2장, 모듈 F4-3200C16-32GVK, 설정 클럭 2666MT/s) | Windows 11 Pro 26H2 (10.0.26300.9818) | 소스 빌드 `G:\Epic Games\UE_5.8.4`, 5.8.4(`Engine/Build/Build.version`, BranchName `UE5`, CompatibleChangelist 55116800), git `f45729c0b`(2026-10-20) | 2560×1440 창, 스케일러빌리티 Epic(3) |

값은 2026-11-09에 읽었다: `Get-CimInstance Win32_Processor`·`Win32_PhysicalMemory`·`Win32_VideoController`, `nvidia-smi --query-gpu=name,driver_version,memory.total`, `git -C "G:\Epic Games\UE_5.8.4" rev-parse --short HEAD`와 `log -1 --format=%cs`. OS 빌드 번호는 M0\~M3 CSV 메타데이터(`[os]`)와 같다. 이 행은 M0\~M3 측정 전체(`runs.csv`의 `pc`=1)에 적용한다. 그 사이 드라이버·OS를 갱신했는지는 이 시점 이전 값을 몰라 확인하지 못했다. RAM 클럭 2666은 모듈 정격(3200)보다 낮다. M3b부터 `runs.csv`의 `build` 열이 실행마다 드라이버와 빌드를 남긴다.
