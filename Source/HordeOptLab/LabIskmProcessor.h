#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "LabIskmProcessor.generated.h"

/**
 * M5 ISKM 갱신(plan.md "M5" 결정 2·4). M5 ISKM 변형에서 ULabVatISMProcessor 자리에 하나만 돈다(HordeOptLab.cpp).
 * 엔진 표현 경로(UMassMovableVisualizationTrait, 표현 LOD, 액터 전환)는 그대로고, 엔진 ISKM 프로세서 둘(UMassUpdateInstancedSkinnedMeshProcessor,
 * UMassConsumeInstancedSkinnedMeshAnimationProcessor)은 쓰지 않는다. 두 프로세서는 위치와 애니 데이터를 같은 배열에 순서로만 맞춰 쌓는데
 * 쿼리와 청크 필터가 달라 순서가 맞는다는 보장이 없다(MassVisualizationComponent.cpp:704-754, Docs/Worklog/M5.md "설계 조사").
 *
 * 한 반복에서 엔티티마다 위치(AddBatchedTransform), 애니 데이터(AddBatchedAnimationData), 커스텀 데이터 0번 = 죽기 색 정도(AddBatchedCustomData)를 함께 쌓는다.
 * 구조는 엔진 UMassUpdateInstancedSkinnedMeshProcessor(MassUpdateSkinnedMeshProcessor.cpp:42-89)와 ULabVatISMProcessor를 따른다.
 * - 액터 준비 대기(bWaitForActorVisualReadiness) 동안 엔진은 PrevRepresentation을 고정하고 ISKM을 떼지 않는다(MassRepresentationProcessor.cpp:119-121, 291).
 *   그동안도 위치와 애니를 계속 써서 인스턴스가 멈추거나 빠지지 않게 한다(VAT 프로세서와 같다).
 * - PrevTransform·PrevLODSignificance는 ISKM으로 그린 엔티티에만 쓴다(엔진 ISKM 프로세서와 같다). 액터 → ISKM 전환 첫 프레임의 PrevTransform은 엔진이 채운다(:365).
 *
 * 트랙(plan.md 결정 4): 엔티티마다 트랙 하나. 엔진은 트랙이 없으면 만들어 그 프레임의 애니 데이터로 쓰고, 이미 있으면 시퀀스 번호가 바뀔 때만 다시 쓴다
 * (MassVisualizationComponent.cpp:761-775). 같은 걷기 클립인데 시작 시각만 다른 경우(풀에서 다시 나온 개체가 이전 생의 걷기 트랙을 가진 때)는 반영되지 않으므로,
 * 표현 상태(FLabAnimStateFragment)가 FLabIskmTrackFragment에 적어 둔 마지막 값과 다를 때만 이 프로세서가 트랙 번호로 SetAutoPlayData를 직접 부른다.
 * 위상: Position = 지금 − StateStartTime, 엔진 기준 시각 = 월드 시각 − Position / PlayRate라 VAT의 무상태 식과 같다(engine-notes.md "ISKM 트랙과 위상").
 */
UCLASS()
class ULabIskmProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabIskmProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};
