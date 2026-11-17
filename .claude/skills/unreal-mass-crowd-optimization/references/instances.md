# 인스턴스와 입자를 쓸 때

Mass 표현의 인스턴스(ISM, VAT 인스턴스, ISKM)와 Niagara 입자를 다루는 아홉 항목이다.

- 항목은 증상, 설명, 경고·주의, 절차, 코드, 결과의 순서다. **경고**는 어기면 결과가 틀려지는 것이다. **주의**는 시간을 버리거나 부작용이 있는 것이다.
- 결과의 수치는 `results.md`의 조건(UE 5.8.4, Ryzen 9 5950X와 RTX 4090, Test 구성 패키지, 1만 개체, 프레임 p95, 세 번 잰 중앙값)에서 잰 참고값이다. "끈 조건"은 최종 구성에서 그 기법만 끈 것이다.
- 코드의 게임 클래스와 변수 이름은 예시 이름이다. 엔진 소스 위치는 UE 5.8.4 기준이다. 줄임말 **MR** = `Engine/Plugins/Runtime/MassGameplay/Source/MassRepresentation`.
- 참고 장면의 거리 구간: 0\~35m 액터, 35\~80m 인스턴스(그림자 켬), 80\~200m 인스턴스(그림자 끔), 200m 밖 None(입자로 그림).

## 목차

1. 먼 개체는 화면 밖 여부보다 거리로 먼저 지운다
2. 움직이는 인스턴스는 Lumen 장면에서 뺀다
3. Mass 인스턴스의 그림자는 설명자에서 켠다
4. ISKM 컴포넌트에 카메라를 따라가는 고정 바운드를 넣는다
5. 움직이는 ISKM 엔티티는 파괴하지 말고 풀로 다시 쓴다
6. ISKM 트랙 풀은 처음에 용량까지 늘려 둔다
7. ISKM에는 가산 클립 대신 가산을 끈 복제본을 넣는다
8. AnimToTexture Bone 방식 메시의 바운드는 정점 범위로 넓힌다
9. Niagara 메시 입자의 VAT 프레임은 Dynamic Parameter로 넘긴다

## 1. 먼 개체는 화면 밖 여부보다 거리로 먼저 지운다

**증상**: 먼 개체를 인스턴스로 그리는 비용이 화면에 보이는 수를 따르지 않는다. 표현을 가진 개체 수를 따라 는다.

**설명**
- 엔진 표현 프로세서는 표현이 None이 아닌 개체를 화면 밖이어도 `CanBeSeen`으로 둔다.
- 그래서 인스턴스 갱신 프로세서의 청크 필터(`AreAnyEntitiesVisibleInChunk`)는 그 청크를 거르지 않는다.
- 표현이 None인 개체는 `CulledByDistance` 청크로 모인다. 표현 프로세서는 그 청크를 0.5초(±10%)마다만 다시 본다(`MR/Private/MassRepresentationProcessor.cpp`, `MR/Private/MassVisualizationLODProcessor.cpp`).

**주의**: Off에서 들어오는 개체는 히스테리시스 때문에 80\~200m 구간의 90% 지점(188m)에서 그려진다. 청크 갱신 간격만큼 늦게 나타나기도 한다. 알려진 동작이다.

**절차**
1. 표현 LOD의 Off 구간에 `EMassRepresentationType::None`을 준다.
2. Off 거리를 둔다.

```cpp
// 예시: 엔티티 설정을 코드로 만드는 자리
const float Bands[EMassLOD::Max] = { 0.f, 3500.f, 8000.f, 20000.f }; // 0, 35, 80, 200m
// ...
Visual->Params.LODRepresentation[EMassLOD::Off] = EMassRepresentationType::None;
```

**결과**
- 200m 밖을 그리지 않는 일만 끈 조건(200m 밖도 맵 끝까지 ISKM)은 프레임 p95가 3.54ms, 게임 스레드 시간이 3.23ms 길었다.
- 화면 밖만 지우는 변형은 앞선 시험에서 이득이 없었다. 원인은 원인 미확정이다.

## 2. 움직이는 인스턴스는 Lumen 장면에서 뺀다

**증상**: 움직이는 정지 메시 인스턴스 수천 개를 그리면 GPU 시간이 크게 는다.

**설명**
- 소프트웨어 Lumen은 `AffectsDynamicIndirectLighting()`이 참이고 거리 필드를 가진 프리미티브를 장면에 넣는다. 거리 필드는 정지 메시, Nanite, 스플라인만 가진다.
- 인스턴스가 움직인 프리미티브는 매 프레임 Lumen 장면 갱신 목록에 든다.
- 스켈레탈 메시는 원래 Lumen 장면 밖이다. 그래서 이 비용은 액터 군중에서는 드러나지 않다가 인스턴스로 바꿀 때 나타난다.

**주의**: Mass 비주얼라이저는 컴포넌트를 처음 쓰일 때 늦게 만든다. 그래서 아래 절차는 컴포넌트를 매 틱 훑는다.

**절차**
1. Mass 비주얼라이저(`AMassVisualizer`)가 만든 ISM·ISKM 컴포넌트마다 `SetAffectDynamicIndirectLighting(false)`를 부른다.
2. 매 틱 비주얼라이저의 컴포넌트를 훑는다.
3. 아직 고치지 않은 컴포넌트만 고친다.

```cpp
// 예시: 월드 서브시스템의 틱
for (TActorIterator<AMassVisualizer> It(GetWorld()); It; ++It) // 컴포넌트는 늦게 생기므로 매 틱 훑는다
{
	// ... ISM·ISKM 컴포넌트 가운데 bAffectDynamicIndirectLighting이 아직 켜진 것만
	Component->SetAffectDynamicIndirectLighting(false);
}
```

**결과**
- 개체는 계속 Lumen의 간접광을 받고 그림자를 그린다.
- 초기 시험(Test 패키지, 움직이는 ISM 2,000개)에서 GPU 시간이 29.0ms에서 6.0ms로 줄었다.
- 어느 Lumen 패스가 시간을 썼는지는 GPU 트레이스로 보지 않았다.

## 3. Mass 인스턴스의 그림자는 설명자에서 켠다

**증상**: 하이드레이션 액터는 그림자를 그린다. 인스턴스로 바뀐 개체는 그림자가 없다.

**설명**
- ISM 설명자(`FMassStaticMeshInstanceVisualizationMeshDesc`)와 ISKM 설명자(`FMassSkinnedMeshInstanceVisualizationMeshDesc`)는 `bCastShadows`의 기본값이 거짓이다(`MR/Public/MassRepresentationTypes.h:105`, `MR/Public/MassSkinnedMeshRepresentationTypes.h:79`).
- 엔진은 이 값을 설명자마다 만드는 컴포넌트의 `SetCastShadow`로만 넘긴다. 인스턴스 단위 그림자 플래그는 없다.
- `bCastShadows`는 설명자 해시에 든다. 그래서 그림자만 다른 두 설명자는 컴포넌트가 둘이 된다.
- 설명자에는 LOD 중요도 범위(`MinLODSignificance`, `MaxLODSignificance`, `SetSignificanceRange`)가 있다. 그림자를 켠 설명자와 끈 설명자를 범위로 나눠 먼 구간의 그림자만 끌 수 있다. 범위 경계 2.0이 표현 LOD Medium/Low 경계와 같다(`MR/Private/MassVisualizationComponent.cpp`).

**절차**
1. 설명자를 만들 때 `bCastShadows`를 참으로 둔다.
2. 먼 구간의 그림자를 끄려면 그림자를 끈 같은 메시의 설명자를 하나 더 두고 중요도 범위로 나눈다.

```cpp
MeshDesc.bCastShadows = true;    // 엔진 기본값은 false(MassRepresentationTypes.h:105)
SkinnedDesc.bCastShadows = true; // 엔진 기본값은 false(MassSkinnedMeshRepresentationTypes.h:79)
```

**결과**
- 최종 구성에서 80m 밖 그림자를 다시 켠 조건은 GPU 시간이 0.48ms 길었다.
- 병목이 게임 스레드라 프레임 p95는 구별되지 않았다.

## 4. ISKM 컴포넌트에 카메라를 따라가는 고정 바운드를 넣는다

**증상**: ISKM 인스턴스가 매 프레임 움직이면 컴포넌트 바운드 계산이 인스턴스 수를 따라 는다.

**설명**
- ISKM 컴포넌트는 인스턴스가 바뀐 프레임마다 `UpdateBounds()`를 부른다.
- 바운드 계산은 `bFastBuild = false`로 박혀 있다. 그래서 인스턴스마다 애니메이션 바운드를 변환해 합친다.
- `SetPrimitiveBoundsOverride(FBox)`로 상자를 넣으면 이 반복 없이 그 상자를 돌려준다.
- 인스턴스 컬링과 GPU LOD 선택은 인스턴스 데이터로 한다. 상자는 컴포넌트 단위 컬링에만 쓰인다.
- 인스턴스가 매 프레임 움직이는 컴포넌트는 다음 갱신 때 새 바운드를 렌더 스레드로 보낸다. 렌더 상태를 다시 만들 필요가 없다.

**절차**
1. 카메라 둘레에 상자를 넣는다. 상자는 그 표현 구간(참고 장면은 200m)을 덮는다.
2. 카메라가 정한 거리보다 멀리 움직이면 상자를 옮긴다.
3. 옮긴 틱에 상자를 다시 넣는다.

```cpp
// 예시 이름: BoundsCenter(카메라를 따라 옮기는 중심), HalfXY(표현 구간 반경 + 여유)
const FBox Fixed(BoundsCenter + FVector(-HalfXY, -HalfXY, -500.f), BoundsCenter + FVector(HalfXY, HalfXY, 1500.f));
Component.SetPrimitiveBoundsOverride(Fixed); // 상자를 옮긴 틱에도 다시 넣는다
Component.UpdateBounds();
```

**결과**: ISKM 고정 바운드만 끈 조건은 프레임 p95가 0.66ms 길었다.

## 5. 움직이는 ISKM 엔티티는 파괴하지 말고 풀로 다시 쓴다

**증상**: 개체가 죽고 다시 생기는 동안 ISKM 트랙 수와 GPU 메모리가 실행 내내 는다.

**설명**
- Mass는 엔티티가 ISKM 컴포넌트에 처음 들 때 엔티티 핸들을 키로 트랙을 잡는다.
- 트랙을 돌려받는 `DeallocateTrack`은 정지 표현 경로에서만 불린다. 그 경로는 `FMassStaticRepresentationTag`를 요구한다(`MR/Private/MassVisualizationComponent.cpp` 773·864행 할당, 1156행 해제).
- 그래서 움직이는 엔티티를 파괴하면 트랙이 활성인 채 남고, 렌더러는 남은 트랙도 매 프레임 평가한다.
- 이 문제는 5.8.4에도 남아 있다. [Epic 개발자 포럼](https://forums.unrealengine.com/t/mass-crowds-using-skinnedmeshinstance-leak-gpu-memory-every-wave-of-spawned-and-killed-entities-permanently-grows-the-animation-track-pool-5-8/2742508)에 같은 보고가 있다.
- 풀에서 꺼낸 엔티티는 핸들이 같다. 그래서 엔진은 옛 트랙을 다시 쓴다.

**경고**: 엔진은 트랙이 있으면 클립 번호가 바뀔 때만 재생 데이터를 쓴다. 풀에서 꺼낸 엔티티가 같은 클립을 처음부터 재생해야 하면 직접 써야 한다.

**절차**
1. 죽은 엔티티를 파괴하지 않고 종류별 풀에 넣는다.
2. 재생성 때 풀에서 엔티티를 꺼낸다.
3. 꺼낸 엔티티에 위치, 체력, 상태를 다시 넣는다.
4. 상태나 시작 시각이 바뀐 프레임에 `SetAutoPlayData`를 직접 부른다.

```cpp
// 예시 이름: EntityPool(종류별 TArray<FMassEntityHandle>), ReinitPooledEntity
const int32 NumReuse = FMath::Min(EntityPool[Type].Num(), Indices.Num());
for (int32 k = 0; k < NumReuse; ++k)
{
	const FMassEntityHandle Entity = EntityPool[Type].Pop(EAllowShrinking::No);
	ReinitPooledEntity(EntityManager, Entity, Locations[Indices[k]]); // 위치, 체력, 상태를 다시 넣는다
}
```

**결과**
- 240초 실행 끝의 트랙은 엔티티 풀을 켜면 20,215개, 끄면 30,797개였다. 프레임 p95는 구별되지 않았다.
- 960초씩 돌린 참고 측정에서 엔티티 풀을 끈 쪽의 트랙은 120,428개가 됐다. GPU 메모리는 끈 쪽 5,110MB, 켠 쪽 3,370MB였다.
- 엔티티 풀을 끈 쪽은 13분 구간부터 프레임 p95가 0.6ms 넘게 높았다.

## 6. ISKM 트랙 풀은 처음에 용량까지 늘려 둔다

**증상**: ISKM으로 그린 같은 종류의 개체 전체가 가끔 한 프레임 세로로 길게 끌린다. 트랙 수가 느는 동안에만 난다.

**설명**
- 모든 개체가 트랙을 가지면 이 일은 멈춘다.
- 트랙 수가 바뀌면 렌더러는 그 컴포넌트의 뼈대 버퍼(트랙 수 × 뼈 수 × 2)를 다시 잡는다. 옛 자리를 먼저 해제하고 새 자리를 할당한다.
- `FSpanAllocator`는 맞는 빈 구간이 없으면 해제한 구간을 이웃과 합친다. 그래서 옛 자리와 겹치는 자리를 줄 수 있다.
- 옛 내용을 옮기는 셰이더는 같은 버퍼를 병렬로 읽고 쓴다. 그래서 이전 프레임 자세 칸이 깨지고, 모션 블러가 그 칸으로 틀린 속도를 그린다.

**주의**: 트랙이 용량을 넘으면 이 일은 다시 생긴다.

**절차**
1. 컴포넌트가 처음 쓰이는 프레임에 트랙을 용량(종류의 개체 수 × 1.1 + 64)만큼 할당한다.
2. 할당한 트랙을 모두 해제한다.
3. 5항목의 엔티티 풀을 함께 쓴다.

```cpp
// Provider: 그 컴포넌트의 UAnimSequenceTransformProviderDataInstance
TArray<int32> Temp;
while (int32(Provider->GetUniqueAnimationCount()) < Capacity) { Temp.Add(Provider->AllocateTrack()); }
for (const int32 TrackIndex : Temp) { Provider->DeallocateTrack(TrackIndex); } // 끝 번호는 남는다
```

**결과**
- 트랙 풀의 끝 번호가 남아 트랙 수는 다시 바뀌지 않는다. 다음 할당은 빈 번호를 다시 쓴다.
- 속도 버퍼를 찍은 Development 빌드 900프레임 캡처에서 튄 프레임이 42개에서 0개가 됐다.
- 이 처리만 끈 조건의 프레임 p95는 구별되지 않았다.

## 7. ISKM에는 가산 클립 대신 가산을 끈 복제본을 넣는다

**증상**: ISKM 개체가 피격 클립을 재생하는 동안 사라지거나 누운 모양이 된다.

**설명**
- ISKM 트랙 제공자(`UAnimSequenceTransformProviderData`)의 렌더러 경로는 압축을 푼 지역 자세를 그대로 뼈 변환으로 쓴다. 기준 자세를 읽는 코드가 없다.
- 가산 클립이면 이 값은 기준 자세와의 차이다. 그래서 뼈가 원점 근처로 모인다.
- 액터의 단일 노드 재생과 AnimToTexture 굽기는 기준 자세에 차이를 더한다. 그래서 같은 클립이 액터와 VAT 인스턴스에서는 맞다.
- 엔진은 원본 데이터를 늘 전체 자세로 두고, 가산 값은 압축할 때 만든다.

**절차**
1. 에디터 스크립트에서 가산 클립을 복제한다.
2. 복제본의 가산 종류를 `AAT_None`으로 둔다.
3. 그 복제본을 트랙 제공자에 넣는다.

```python
# Python Editor Script. eal = unreal.EditorAssetLibrary
if kind == unreal.AdditiveAnimationType.AAT_NONE:
    return anim
full = eal.duplicate_asset(src_path, path)
full.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)  # PostEditChange가 다시 압축한다
```

**결과**: 엔진은 복제본을 기준 자세에 차이를 더한 전체 자세로 다시 압축한다. 화면에서 피격 클립이 액터와 같게 보인다.

## 8. AnimToTexture Bone 방식 메시의 바운드는 정점 범위로 넓힌다

**증상**: VAT 인스턴스가 죽기 애니메이션을 재생하는 동안 무기 끝 같은 먼 정점의 그림자가 바닥에 남는다.

**설명**
- AnimToTexture의 Bone 모드는 정지 메시 바운드를 뼈 위치 범위(`BoneMinBBox`, `BoneSizeBBox`)로만 넓힌다. Vertex 모드는 모든 프레임의 정점 범위로 넓힌다(`AnimToTextureBPLibrary.cpp`).
- Virtual Shadow Map은 물체 바운드 안쪽의 페이지만 무효화한다. 그래서 바운드 밖으로 나간 정점의 그림자가 캐시에 남는다.
- 셰이더로 정점을 옮기는 기법(VAT, WPO)은 바운드를 직접 챙겨야 한다.

**주의**: 그림자 캐시를 매 프레임 무효화하는 설정(`ShadowCacheInvalidationBehavior::Always`)도 그림자를 지운다. 비용이 커서 쓰지 않았다.

**절차**
1. 같은 메시를 Vertex 모드로 먼저 굽는다.
2. 구운 결과에서 정점 범위(`VertexMinBBox`, `VertexSizeBBox`)를 얻는다.
3. Bone 메시의 `PositiveBoundsExtension`·`NegativeBoundsExtension`을 그 범위까지 넓힌다.

```python
# Python Editor Script. vmin, vsize: 먼저 구운 Vertex 모드의 정점 범위
new_max = unreal.Vector(max(base_max.x, vmin.x + vsize.x), max(base_max.y, vmin.y + vsize.y), max(base_max.z, vmin.z + vsize.z))
sm.set_editor_property("positive_bounds_extension", unreal.Vector(new_max.x - base_max.x, new_max.y - base_max.y, new_max.z - base_max.z))
# negative_bounds_extension도 min으로 같은 식을 쓴다
```

**확인**: 죽기 애니메이션의 정지 프레임에서 바닥의 그림자 잔상을 본다.

## 9. Niagara 메시 입자의 VAT 프레임은 Dynamic Parameter로 넘긴다

**증상**: ISM에서 맞게 걷던 VAT 머티리얼을 Niagara 메시 입자에 쓰면 프레임이 엉뚱하게 나온다.

**설명**
- Niagara 메시 렌더러는 GPU Scene 경로에서 인스턴스 커스텀 데이터 float 하나를 잡고, 그 0번에 입자 인덱스의 비트를 쓴다.
- 그래서 `PerInstanceCustomData`로 프레임을 읽는 VAT 머티리얼은 Niagara에서 입자 인덱스를 읽는다.
- AnimToTexture 머티리얼 함수 `GetFrameSwitch`는 정적 스위치 `AutoPlay`와 `UseDynamicParameters`를 켜면 프레임을 Dynamic Material Parameter 1번에서 읽는다. 1번의 네 값은 TimeOffset, Playrate, StartFrame, EndFrame이다.

**절차**
1. 머티리얼 인스턴스에서 정적 스위치 `AutoPlay`와 `UseDynamicParameters`를 켠다.
2. 이미터에 `DynamicMaterialParameters` 모듈을 더한다.
3. 그 모듈의 쓰기 인덱스 0을 끄고 쓰기 인덱스 1을 Float로 둔다.
4. 인덱스 1에 네 값을 넣는다.

```text
MI 정적 스위치: AutoPlay = true, UseDynamicParameters = true
DynamicMaterialParameters: Write Parameter Index 0 = Off, Write Parameter Index 1 = Float
Index 1 Param 1..4 = TimeOffset, Playrate, StartFrame, EndFrame
```

**결과**: Playrate를 0으로, StartFrame을 죽기 클립 안에 둔 화면에서 모든 입자가 같은 쓰러진 자세로 멈췄다. 이 화면으로 네 값이 제자리에 들어간 것을 확인한다.
