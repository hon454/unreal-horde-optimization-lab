# M3 전환(하이드레이션) 대책의 단계별 기록

VAT 인스턴스가 30m 안에서 액터로 바뀌는 순간의 화면을, 대책을 하나씩 쌓은 단계마다 같은 장면으로 다시 찍었다(2026-11-09, 커밋 `7689aaf` 이후 단계 토글 추가). 사용자 관찰("팝핑", "깜빡임")에서 시작해 원인을 하나씩 찾은 순서다. 경위와 엔진 근거는 [post-material.md](../../../../Docs/post-material.md) "04 액터 하이드레이션".

## 찍은 방법

- 개발 빌드(`Scripts/run-dev.ps1`), N=5000, P1, 시드 20261106, 1280x720 창.
- `-benchmark -fps=60`: 고정 시간 간격(1/60초)이라 캡처가 느려도 장면이 실제 60fps 간격으로 진행한다. 처음 캡처는 이 옵션 없이 찍어 프레임 간격이 약 150ms였고 깜빡임을 놓쳤다.
- `-LabShotBurst=120 -LabShotAt=10`: 10초부터 120프레임을 한 장씩(`Saved/Screenshots/LabBurst/f<프레임>.png`, 개발 빌드만).
- `-LabHydrateFix=0..4`: 대책 단계(`Source/HordeOptLab/LabConfig.h` `HydrateFix`). 로그 `lab_event hydrate|shown frame=… u=… v=…`에서 전환 사건의 프레임과 화면 좌표를 얻는다.
- 같은 사건(4단계 로그의 hydrate) 주변 60x60 픽셀을 사건 2프레임 전부터 8프레임 잘라 단계마다 한 줄로 붙였다(`Scripts/transition-strips.py`). 고정 시간 간격과 같은 시드라 단계 사이 장면이 거의 같다.

명령(단계마다):

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -N 5000 -Preset P1 -Duration 25 -ShotAt 10 -Extra "-LabStage=M3 -LabShotBurst=120 -benchmark -fps=60 -LabHydrateFix=0"
```

## 단계와 화면

| 단계 | 대책 | 화면에서 본 것(아래 이미지의 줄) |
| --- | --- | --- |
| L0 | 상태(걷기·피격·죽기)만 맞춘다 | 미니언이 한 프레임 사라지고 번진 뒤, 다른 자세로 나타난다. 액터의 애니가 클립 처음부터 시작한다 |
| L1 | + 시작 시각 맞추기(AnimBP Start Position을 `StartTime`에 묶음, `Scripts/bind-anim-start.md`) | 자세는 이어진다. 붙는 순간의 빈 프레임과 번짐은 남는다 |
| L2 | + 준비될 때까지 VAT 유지(`bWaitForActorVisualReadiness`, 그동안 액터를 `SetActorHiddenInGame`으로 숨김), 되돌아갈 때 한 프레임 더(`bKeepActorExtraFrame`) | 빈 프레임은 사라지지만, 액터가 보이는 순간부터 2\~3프레임 번진다. 숨기면 렌더 프록시가 장면에서 빠져, 다시 보일 때 지난번 위치·뼈가 이전 프레임으로 쓰인다(모션 블러) |
| L3 | 숨김 대신 메인·깊이 패스와 그림자에서만 뺀다 | 첫 번짐은 사라지고, 2프레임 세로 줄무늬가 남는다. 엔진이 액터를 엔티티 발밑 높이(z=0)로 옮겨(`MassRepresentationProcessor.cpp:272, 428`) 88cm 가라앉았다 올라온다 |
| L4 | + 엔진 이동에도 캡슐 반높이를 더한다(`ULabActorManagement::TeleportActor`) | 전환이 보이지 않는다 |

![사건 A](levels-event-a.png)

![사건 B](levels-event-b.png)

각 줄은 왼쪽부터 사건 2프레임 전 → 5프레임 뒤다. 위에서부터 L0, L1, L2, L3, L4.

## 남은 것

- 액터 → VAT 인스턴스(되돌아가는 쪽)는 이 장면에서 드물어(10초 사이 0회) 같은 방식으로 찍지 못했다.
- 영상은 찍지 않았다(녹화는 사용자 승인 뒤).
