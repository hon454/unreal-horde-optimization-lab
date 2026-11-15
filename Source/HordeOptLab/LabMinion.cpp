#include "LabMinion.h"
#include "LabConfig.h"
#include "LabCrowdSpawner.h"
#include "LabSnipe.h" // LabSnipeTraceChannel(v2 W5)
#include "LabUnitAssets.h"
#include "HordeOptLab.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	// ALabMinionPawn은 CharacterMovement의 틱을 끈다.
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
	Health = FLabConfig::Get().HealthFor(InSerial);
	if (const float Scale = FLabUnitAssetPaths::Get(UnitType).Scale; Scale != 1.f)
	{
		SetActorScale3D(FVector(Scale)); // 대형: 캡슐과 메시가 함께 커진다. 스포너가 캡슐 중심 높이도 같은 배율로 올린다(LabCrowdSpawner.cpp SpawnAt)
	}
	// v2 W6 액터 기준선(Docs/v2-spec.md 3절 기능 2·3, Docs/Worklog/V2.md "W6"). 엔진이 ACharacter에 주는 보통의 수단으로 겹침을 막는다:
	// 캡슐끼리 Pawn 채널로 막고, CharacterMovement의 스윕 이동이 막히면 미끄러지고(MoveAlongFloor → SlideAlongSurface) 시작부터 겹쳐 있으면 밀어낸다
	// (SafeMoveUpdatedComponent → ResolvePenetration, engine-notes.md 39절). 캡슐 반지름은 겹친 쌍의 몸통 반지름(소형 40, 대형 94, FLabUnitAssetPaths::BoundRadius)과 같게 둔다.
	// 대형은 액터 배율(2)이 캡슐에도 곱해지므로 배율로 나눈다. 반높이(88)는 ACharacter 기본값 그대로다.
	// 저격 트레이스 채널(LabSnipe)을 막게 해 오브의 캡슐 라인 트레이스가 맞는다(하이드레이션 액터의 Trace 설정과 같은 채널, ALabOrb::TraceCapsule).
	{
		UCapsuleComponent* Capsule = GetCapsuleComponent();
		const FLabUnitAssetPaths& Paths = FLabUnitAssetPaths::Get(UnitType);
		Capsule->SetCapsuleRadius(Paths.BoundRadius / FMath::Max(Paths.Scale, 0.1f), /*bUpdateOverlaps*/ false);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, FLabConfig::Get().UsesActorBlocking() ? ECR_Block : ECR_Ignore); // -LabSeparate=0이면 W5까지의 M0(서로 무시)
		Capsule->SetCollisionResponseToChannel(LabSnipeTraceChannel, ECR_Block);
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
		// AnimBP 에셋을 찾지 못했을 때의 대체 경로(에셋이 빠진 체크아웃). 측정 실행은 AnimBP를 쓴다.
		bSingleNode = true;
		SkelMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
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
	if (NewState == ELabMinionState::Dead)
	{
		// v2 W6: 사망 중인 개체는 막지 않는다. Mass 분리 프로세서도 사망 개체를 밀지도, 밀리지도 않게 둔다(Docs/Worklog/V2.md "W4" 결정 6). 피격 중인 개체는 막는다(멈춘 채)
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh->GetSkeletalMeshAsset())
	{
		return; // 메시 에셋이 빠진 체크아웃
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
			// ALabMinionPawn: 평지라 바닥 검사 없이 위치만 옮기고 진행 방향을 본다(스윕 없음). 하이드레이션 액터는 틱하지 않아 여기 오지 않는다.
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

// 이동 틱 없는 액터(하이드레이션 액터의 부모) ------------------------------

namespace
{
	/** CharacterMovement 틱을 끄고 위치를 직접 옮기게 한다. 하이드레이션 액터가 이 설정을 물려받는다. */
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

// 하이드레이션 액터 ----------------------------------------------------------

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
	// 캡슐 콜리전(FLabConfig::ActorCollision): v2 W5부터 기본 Trace(저격 트레이스 채널만 막는 QueryOnly, Docs/Worklog/V2.md "W5").
	// 끔(-LabActorCollision=0)은 옛 m3nocol 처리(E3 (6), Docs/Worklog/Q5.md "1b단위"), 켬(=1)은 캡슐 기본값(Pawn 프로필) 그대로다.
	// 범위 피해와 저격 대상 선택은 캡슐을 쓰지 않는다. 저격의 캡슐 트레이스는 확인용이다(ALabOrb::TraceSnipeCapsules).
	const FLabConfig::EActorCollision Collision = FLabConfig::Get().ActorCollision;
	if (Collision != FLabConfig::EActorCollision::On)
	{
		UCapsuleComponent* Capsule = GetCapsuleComponent();
		Capsule->SetShouldUpdatePhysicsVolume(false);
		Capsule->SetGenerateOverlapEvents(false);
		if (Collision == FLabConfig::EActorCollision::Trace)
		{
			// 오브젝트 종류는 Pawn 그대로, 응답은 저격 채널만 막음. 캡슐끼리(Pawn), 바닥, 카메라와는 아무 일도 하지 않는다.
			// 응답을 먼저 넣고 콜리전을 바꾼다. QueryAndPhysics → QueryOnly에서 바디를 한 번 다시 만든다(BodyInstance.cpp:757-767)
			FCollisionResponseContainer Responses(ECR_Ignore);
			Responses.SetResponse(LabSnipeTraceChannel, ECR_Block);
			Capsule->SetCollisionResponseToChannels(Responses);
		}
		// Trace: 트레이스·스윕은 받고 물리 시뮬레이션 바디는 만들지 않는다. NoCollision: 피직스 상태를 지운다(이동 때 바디 변환을 보내지 않는다)
		Capsule->SetCollisionEnabled(Collision == FLabConfig::EActorCollision::Off ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
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
	SkelMesh->SetCastShadow(true);
}

void ALabMinionMassActor::PrepareForPooling_Implementation()
{
	GetMesh()->SetComponentTickEnabled(false);
	BoundEntity.Reset();
	// 엔진 풀은 액터를 숨기기만 하고 콜리전은 그대로 둔다(MassActorSpawnerSubsystem.cpp:119-145). 숨은 액터의 캡슐이 마지막 자리에 남아
	// 트레이스에 맞지 않게 액터 단위로 끈다. 꺼낼 때(PrepareForGame) 다시 켠다(v2 W5)
	SetActorEnableCollision(false);
	// 죽은 채 풀에 들어온 액터의 진홍색을 지운다. 숨긴 동안 바꾼 커스텀 프리미티브 데이터는 보이는 첫 프레임에 반영되지 않는 것을 연속 캡처로 봤다.
	if (FLabConfig::Get().HasFix(FLabConfig::FixTintReset))
	{
		GetMesh()->SetCustomPrimitiveDataFloat(0, 0.f);
		AppliedDeadAmount = 0.f;
	}
}

void ALabMinionMassActor::PrepareForGame_Implementation()
{
	// 엔진은 풀에서 꺼낸 액터를 스폰 요청의 변환(엔티티 변환, 스케일 1)으로 SetActorTransform한 바로 다음에 이 함수를 부른다
	// (MassActorSpawnerSubsystem.cpp:253-255). 재사용된 대형 액터가 소형 크기로 돌아가지 않게 배율을 다시 넣는다.
	// 풀은 클래스가 키라 대형 액터가 소형으로 재사용되는 일은 없다(:239).
	if (const float Scale = FLabUnitAssetPaths::Get(UnitType).Scale; Scale != 1.f)
	{
		SetActorScale3D(FVector(Scale));
	}
	GetMesh()->SetComponentTickEnabled(true);
	SetActorEnableCollision(true); // 풀에 넣을 때 끈 것을 되돌린다(PrepareForPooling). 컴포넌트 설정(Trace·끔)은 그대로다
	// 엔진은 풀에서 꺼낸 액터를 이 함수 바로 앞에서 보이게 한다(MassActorSpawnerSubsystem.cpp:252-255). 포즈를 맞출 때까지 다시 숨긴다.
	if (FLabConfig::Get().HasFix(FLabConfig::FixWait))
	{
		HideUntilPoseReady();
	}
}

// 화면 크기 애니 예산 하이드레이션 액터 ----------------------------------------

ALabMinionMassActorBudget::ALabMinionMassActorBudget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName))
{
}

void ALabMinionMassActorBudget::BeginPlay()
{
	Super::BeginPlay();
	PrepareForGame_Implementation(); // 중요도는 동기화 프로세서가 매 프레임 넘긴다
}

void ALabMinionMassActorBudget::PrepareForPooling_Implementation()
{
	Super::PrepareForPooling_Implementation();
	CastChecked<USkeletalMeshComponentBudgeted>(GetMesh())->SetComponentSignificance(0.f);
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
