#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabMinion.h"
#include "LabOverlap.h"
#include "LabFarStream.h"
#include "LabCrowdSpawner.generated.h"

/**
 * 시작 시 디스크 전체에 정상 상태 분포(반지름당 개체 수 일정, 밀도 ∝ 1/r)로 N개를 놓고,
 * 사라진 개체는 가장자리 링에서 다시 만든다. 초기 위치와 재생성 순서는 시드로 재현한다. plan.md "M0".
 */
UCLASS()
class ALabCrowdSpawner : public AActor
{
	GENERATED_BODY()
public:
	ALabCrowdSpawner();

	void SpawnInitial();
	void OnMinionFinished(ALabMinion* Minion);
	/**
	 * 단계 2부터: 지워진 엔티티 수만큼 새로 만든다(종류별 수, LabUnitAssets.h). ULabCrowdSubsystem이 부른다.
	 * M6 수정 A(FLabConfig::bFarHandoff): 같은 종류에서 오브에 가장 가까운 바깥 기록의 자리·일련번호로 만든다. 끄면 가장자리 링의 무작위 자리(종류는 일련번호 열이 정한다)
	 */
	void RespawnEntities(const int32 (&CountByType)[LabUnitTypeMax]);
	/** M6 수정 A: 바깥 기록을 옮긴다. 게임 모드 틱이 부른다(모든 단계) */
	void TickFarStream(float DeltaSeconds);
	/** M6 수정 A의 바깥 기록. 꺼져 있으면 nullptr. 모으기 프로세서(게임 스레드)가 읽는다 */
	const FLabFarStream* GetFarStream() const { return FarStream.IsActive() ? &FarStream : nullptr; }

	/** 오브의 지금 지면 위치. Spike1 궤도(FLabOrbit)를 켜지 않으면 원점이다 */
	FVector GetOrbLocation() const;
	const TArray<TObjectPtr<ALabMinion>>& GetMinions() const { return Minions; }
	/** 단계와 무관한 개체 수(M0 액터, M1부터 엔티티). 러너의 LabAlive와 오버레이가 쓴다. */
	int32 GetAliveCount() const;
	int32 GetRespawnCount() const { return RespawnCount; }

	/**
	 * v2 W6: M0 액터의 겹친 쌍(LabOverlap.h, Mass의 ULabOverlapCountProcessor와 같은 정의). 게임 모드 틱이 매 프레임 부르고 LabOverlapSampleSec마다 센다.
	 * 몸통은 액터 위치(캡슐 중심)의 수평 원이고 반지름은 종류별 BoundRadius(소형 40, 대형 94)다. 사망 중인 개체는 뺀다
	 */
	void SampleActorOverlaps(double Now);
	int32 GetActorOverlapPairs() const { return ActorOverlapPairs; }
	int32 GetActorOverlapDeep() const { return ActorOverlapDeep; }

protected:
	ALabMinion* SpawnAt(float Radius, float Angle);
	/** 땅 위 자리와 일련번호로 액터를 만든다(SpawnAt과 M6 수정 A의 단계 1 재생성) */
	ALabMinion* SpawnActorAt(const FVector& Ground, uint32 Serial);
	void LoadAssets();
	void SpawnEntitiesAt(TConstArrayView<FVector2D> RadiusAngles);
	/** M6 수정 A: 종류 Type의 맨 앞 기록 Count개를 꺼내 Out에 더한다. 기록이 모자라면 가장자리 링에서 옛 규칙으로 채운다 */
	void PopFarRecords(int32 Type, int32 Count, TArray<FVector>& OutLocations, TArray<uint32>& OutSerials);

	UPROPERTY() FLabMinionAssets Assets;      // 소형(기본)
	UPROPERTY() FLabMinionAssets LargeAssets; // 2종 대형(-LabTypes=2). 대형 에셋이 하나라도 없으면 Assets와 같다(대체, 스모크 전용. LabUnitAssets.h)
	UPROPERTY() TArray<TObjectPtr<ALabMinion>> Minions;

	UPROPERTY() TObjectPtr<class ULabCrowdSubsystem> Crowd; // 단계 2부터
	FRandomStream Stream;
	uint32 NextSerial = 0;
	int32 RespawnCount = 0;
	TArray<FLabBody> OverlapBodies; // v2 W6 M0 겹친 쌍 표본(SampleActorOverlaps)
	double NextOverlapSample = 0.;
	int32 ActorOverlapPairs = 0;
	int32 ActorOverlapDeep = 0;
	FLabFarStream FarStream; // M6 수정 A. FLabConfig::bFarHandoff일 때만 채운다(IsActive)
#if !UE_BUILD_TEST
	double FarLogNext = 0.; // 개발 로그 lab_farstream(5초마다)
#endif
};
