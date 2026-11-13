#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "LabSnipe.h"
#include "LabUnitAssets.h"
#include "LabCrowdSubsystem.generated.h"

class ALabCrowdSpawner;
class UMassEntityConfigAsset;
class UStaticMesh;
class USkeletalMesh;
class UTransformProviderData;
class UInstancedSkinnedMeshComponent;
struct FMassEntityManager;

/** VAT 텍스처 안의 클립 하나(프레임 범위, 끝 포함). Scripts/bake-vat.py가 굽는 순서: 걷기, 피격, 죽기. */
struct FLabVatClip
{
	int32 StartFrame = 0;
	int32 EndFrame = 0;
};

/** M2 VAT 재생 정보. 데이터 에셋(UAnimToTextureDataAsset::Animations, SampleRate)에서 읽는다. */
struct FLabVatClips
{
	FLabVatClip Walk;
	FLabVatClip Hit;
	FLabVatClip Death;
	float SampleRate = 30.f;
};

/**
 * M1부터 쓰는 Mass 군중의 공유 상태. 스포너(위치·일련번호의 주인)와 오브(이펙트), 프로세서(로직)가 이 객체로 이어진다.
 * EntityConfig는 에셋이 아니라 여기서 실행 시 만든다(ADR-0005). M0에서는 활성화하지 않는다.
 */
UCLASS()
class ULabCrowdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/** M1 단계에서 스포너가 부른다. 종류별(MeshesByType.Num() = -LabTypes) EntityConfig를 만들고 템플릿을 준비한다. 종류마다 아키타입·ISM 컴포넌트·액터 클래스(풀)가 갈린다. */
	void Activate(ALabCrowdSpawner* InSpawner, TConstArrayView<UStaticMesh*> MeshesByType, float InHitDuration, float InDeathDuration);
	bool IsActive() const { return bActive; }

	/** 땅 위(z=0) 위치와 일련번호로 엔티티를 만든다. 진행 방향은 오브를 본다. 2종이면 일련번호로 종류를 갈라 각 템플릿으로 만든다(순서 유지). */
	void SpawnEntities(TConstArrayView<FVector> Locations, TConstArrayView<uint32> Serials);
	/** 사망 프로세서가 지운 엔티티 수만큼 스포너에 재생성을 맡긴다(게임 스레드, 커맨드 플러시 중). */
	void RequestRespawn(int32 Count);
	/**
	 * M5 엔티티 풀링(-LabEntityPool=1): 사망이 끝난 엔티티를 종류별 풀에 넣고 같은 수만큼 재생성을 맡긴다(게임 스레드, 커맨드 플러시 중).
	 * 재생성 위치와 일련번호는 파괴 경로(RequestRespawn)와 같은 스포너 스트림에서 나온다. SpawnEntities가 일련번호의 종류에 맞는 풀 엔티티를 먼저 다시 쓰고,
	 * 모자라면 새로 만든다. 다시 쓰이지 않은 엔티티만 그리는 거리 밖에 세워 두고 FLabPooledTag를 붙인다.
	 */
	void RecycleAndRespawn(TConstArrayView<FMassEntityHandle> Finished);

	int32 GetAliveCount() const { return AliveCount; }

	/**
	 * 미니언 ISM을 Lumen 장면에서 뺀다. ISM 컴포넌트는 표현 프로세서가 첫 갱신 때 만들므로 오브가 매 틱 부르고, 이미 적용된 컴포넌트는 건너뛴다.
	 * 이유: 매 프레임 움직이는 인스턴스 2,000개가 Lumen 장면에 들어가면 GPU가 29ms가 된다(시험). 스켈레탈 메시 캐릭터는 원래 Lumen 장면 밖이다.
	 */
	void ExcludeVisualsFromLumen();

	// 오브 상태. 오브가 Mass PrePhysics 단계 시작(게임 스레드, 프로세서 전)에 쓰고 프로세서가 읽는다(ALabOrb::OnMassPrePhysicsStart).
	FVector OrbCenter = FVector::ZeroVector;
	std::atomic<uint32> PendingSnipeTick{0}; // 0이 아니면 이번 처리에서 저격한다. 오브가 단계 시작에 쓰고 저격 프로세서(워커일 수 있음)가 exchange(0)으로 가져간다
	FLabSnipeView SnipeView;     // 화면 판정용 카메라(직전 프레임에 그린 카메라, 오브가 단계 시작마다 갱신)
	float HitDuration = 0.5f;
	float DeathDuration = 1.f;
	FLabVatClips VatClipsByType[LabUnitTypeMax]; // M2부터, 종류별(2종). Activate 전에 스포너가 채운다. 1종은 [0]만 쓴다
	/**
	 * 걷기 루프 길이(초), 종류별. 액터 전환 때 시작 시각을 감는 데 쓴다(LabActorSyncProcessor.cpp). 스포너가 채운다.
	 * VAT는 클립 프레임 수 ÷ 샘플 레이트(M2~M4와 같은 값), M5 ISKM은 ASTPD의 GetSequencePlayLength(0)이다(plan.md "M5" 결정 4).
	 */
	float WalkLengthByType[LabUnitTypeMax] = { 1.f, 1.f };
	/**
	 * 사망 클립 재생 배율, 종류별(Q1b). 사망 유지 시간(DeathDuration)은 소형 사망 클립 길이 하나라서 더 짧은 시즈 클립은 늘려 재생해야 그 시간에 맞춰 끝난다.
	 * 배율 = (그 종류의 사망 클립 길이) ÷ (소형 사망 클립 길이). 표현 경로마다 클립 길이의 출처가 달라(VAT 프레임 수, ISKM 시퀀스, 액터 애니 에셋) 배열도 셋이다. 스포너가 채운다. 소형은 1.
	 */
	float DeathRateVat[LabUnitTypeMax] = { 1.f, 1.f };
	float DeathRateIskm[LabUnitTypeMax] = { 1.f, 1.f };
	float DeathRateActor[LabUnitTypeMax] = { 1.f, 1.f };
	/** M5 ISKM 에셋(종류별). Activate 전에 스포너가 채운다. 메시는 Scripts/create-iskm.py의 복제본(SK_*ISKM), 제공자는 ASTPD(ASTP_*). 클립 0 걷기, 1 피격, 2 죽기 */
	UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> IskmMeshes;
	UPROPERTY() TArray<TObjectPtr<UTransformProviderData>> IskmProviders;

	// 프로세서가 쓰고 오브가 읽는다. 청크 단위로 병렬 실행될 수 있어 잠금으로 모은다.
	FCriticalSection ResultLock;
	TArray<FVector> SnipeTargets;    // 이번 저격의 대상 위치(이펙트용)
	bool bHasNearest = false;        // 피해 범위 안에서 가장 가까운 개체(테슬라 코일 번개 대상)
	float NearestDistSq = 0.f;
	FVector Nearest = FVector::ZeroVector;
	float NearestHoldUntil = 0.f;    // M4: 이 시각까지는 범위 피해가 돌지 않은 프레임에도 번개 대상을 지우지 않는다(LabMassProcessors.cpp)
	// 누적 사망 수. 프로세서(워커일 수 있음)가 ResultLock 안에서 더하고 게임 스레드(게임 모드·HUD)가 잠금 없이 읽으므로 원자 변수다
	std::atomic<int32> KillCount{0};
	std::atomic<int32> SnipeCount{0};
	std::atomic<int64> DamageHits{0}; // E1 (d): 체력에 실제로 적용된 비치명 피해 건수(확인용)
	/**
	 * 애니 갱신 통계(종류별): 액터×프레임 합, 그중 애니 갱신을 하지 않은 수(보간 + 건너뜀), 그중 보간한 수. 건너뜀 = Skips − Interps.
	 * ULabActorSyncProcessor가 더하고 게임 모드가 측정 구간 차이를 CSV 메타데이터 LabLargeActor*·LabSmallActor*로 쓴다.
	 */
	mutable std::atomic<int64> LargeActorFrames{0};
	mutable std::atomic<int64> LargeActorSkips{0};
	mutable std::atomic<int64> LargeActorInterps{0};
	mutable std::atomic<int64> SmallActorFrames{0};
	mutable std::atomic<int64> SmallActorSkips{0};
	mutable std::atomic<int64> SmallActorInterps{0};
	/** Q5 동등성 통계: 오브 수평 30m 안 엔티티 수(ULabNearOrbCountProcessor가 매 프레임 쓴다, FLabEquivalence). 게임 모드가 CSV 스탯 Lab/Near30으로 남긴다 */
	mutable std::atomic<int32> Near30{0};
	/** Q5 HUD: 이 프레임 동기화한 하이드레이션 액터 수(CSV 스탯 Lab/Actors와 같은 값, ULabActorSyncProcessor가 쓴다) */
	mutable std::atomic<int32> ActorCount{0};
	/**
	 * 독자용 HUD(Docs/Worklog/Q5-hud.md): 이 프레임 인스턴스로 그린 개체 수(VAT·ISKM 프로세서의 밴드 집계 Lab/IsmBand0~3의 합).
	 * HUD가 보일 때만 ALabHud가 bHudCounts를 켜고, 꺼져 있으면 프로세서는 이 값을 쓰지 않는다(측정 실행의 동작 그대로).
	 */
	std::atomic<bool> bHudCounts{false};
	mutable std::atomic<int32> HudInstances{0};
	/**
	 * Q2b 엔티티 구동 입자(-LabCrowdFx=1, -LabCrowdFxEntity=1). 게임 모드가 배경 무리 컴포넌트를 켠 뒤 넣고, ULabCrowdFxGatherProcessor(게임 스레드)가 매 프레임 배열과 시각을 넣는다.
	 * bCrowdFx*는 에셋에 그 사용자 변수가 있는지(게임 모드가 활성화 전에 한 번 확인한다). 없는 값은 넣지 않는다(없는 이름은 Niagara가 부를 때마다 경고를 남긴다).
	 */
	TWeakObjectPtr<class UNiagaraComponent> CrowdFx;
	bool bCrowdFxSmallRecs = false;
	bool bCrowdFxLargeRecs = false;
	bool bCrowdFxNow = false;
	/** 지난 프레임에 넣은 엔티티 구동 원소 수(소형 + 대형). 게임 모드의 개발 로그 lab_crowdfx_count가 읽는다 */
	int32 CrowdRecsLast = 0;
	// M5 엔티티 풀링 집계(게임 스레드). 재생성 중 풀에서 다시 쓴 수와 새로 만든 수, 풀에 남은 수는 EntityPool의 Num
	int32 PoolReused = 0;
	int32 PoolCreated = 0;
	int32 GetPooledCount() const { return EntityPool[0].Num() + EntityPool[1].Num(); }
	/**
	 * 고정 바운드가 실제로 들어간 ISKM 메시 종류 수(인스턴스가 들어온 뒤 바운드가 고정값인 것으로 확인한 컴포넌트의 스켈레탈 메시 종류). CSV 메타데이터 LabIskmBounds.
	 * Q4b 원거리 그림자 끔이면 종류마다 컴포넌트가 둘이라(그림자 켬·끔) 컴포넌트가 아니라 메시 종류를 센다. 2종이면 2이고 P2(Medium이 비어 있다)에서도 같다.
	 */
	int32 GetIskmBoundsApplied() const { return IskmBoundsAssets.Num(); }
#if !UE_BUILD_TEST
	/**
	 * 개발용 프레임 진단(-LabFrameLog, E3b (8) 화질 캡처): 화면 안 30m에서 아직 VAT 인스턴스로 그리는 개체 수와 보이는 액터 수를 프레임마다 로그에 남긴다.
	 * VAT·ActorSync 프로세서가 GFrameCounter&1 슬롯에 더하고, 게임 모드 Tick이 다음 프레임에 이전 슬롯을 읽고 비운다(두 프로세서의 실행 순서와 무관).
	 */
	struct FFrameDiag { std::atomic<int32> NearVat{0}; std::atomic<int32> NearActors{0}; std::atomic<int32> ViewActors{0}; };
	mutable FFrameDiag FrameDiag[2];
#endif

private:
	UMassEntityConfigAsset* BuildEntityConfig(int32 UnitType, UStaticMesh* Mesh);
	/** m5bounds: ISKM 컴포넌트에 고정 바운드를 넣고, 인스턴스가 들어온 뒤 실제로 적용됐는지 한 번 로그에 남긴다 */
	void ApplyFixedIskmBounds(UInstancedSkinnedMeshComponent& Component);
	TArray<const UInstancedSkinnedMeshComponent*, TInlineAllocator<LabUnitTypeMax * 2>> IskmBoundsConfirmed; // 비교에만 쓴다(역참조하지 않는다)
	TArray<const UObject*, TInlineAllocator<LabUnitTypeMax>> IskmBoundsAssets; // 확인한 컴포넌트의 스켈레탈 메시(비교에만 쓴다)
	/** 풀 엔티티를 새 개체로 되살린다(위치·일련번호·체력·상태·이동). 태그는 부른 쪽이 한꺼번에 바꾼다 */
	void ReinitPooledEntity(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FVector& Location, uint32 Serial) const;
	TArray<FMassEntityHandle> EntityPool[LabUnitTypeMax]; // M5 엔티티 풀링: 종류별 대기 엔티티(뒤에서 꺼낸다)
#if !UE_BUILD_TEST
	uint32 SpawnCheckHash = 0; // 개발 확인: 재생성 일련번호·위치의 누적 해시(풀링 켬/끔이 같은 열을 만드는지, lab_pool_check)
#endif
	UPROPERTY() TArray<TObjectPtr<UMassEntityConfigAsset>> EntityConfigs; // 종류별(LabUnitAssets.h). [0] 소형, [1] 대형(-LabTypes=2)
	TWeakObjectPtr<ALabCrowdSpawner> Spawner;
	bool bActive = false;
	int32 AliveCount = 0;
};
