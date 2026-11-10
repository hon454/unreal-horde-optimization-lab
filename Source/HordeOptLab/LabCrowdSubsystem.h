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

	int32 GetAliveCount() const { return AliveCount; }

	/**
	 * 미니언 ISM을 Lumen 장면에서 뺀다. ISM 컴포넌트는 표현 프로세서가 첫 갱신 때 만들므로 오브가 매 틱 부르고, 찾으면 한 번만 적용한다.
	 * 이유: 매 프레임 움직이는 인스턴스 2,000개가 Lumen 장면에 들어가면 GPU가 29ms가 된다(2026-11-07 시험). 스켈레탈 메시 캐릭터는 원래 Lumen 장면 밖이다.
	 */
	void ExcludeVisualsFromLumen();

	// 오브 상태. 오브가 쓰고 프로세서가 읽는다.
	FVector OrbCenter = FVector::ZeroVector;
	std::atomic<uint32> PendingSnipeTick{0}; // 0이 아니면 이번 처리에서 저격한다. 오브 틱(게임 스레드)이 쓰고 저격 프로세서(워커일 수 있음)가 exchange(0)으로 가져간다(M3b 정비)
	FLabSnipeView SnipeView;     // 화면 판정용 카메라(오브가 매 틱 갱신)
	float HitDuration = 0.5f;
	float DeathDuration = 1.f;
	FLabVatClips VatClipsByType[LabUnitTypeMax]; // M2부터, 종류별(2종). Activate 전에 스포너가 채운다. 1종은 [0]만 쓴다

	// 프로세서가 쓰고 오브가 읽는다. 청크 단위로 병렬 실행될 수 있어 잠금으로 모은다.
	FCriticalSection ResultLock;
	TArray<FVector> SnipeTargets;    // 이번 저격의 대상 위치(이펙트용)
	bool bHasNearest = false;        // 피해 범위 안에서 가장 가까운 개체(테슬라 코일 번개 대상)
	float NearestDistSq = 0.f;
	FVector Nearest = FVector::ZeroVector;
	float NearestHoldUntil = 0.f;    // M4: 이 시각까지는 범위 피해가 돌지 않은 프레임에도 번개 대상을 지우지 않는다(LabMassProcessors.cpp)
	int32 KillCount = 0;
	int32 SnipeCount = 0;
	std::atomic<int64> DamageHits{0}; // E1 (d): 체력에 실제로 적용된 비치명 피해 건수(확인용)
	// E3 (3c) 스탯(2종): 대형 액터가 있었던 액터×프레임 합과 그중 애니 틱을 건너뛴 수. ULabActorSyncProcessor가 더하고 게임 모드가 측정 구간 차이를 CSV 메타데이터로 쓴다
	mutable std::atomic<int64> LargeActorFrames{0};
	mutable std::atomic<int64> LargeActorSkips{0};
#if !UE_BUILD_TEST
	/**
	 * 개발용 프레임 진단(-LabFrameLog, E3b (8) 화질 캡처 2026-11-09): 화면 안 30m에서 아직 VAT 인스턴스로 그리는 개체 수와 보이는 액터 수를 프레임마다 로그에 남긴다.
	 * VAT·ActorSync 프로세서가 GFrameCounter&1 슬롯에 더하고, 게임 모드 Tick이 다음 프레임에 이전 슬롯을 읽고 비운다(두 프로세서의 실행 순서와 무관).
	 */
	struct FFrameDiag { std::atomic<int32> NearVat{0}; std::atomic<int32> NearActors{0}; std::atomic<int32> ViewActors{0}; };
	mutable FFrameDiag FrameDiag[2];
#endif

private:
	UMassEntityConfigAsset* BuildEntityConfig(int32 UnitType, UStaticMesh* Mesh);
	UPROPERTY() TArray<TObjectPtr<UMassEntityConfigAsset>> EntityConfigs; // 종류별(LabUnitAssets.h). [0] 소형, [1] 대형(-LabTypes=2)
	TWeakObjectPtr<ALabCrowdSpawner> Spawner;
	bool bActive = false;
	bool bLumenExcluded = false;
	int32 AliveCount = 0;
};
