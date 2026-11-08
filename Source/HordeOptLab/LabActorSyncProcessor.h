#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassRepresentationActorManagement.h"
#include "LabActorSyncProcessor.generated.h"

/** 하이드레이션 액터가 포즈를 맞춘 뒤에만 "준비됨"을 엔진에 알린다. 그 전까지 엔진이 VAT 인스턴스를 계속 그린다(bWaitForActorVisualReadiness). */
UCLASS()
class ULabActorManagement : public UMassRepresentationActorManagement
{
	GENERATED_BODY()
protected:
	virtual bool IsActorReadyForRepresentation(const AActor& Actor) const override;
	virtual EMassActorSpawnRequestAction OnPostActorSpawn(const FMassActorSpawnRequestHandle& SpawnRequestHandle, FConstStructView SpawnRequest, TSharedRef<FMassEntityManager> EntityManager) const override;
};

/**
 * M3 액터 동기화. 0~30m에서 액터로 그리는 엔티티마다 매 프레임 액터 위치, 애니 상태, 죽기 색을 쓴다(plan.md "M3", 2026-11-08 사용자 결정).
 * 엔진은 액터를 붙이는 순간에만 위치를 맞춘다(MassRepresentationProcessor.cpp:272).
 *
 * 애니 시각: 엔티티가 새로 붙은 액터(또는 풀에서 다시 쓴 액터)면 VAT 인스턴스가 재생하던 시각을 StartTime에 넣고
 * 애니 인스턴스를 다시 초기화해 그 시각부터 재생한다(AnimBP 시퀀스 플레이어 Start Position이 StartTime에 묶여 있다, Scripts/bind-anim-start.md).
 * 그 뒤 상태가 바뀌면 StartTime을 0으로 두고 상태만 바꾼다(블렌드 리스트가 다시 활성화된 클립을 처음부터 재생한다).
 * 게임 스레드에서 돈다(액터 API).
 */
UCLASS()
class ULabActorSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabActorSyncProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
};
