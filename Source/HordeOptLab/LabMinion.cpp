#include "LabMinion.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabUnitAssets.h"
#include "HordeOptLab.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SignificanceManager.h"
#include "SkeletalMeshComponentBudgeted.h"

ALabMinion::ALabMinion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	// 개체끼리 막지 않는다. M0과 M1(a)는 회피 없이 비교한다(plan.md).
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Paragon 미니언 피직스 에셋의 바디가 PhysType_Simulated라 바디가 생기면 래그돌이 된다(engine-notes.md 9절).
	// 메시 콜리전을 꺼서 피직스 바디를 만들지 않는다. 충돌은 캡슐이 맡는다.
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkelMesh->SetGenerateOverlapEvents(false);
	SkelMesh->SetRelativeLocation(FVector(0.f, 0.f, -Capsule->GetUnscaledCapsuleHalfHeight()));
	SkelMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	// pawn 변형은 CharacterMovement를 만들지 않는다(ALabMinionPawn).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bRunPhysicsWithNoController = true;
		Move->MaxWalkSpeed = FLabScenario::MoveSpeed;
		Move->bOrientRotationToMovement = true;
	}
	bUseControllerRotationYaw = false;
}

void ALabMinion::InitMinion(ALabCrowdSpawner* InSpawner, const FLabMinionAssets& Assets, uint32 InSerial)
{
	Spawner = InSpawner;
	AssetsRef = &Assets;
	Serial = InSerial;
	UnitType = FLabUnitTypes::TypeOf(InSerial); // 2종이면 10의 배수가 대형
	Health = FLabScenario::MinionHealth;
	if (const float Scale = FLabUnitAssetPaths::Get(UnitType).Scale; Scale != 1.f)
	{
		SetActorScale3D(FVector(Scale)); // 대형: 캡슐과 메시가 함께 커진다. 스포너가 캡슐 중심 높이도 같은 배율로 올린다(LabCrowdSpawner.cpp SpawnAt)
	}
	SetupVisual(Assets);
	EnterState(ELabMinionState::Walk);
}

void ALabMinion::SetupVisual(const FLabMinionAssets& Assets)
{
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetSkeletalMesh(Assets.Mesh);
	if (Assets.ActorMaterial)
	{
		SkelMesh->SetMaterial(0, Assets.ActorMaterial); // 대형만: M2~M4와 같은 짙은 회색(LabMinion.h FLabMinionAssets)
	}
	if (Assets.AnimClass)
	{
		SkelMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		SkelMesh->SetAnimInstanceClass(Assets.AnimClass);
	}
	else
	{
		// AnimBP가 아직 없을 때의 임시 경로. 측정 전에 AnimBP로 바꾼다(STATUS.md).
		bSingleNode = true;
		SkelMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	if (FLabConfig::Get().Variant == ELabVariant::Uro)
	{
		SkelMesh->bEnableUpdateRateOptimizations = true;
	}
}

void ALabMinion::EnterState(ELabMinionState NewState)
{
	State = NewState;
	UAnimSequence* Seq = nullptr;
	switch (NewState)
	{
	case ELabMinionState::Walk: Seq = AssetsRef->Walk; StateTimeLeft = 0.f; break;
	case ELabMinionState::Hit: Seq = AssetsRef->Hit; StateTimeLeft = Seq ? Seq->GetPlayLength() : 0.5f; break;
	case ELabMinionState::Dead: Seq = AssetsRef->Death; StateTimeLeft = Seq ? Seq->GetPlayLength() : 1.f; break;
	}
	if (NewState != ELabMinionState::Walk && !bManualMove)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh->GetSkeletalMeshAsset())
	{
		return; // static 변형
	}
	if (bSingleNode)
	{
		SkelMesh->PlayAnimation(Seq, NewState == ELabMinionState::Walk);
	}
	else if (ULabMinionAnimInstance* Anim = Cast<ULabMinionAnimInstance>(SkelMesh->GetAnimInstance()))
	{
		Anim->State = NewState;
	}
}

bool ALabMinion::ApplyDamage(float Amount)
{
	if (!IsAlive())
	{
		return false;
	}
	Health -= Amount;
	if (Health > 0.f)
	{
		return false;
	}
	EnterState(ELabMinionState::Dead);
	return true;
}

void ALabMinion::Snipe()
{
	if (IsAlive())
	{
		EnterState(ELabMinionState::Hit);
	}
}

void ALabMinion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == ELabMinionState::Walk)
	{
		const FVector ToOrb = Spawner->GetOrbLocation() - GetActorLocation();
		const FVector Dir = FVector(ToOrb.X, ToOrb.Y, 0.f).GetSafeNormal();
		if (!bManualMove)
		{
			AddMovementInput(Dir);
		}
		else
		{
			// pawn 변형: 평지라 바닥 검사 없이 위치만 옮기고 진행 방향을 본다(스윕 없음).
			SetActorLocationAndRotation(GetActorLocation() + Dir * FLabScenario::MoveSpeed * DeltaSeconds, Dir.Rotation());
		}
		return;
	}

	StateTimeLeft -= DeltaSeconds;
	if (StateTimeLeft > 0.f)
	{
		return;
	}
	if (State == ELabMinionState::Hit)
	{
		EnterState(ELabMinionState::Dead);
	}
	else
	{
		Spawner->OnMinionFinished(this);
	}
}

// static 변형 ---------------------------------------------------------------

ALabMinionStatic::ALabMinionStatic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StaticVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticVisual"));
	StaticVisual->SetupAttachment(GetMesh());
	StaticVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticVisual->SetGenerateOverlapEvents(false);
	LabExcludeFromLumen(*StaticVisual);
}

void ALabMinionStatic::SetupVisual(const FLabMinionAssets& Assets)
{
	GetMesh()->SetSkeletalMesh(nullptr);
	GetMesh()->SetComponentTickEnabled(false);
	StaticVisual->SetStaticMesh(Assets.StaticMesh);
}

// pawn 변형 ----------------------------------------------------------------

namespace
{
	/** CharacterMovement 틱을 끄고 위치를 직접 옮기게 한다. pawn·staticpawn·M3 하이드레이션 액터가 같은 설정을 쓴다. */
	void LabDisableCharacterMovement(UCharacterMovementComponent* Move)
	{
		// ACharacter는 CharacterMovement를 필수 하위 오브젝트로 만들어 DoNotCreateDefaultSubobject가 무시된다
		// ("Ignored DoNotCreateDefaultSubobject for CharMoveComp as it's marked as required", 2026-11-07 로그). 그래서 컴포넌트는 두고 틱을 끈다.
		Move->PrimaryComponentTick.bCanEverTick = false;
		Move->PrimaryComponentTick.bStartWithTickEnabled = false;
	}
}

ALabMinionPawn::ALabMinionPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bManualMove = true;
	LabDisableCharacterMovement(GetCharacterMovement());
}

ALabMinionStaticPawn::ALabMinionStaticPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bManualMove = true;
	LabDisableCharacterMovement(GetCharacterMovement());
}

// budget 변형 ---------------------------------------------------------------

namespace
{
	const FName LabMinionSignificanceTag(TEXT("LabMinion"));

	/** 거리 단계(25/50/100/250m) → 1, 0.75, 0.5, 0.25, 그 밖 0. 값이 클수록 중요하다(engine-notes.md 3절). */
	float LabTierSignificance(USignificanceManager::FManagedObjectInfo* Info, const FTransform& Viewpoint)
	{
		const AActor* Actor = CastChecked<AActor>(Info->GetObject());
		const float Dist = FVector::Dist(Actor->GetActorLocation(), Viewpoint.GetLocation());
		for (int32 i = 0; i < 4; ++i)
		{
			if (Dist <= FLabScenario::BudgetTiers[i])
			{
				return 1.f - 0.25f * i;
			}
		}
		return 0.f;
	}
}

ALabMinionBudget::ALabMinionBudget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName))
{
}

void ALabMinionBudget::BeginPlay()
{
	Super::BeginPlay();
	if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
	{
		Sig->RegisterObject(this, LabMinionSignificanceTag, &LabTierSignificance,
			USignificanceManager::EPostSignificanceType::Sequential,
			[](USignificanceManager::FManagedObjectInfo* Info, float /*Old*/, float New, bool /*bFinal*/)
			{
				ACharacter* Character = CastChecked<ACharacter>(Info->GetObject());
				CastChecked<USkeletalMeshComponentBudgeted>(Character->GetMesh())->SetComponentSignificance(New);
			});
	}
}

void ALabMinionBudget::EndPlay(const EEndPlayReason::Type Reason)
{
	if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
	{
		Sig->UnregisterObject(this);
	}
	Super::EndPlay(Reason);
}

// M3 하이드레이션 액터 ------------------------------------------------------

ALabMinionMassActor::ALabMinionMassActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 로직은 엔티티에 있다. 엔진이 SetActorTickEnabled(true)를 불러도 bCanEverTick이 false면 틱 함수가 등록되지 않는다.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ALabMinionMassActor::BeginPlay()
{
	Super::BeginPlay();
	// M0 스포너와 같은 에셋(LabCrowdSpawner.cpp LoadAssets, 경로는 ini [LabUnitAssets]). 이미 메모리에 있으면 LoadObject는 찾기만 한다.
	// 종류(UnitType)는 클래스가 정한다(…Large). 대형 에셋이 없어 대체 중이면 소형 에셋이다(FLabUnitTypes::AssetType).
	const FLabUnitAssetPaths& Paths = FLabUnitAssetPaths::Get(FLabUnitTypes::AssetType(UnitType));
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, *Paths.SkeletalMesh));
	SkelMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	SkelMesh->SetAnimInstanceClass(LoadClass<UAnimInstance>(nullptr, *Paths.AnimClass));
	if (UMaterialInterface* Material = Paths.ActorMaterial.IsEmpty() ? nullptr : LoadObject<UMaterialInterface>(nullptr, *Paths.ActorMaterial, nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		SkelMesh->SetMaterial(0, Material);
	}
	if (const float Scale = FLabUnitAssetPaths::Get(UnitType).Scale; Scale != 1.f)
	{
		// 대형: VAT 인스턴스와 같은 배율. 엔진이 액터를 옮길 때 스케일을 지우지 않게 ULabActorManagement::TeleportActor가 액터 스케일을 유지한다
		SetActorScale3D(FVector(Scale));
	}
	SkelMesh->SetCustomPrimitiveDataFloat(0, 0.f);
	// E3 (6) 콜리전 줄이기(2026-11-09 사용자 결정 A). 대표(mass)는 캡슐 기본값(Pawn 프로필) 그대로다.
	const ELabVariant Variant = FLabConfig::Get().Variant;
	if (Variant == ELabVariant::M3ColQuery || Variant == ELabVariant::M3NoCol)
	{
		GetCapsuleComponent()->SetShouldUpdatePhysicsVolume(false);
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);
		// QueryOnly: 트레이스·스윕은 받고 물리 시뮬레이션 바디는 만들지 않는다. NoCollision: 피직스 씬에서 뺀다
		GetCapsuleComponent()->SetCollisionEnabled(Variant == ELabVariant::M3NoCol ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}
	if (FLabConfig::Get().Variant == ELabVariant::M3Uro)
	{
		SkelMesh->bEnableUpdateRateOptimizations = true; // E3 (2). 거리 단계는 FAnimUpdateRateParameters 기본값(M0 uro와 같다)
	}
}

void ALabMinionMassActor::HideUntilPoseReady()
{
	bPoseReady = false;
	PendingReadyFrames = 0;
	if (!FLabConfig::Get().HasFix(FLabConfig::FixPassHide))
	{
		SetActorHiddenInGame(true); // 대책 2단계의 숨김 방식(모션 블러 번짐이 생긴다). 기록용
		return;
	}
	SetActorHiddenInGame(false);
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRenderInMainPass(false);
	SkelMesh->SetRenderInDepthPass(false);
	SkelMesh->SetCastShadow(false);
}

void ALabMinionMassActor::ShowPoseReady()
{
	bPoseReady = true;
	if (!FLabConfig::Get().HasFix(FLabConfig::FixPassHide))
	{
		SetActorHiddenInGame(false);
		return;
	}
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRenderInMainPass(true);
	SkelMesh->SetRenderInDepthPass(true);
	SkelMesh->SetCastShadow(!IsA<ALabMinionMassActorLow>()); // 저품질 액터는 그림자를 끈다(E3 (5))
}

void ALabMinionMassActor::PrepareForPooling_Implementation()
{
	GetMesh()->SetComponentTickEnabled(false);
	BoundEntity.Reset();
	// 죽은 채 풀에 들어온 액터의 진홍색을 지운다. 숨긴 동안 바꾼 커스텀 프리미티브 데이터는 보이는 첫 프레임에 반영되지 않는 것을 연속 캡처로 봤다(2026-11-08).
	if (FLabConfig::Get().HasFix(FLabConfig::FixTintReset))
	{
		GetMesh()->SetCustomPrimitiveDataFloat(0, 0.f);
		AppliedDeadAmount = 0.f;
	}
}

void ALabMinionMassActor::PrepareForGame_Implementation()
{
	// 엔진은 풀에서 꺼낸 액터를 스폰 요청의 변환(엔티티 변환, 스케일 1)으로 SetActorTransform한 바로 다음에 이 함수를 부른다
	// (MassActorSpawnerSubsystem.cpp:253-255). 재사용된 대형 액터가 소형 크기로 돌아가지 않게 배율을 다시 넣는다(2026-11-10 사용자가 본 증상).
	// 풀은 클래스가 키라 대형 액터가 소형으로 재사용되는 일은 없다(:239).
	if (const float Scale = FLabUnitAssetPaths::Get(UnitType).Scale; Scale != 1.f)
	{
		SetActorScale3D(FVector(Scale));
	}
	GetMesh()->SetComponentTickEnabled(true);
	// 엔진은 풀에서 꺼낸 액터를 이 함수 바로 앞에서 보이게 한다(MassActorSpawnerSubsystem.cpp:252-255). 포즈를 맞출 때까지 다시 숨긴다.
	if (FLabConfig::Get().HasFix(FLabConfig::FixWait))
	{
		HideUntilPoseReady();
	}
}

// E3 (3) Budget 하이드레이션 액터 --------------------------------------------

ALabMinionMassActorBudget::ALabMinionMassActorBudget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName))
{
}

void ALabMinionMassActorBudget::BeginPlay()
{
	Super::BeginPlay();
	PrepareForGame_Implementation();
}

void ALabMinionMassActorBudget::EndPlay(const EEndPlayReason::Type Reason)
{
	if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
	{
		Sig->UnregisterObject(this);
	}
	Super::EndPlay(Reason);
}

void ALabMinionMassActorBudget::PrepareForPooling_Implementation()
{
	Super::PrepareForPooling_Implementation();
	if (USignificanceManager* Sig = USignificanceManager::Get(GetWorld()))
	{
		Sig->UnregisterObject(this);
	}
	CastChecked<USkeletalMeshComponentBudgeted>(GetMesh())->SetComponentSignificance(0.f);
}

void ALabMinionMassActorBudget::PrepareForGame_Implementation()
{
	Super::PrepareForGame_Implementation();
	if (FLabConfig::Get().Variant != ELabVariant::M3BudgetDist)
	{
		return; // (b) 중요도는 동기화 프로세서가 매 프레임 넘긴다
	}
	USignificanceManager* Sig = USignificanceManager::Get(GetWorld());
	if (Sig && !Sig->GetManagedObject(this))
	{
		// M0 budget 변형과 같은 거리 단계(ALabMinionBudget::BeginPlay)
		Sig->RegisterObject(this, LabMinionSignificanceTag, &LabTierSignificance,
			USignificanceManager::EPostSignificanceType::Sequential,
			[](USignificanceManager::FManagedObjectInfo* Info, float /*Old*/, float New, bool /*bFinal*/)
			{
				ACharacter* Character = CastChecked<ACharacter>(Info->GetObject());
				CastChecked<USkeletalMeshComponentBudgeted>(Character->GetMesh())->SetComponentSignificance(New);
			});
	}
}

// 2종 대형 액터(M3b) --------------------------------------------------------

ALabMinionMassActorLarge::ALabMinionMassActorLarge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnitType = LabUnitLarge;
}

ALabMinionMassActorBudgetLarge::ALabMinionMassActorBudgetLarge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnitType = LabUnitLarge;
}

// E3 (5) 저품질 액터 ---------------------------------------------------------

ALabMinionMassActorLow::ALabMinionMassActorLow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ALabMinionMassActorLow::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->bEnableUpdateRateOptimizations = true;
	SkelMesh->SetForcedLOD(SkelMesh->GetNumLODs()); // 1부터 센다(0은 자동). 가장 낮은 LOD
	SkelMesh->SetCastShadow(false);
	UE_LOG(LogLab, Verbose, TEXT("lab_lowres lods=%d"), SkelMesh->GetNumLODs());
}
