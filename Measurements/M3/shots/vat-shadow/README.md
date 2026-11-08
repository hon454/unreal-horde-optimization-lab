# VAT 인스턴스의 남는 그림자: 원인 찾기와 전후 기록

2026-11-08, 개발 빌드. 사용자가 실행 화면을 보고 찾았고, 같은 조건의 창을 번호를 붙여 차례로 띄워 원인을 좁혔다. 경위는 [post-material.md](../../../../Docs/post-material.md) "03 ISM + VAT"의 "죽는 VAT 인스턴스의 칼날 그림자가 바닥에 남았다".

## 증상

화면 어디에서나, 죽기 애니메이션을 재생하는 VAT 인스턴스의 칼날 끝 쪽 그림자 여러 개가 바닥에 남겨지듯 보이다가, 인스턴스가 지워지면 함께 사라졌다(사용자 관찰). 정지 캡처(1280x720)로는 잘 보이지 않아 실행 화면으로 판단했다.

## 갈라 본 순서(모두 N=1000, P1, 60초, 사용자가 보고 판단)

| 창 | 조건 | 남는 그림자 | 알게 된 것 |
| --- | --- | --- | --- |
| 1 | M3 그대로 | 거의 안 보임 | 가까운 쪽에서 죽는 미니언은 액터(스켈레탈 메시)라 그림자가 제대로 갱신된다 |
| 2 | M3, Virtual Shadow Map 캐시 끔(`-dpcvars=r.Shadow.Virtual.Cache=0`) | 없음 | |
| 3 | M2(액터 없음) | 있음 | VAT 인스턴스 쪽 문제 |
| 4 | M2 + `ShadowCacheInvalidationBehavior = Always` | 있음 | "움직임을 모르는 물체" 문제가 아니다 |
| 5 | M2, 캐시 끔 | 없음 | 원인은 그림자 캐시가 맞다 |
| 6 | M2 + VAT 메시 바운드 수정 | 없음 | 수정 확인 |

## 원인

Virtual Shadow Map은 캐시한 그림자 페이지를 물체의 **바운드** 안쪽에서만 무효화한다(4번의 `Always`도 바운드 기준이라 듣지 않았다). AnimToTexture는 Bone 모드에서 정적 메시 바운드를 **뼈 위치의 범위**로만 넓힌다(`Engine/Plugins/Experimental/AnimToTexture/Source/AnimToTextureEditor/Private/AnimToTextureBPLibrary.cpp:290` `SetBoundsExtensions(..., BoneMinBBox, BoneSizeBBox)`). 손뼈에서 멀리 뻗은 칼날 끝 정점은 이 상자 밖으로 나가, 쓰러지는 동안 상자 밖에 그려진 칼날 그림자 페이지가 무효화되지 않고 남았다.

## 바운드 전후(로컬, cm, `Scripts/bake-vat.py` 로그)

| | X | Y | Z |
| --- | --- | --- | --- |
| 수정 전(Bone 모드, 뼈 위치 범위) | -93 \~ 65 | -72 \~ 138 | -74 \~ 176 |
| 수정 후(모든 프레임의 정점 범위, Vertex 굽기 값) | -212 \~ 118 | -276 \~ 138 | -89 \~ 185 |

Y 음수 쪽(칼날이 앞으로 떨어지는 방향)이 204cm 모자랐다.

## 수정

`Scripts/bake-vat.py`: Vertex 모드를 먼저 구워 엔진이 계산한 모든 프레임의 정점 범위(`VertexMinBBox`, `VertexSizeBBox`)를 얻고, Bone 메시의 바운드 확장을 그 범위로 둔다. `LAB_VAT_TIGHT=1`로 구우면 수정 전 바운드를 다시 만들 수 있다(전후 비교용).

`Always`는 매 프레임 그림자를 다시 그려 비용이 커서 되돌렸다(`ULabCrowdSubsystem::ExcludeVisualsFromLumen`).

## 남은 것

- 남는 그림자를 보이는 화면 자료(전후 영상 또는 고해상도 연속 캡처)는 아직 없다. 영상 녹화는 사용자 승인 뒤.
- 바운드가 커져 컬링·그림자 비용이 조금 늘 수 있다. E3 측정(M2 + Off, M3)이 이 수정 뒤의 빌드로 잰다.
