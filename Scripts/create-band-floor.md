# M_LabBandFloor 만들기 (M3b, 결정 6)

개발 빌드에서 오브 중심 거리 띠(30/80/200m)를 바닥에 그리는 머티리얼 `/Game/Lab/Materials/M_LabBandFloor`를 만든 순서다(2026-11-09). 측정 빌드는 기본 바닥(`MI_ProcGrid`)을 그대로 쓴다. 띠의 이유와 버린 대안은 [backlog.md](../Docs/backlog.md) "바닥 밴드 표시".

## 만들기

에셋은 Python 스크립트 하나로 만든다(에디터가 떠 있어도 된다. 새 패키지만 쓴다). Unreal MCP 없이 다시 만들 수 있게 그래프를 MCP 호출 대신 스크립트에 두었다.

```bash
"G:/Epic Games/UE_5.8.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/HordeOptLab.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/create-band-floor.py" -unattended -nosplash -AllowCommandletRendering -stdout
```

로그에 `LABBAND saved /Game/Lab/Materials/M_LabBandFloor… params=…`가 찍힌다. 스크립트([create-band-floor.py](create-band-floor.py))가 하는 일:

1. 기존 에셋이 있으면 지우고 빈 머티리얼을 만든다.
2. Custom 노드 하나(`LabBandFloor`, 출력 float3)에 띠 계산 HLSL을 넣는다. `d = length(WP.xy - Center.xy)`로 오브에서의 XY 거리를 재고, `d < R1`이면 ColorNear, `< R2`면 ColorMid, `< R3`면 ColorFar, 그 밖은 ColorOff를 고른다. 경계 `R1·R2·R3`에서 `LineWidth` 폭의 선을 LineColor로 섞고, `GridSize` 간격 격자를 옅게 어둡게 한다. 바닥 회색 `BaseGray`에 구간 색을 곱한다.
3. 입력: Absolute World Position(`WP`)과 아래 파라미터 노드들을 Custom 노드 입력에 잇는다.
4. Custom 출력 → Base Color, 상수 0.9 → Roughness. 컴파일하고 저장한다.

## 파라미터(코드가 넣는다)

| 이름 | 종류 | 기본값 | 뜻 |
| --- | --- | --- | --- |
| `BandCenter` | Vector | (0, 0, 0) | 오브 월드 위치(cm). XY만 쓴다 |
| `BandRadius1` | Scalar | 3000 | 첫 경계(cm). 액터 ↔ VAT, 저격 구간 1 |
| `BandRadius2` | Scalar | 8000 | 둘째 경계(cm). 저격 구간 2 |
| `BandRadius3` | Scalar | 20000 | 셋째 경계(cm). M3부터 Off |
| `ColorNear`·`ColorMid`·`ColorFar`·`ColorOff` | Vector | 주황 (1, 0.62, 0.32), 초록 (0.45, 0.9, 0.5), 파랑 (0.45, 0.6, 1), 흰색 | 구간 색. 바닥 회색에 곱한다 |
| `LineColor`·`LineWidth` | Vector·Scalar | (0.9, 0.9, 0.9), 40 | 경계선 색과 폭(cm) |
| `GridSize`·`BaseGray` | Scalar | 1000, 0.35 | 격자 간격(cm)과 바닥 회색 |

코드(`-LabBandFloor`, 코드 세션 몫)는 Floor(`L_Bench`의 `Floor_0`, `StaticMeshComponent0`)에 이 머티리얼의 `UMaterialInstanceDynamic`을 넣고 `BandCenter`에 오브 위치를 넣는다. 반경 기본값이 `FLabSnipeBands`와 같으므로 반경은 넣지 않아도 된다(넣으면 저격 구간과 어긋나지 않게 같은 상수에서 넣는다).

## 확인

에디터가 `L_Bench`를 연 상태에서 Unreal MCP로 확인했다(HTTP 직접 호출, [commands.md](../Docs/Guides/commands.md) "Unreal MCP를 HTTP로 직접 부르기"). `ObjectTools.set_properties`로 `Floor_0.StaticMeshComponent0`의 `OverrideMaterials`를 `M_LabBandFloor`로 바꾸고, `EditorAppToolset.CaptureViewport`로 탑뷰(0, 0, 260m, pitch −89.9)와 오브 뒤 낮은 시점(−15m, 0, 9m, pitch −15)을 찍은 뒤 `MI_ProcGrid`로 되돌렸다(레벨은 저장하지 않았다). 탑뷰에서 세 원의 반지름 비가 30 : 80 : 200과 맞았다. 캡처: `Saved/Screenshots/Lab/m3b_bandfloor_top.png`, `m3b_bandfloor_p1.png`(git 밖).
