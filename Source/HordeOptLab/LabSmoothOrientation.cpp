#include "LabSmoothOrientation.h"
#include "LabConfig.h"
#include "LabMassProcessors.h" // ULabSeparationProcessor(개발 검사의 순서)
#include "HordeOptLab.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassNavigationUtils.h"
#include "MassSimulationLOD.h"
#include "SmoothOrientation/MassSmoothOrientationFragments.h"
#include "SmoothOrientation/MassSmoothOrientationProcessors.h"
#include "Engine/World.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"
#include <atomic>

#include UE_INLINE_GENERATED_CPP_BY_NAME(LabSmoothOrientation)

namespace
{
	/**
	 * 개체 하나의 HighRes 몫. 출처: UE 5.8.4 Engine/Plugins/AI/MassAI/Source/MassNavigation/Private/MassSmoothOrientationProcessors.cpp:62-112
	 * (UMassSmoothOrientationProcessor::Execute, Copyright Epic Games, Inc.). 식, 형(float과 FVector::FReal), 연산 순서를 바꾸지 않았다.
	 * 행동이 Animate면 아무것도 쓰지 않고 거짓을 낸다(엔진의 continue).
	 */
	FORCEINLINE bool LabSmoothHighRes(const FMassMoveTargetFragment& MoveTarget, FMassDesiredMovementFragment& DesiredMovement, FTransform& CurrentTransform,
		const FMassSmoothOrientationParameters& OrientationParams, const float DeltaTime)
	{
		// Do not touch transform at all when animating
		if (MoveTarget.GetCurrentAction() == EMassMovementAction::Animate)
		{
			return false;
		}

		const FVector CurrentForward = CurrentTransform.GetRotation().GetForwardVector();
		const FVector::FReal CurrentHeading = UE::MassNavigation::GetYawFromDirection(CurrentForward);

		const float EndOfPathAnticipationDistance = OrientationParams.EndOfPathDuration * MoveTarget.DesiredSpeed.Get();

		FVector::FReal MoveTargetWeight = 0.5;
		FVector::FReal VelocityWeight = 0.5;

		if (MoveTarget.GetCurrentAction() == EMassMovementAction::Move)
		{
			if (MoveTarget.IntentAtGoal == EMassMovementAction::Stand && MoveTarget.DistanceToGoal < EndOfPathAnticipationDistance)
			{
				// Fade towards the movement target direction at the end of the path.
				const float Fade = FMath::Square(FMath::Clamp(MoveTarget.DistanceToGoal / EndOfPathAnticipationDistance, 0.0f, 1.0f)); // zero at end of the path

				MoveTargetWeight = FMath::Lerp(OrientationParams.Standing.MoveTargetWeight, OrientationParams.Moving.MoveTargetWeight, Fade);
				VelocityWeight = FMath::Lerp(OrientationParams.Standing.VelocityWeight, OrientationParams.Moving.VelocityWeight, Fade);
			}
			else
			{
				MoveTargetWeight = OrientationParams.Moving.MoveTargetWeight;
				VelocityWeight = OrientationParams.Moving.VelocityWeight;
			}
		}
		else // Stand
		{
			MoveTargetWeight = OrientationParams.Standing.MoveTargetWeight;
			VelocityWeight = OrientationParams.Standing.VelocityWeight;
		}

		const FVector::FReal VelocityHeading = UE::MassNavigation::GetYawFromDirection(DesiredMovement.DesiredVelocity);
		const FVector::FReal MovementHeading = UE::MassNavigation::GetYawFromDirection(MoveTarget.Forward);

		const FVector::FReal Ratio = MoveTargetWeight / (MoveTargetWeight + VelocityWeight);
		const FVector::FReal DesiredHeading = UE::MassNavigation::LerpAngle(VelocityHeading, MovementHeading, Ratio);
		DesiredMovement.DesiredFacing = FQuat(FVector::UpVector, DesiredHeading);

		const FVector::FReal NewHeading = UE::MassNavigation::ExponentialSmoothingAngle(CurrentHeading, DesiredHeading, DeltaTime, OrientationParams.OrientationSmoothingTime);

		FQuat Rotation(FVector::UpVector, NewHeading);
		CurrentTransform.SetRotation(Rotation);
		return true;
	}

	/** 개체 하나의 LowRes(Off) 몫. 출처: 같은 파일 127-131. 이동 목표 방향으로 바로 맞춘다 */
	FORCEINLINE void LabSmoothLowRes(const FMassMoveTargetFragment& MoveTarget, FTransform& CurrentTransform)
	{
		// Snap position to move target directly
		CurrentTransform.SetRotation(FQuat::FindBetweenNormals(FVector::ForwardVector, MoveTarget.Forward));
	}

#if !UE_BUILD_TEST
	/** 개발 검사의 예측 하나. 입력(바라는 속도, 이동 목표)도 적어 두고, 엔진 프로세서와 검사 사이에 바뀐 개체는 견주지 않는다 */
	struct FLabOrientPred
	{
		FMassEntityHandle Entity;
		FQuat Rotation;
		FQuat Facing;
		FVector DesiredVelocity;
		FVector MoveForward;
		float DesiredSpeed = 0.f;
		float DistanceToGoal = 0.f;
		uint8 Action = 0;
		uint8 Intent = 0;
		bool bLowRes = false;
	};
	FCriticalSection GLabPredLock;
	TArray<FLabOrientPred> GLabPreds;
	uint64 GLabPredFrame = 0;

	bool LabSameBits(const FQuat& A, const FQuat& B) { return FMemory::Memcmp(&A, &B, sizeof(FQuat)) == 0; }
	bool LabSameBits(const FVector& A, const FVector& B) { return FMemory::Memcmp(&A, &B, sizeof(FVector)) == 0; }
	bool LabSameBits(float A, float B) { return FMemory::Memcmp(&A, &B, sizeof(float)) == 0; }
#endif
}

ULabSmoothOrientationProcessor::ULabSmoothOrientationProcessor()
	: HighResEntityQuery(*this)
	, LowResEntityQuery_Conditional(*this)
{
	bAutoRegisterWithProcessingPhases = false; // FLabConfig::bSmoothOrient(또는 개발 검사)일 때만 게임 모듈이 켠다(HordeOptLab.cpp)
	// 엔진 UMassSmoothOrientationProcessor 생성자와 같다(MassSmoothOrientationProcessors.cpp:24-25)
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
#if !UE_BUILD_TEST
	bPredict = FLabConfig::Get().bSmoothOrientCheck;
	if (bPredict)
	{
		// 검사: 엔진 프로세서와 같은 입력을 보도록 그 앞에 돈다. 엔진 프로세서는 앞 선언이 없어서 가변 틱 값(LOD 그룹), 바라는 속도(ApplyForces 그룹,
		// MassMovementProcessors.cpp:25-26,56), 이동 목표(Tasks 그룹)가 쓰이기 전에도 뒤에도 돌 수 있다. 검사 모드에서만 예측을 둘 뒤에 두어 예측과 엔진이 같은 값을 보게 한다
		// (첫 검사 실행에서 LowRes가 가변 틱 차이로, HighRes가 입력 변화로 대부분 갈렸다. Docs/Worklog/V2.md "방향 보간 병렬 사본")
		ExecutionOrder.ExecuteBefore.Add(UMassSmoothOrientationProcessor::StaticClass()->GetFName());
		ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
		ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::ApplyForces);
		ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Tasks); // 이동 목표(ULabApproachProcessor, Tasks 그룹)
	}
#endif
}

void ULabSmoothOrientationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// 엔진 쿼리와 같다(MassSmoothOrientationProcessors.cpp:30-40). 검사의 예측은 프래그먼트를 쓰지 않으므로 읽기 전용이다
	const EMassFragmentAccess Write = bPredict ? EMassFragmentAccess::ReadOnly : EMassFragmentAccess::ReadWrite;
	HighResEntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	HighResEntityQuery.AddRequirement<FMassDesiredMovementFragment>(Write);
	HighResEntityQuery.AddRequirement<FTransformFragment>(Write);
	HighResEntityQuery.AddTagRequirement<FMassOffLODTag>(EMassFragmentPresence::None);
	HighResEntityQuery.AddConstSharedRequirement<FMassSmoothOrientationParameters>(EMassFragmentPresence::All);

	LowResEntityQuery_Conditional.AddRequirement<FTransformFragment>(Write);
	LowResEntityQuery_Conditional.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	LowResEntityQuery_Conditional.AddTagRequirement<FMassOffLODTag>(EMassFragmentPresence::All);
	LowResEntityQuery_Conditional.AddChunkRequirement<FMassSimulationVariableTickChunkFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	LowResEntityQuery_Conditional.SetChunkFilter(&FMassSimulationVariableTickChunkFragment::ShouldTickChunkThisFrame);
}

void ULabSmoothOrientationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Clamp max delta time to avoid force explosion on large time steps (i.e. during initialization). (엔진과 같다, MassSmoothOrientationProcessors.cpp:46-47)
	const float DeltaTime = FMath::Min(0.1f, Context.GetDeltaTimeSeconds());

#if !UE_BUILD_TEST
	const uint64 StartCycles = FPlatformTime::Cycles64();
	std::atomic<uint64> FirstChunkCycles{ 0 };
	std::atomic<int64> HiChunks{ 0 }, HiEnts{ 0 }, LoEnts{ 0 };
	auto NoteChunk = [&FirstChunkCycles, StartCycles](std::atomic<int64>& Ents, int32 Num, std::atomic<int64>* Chunks)
	{
		uint64 Expected = 0;
		FirstChunkCycles.compare_exchange_strong(Expected, FPlatformTime::Cycles64());
		Ents += Num;
		if (Chunks) { ++*Chunks; }
	};
	const bool bSerial = FLabConfig::Get().bSmoothOrientSerial;

	if (bPredict)
	{
		// 검사의 예측: 프래그먼트의 사본에 같은 식을 돌려 결과를 모은다. 엔진 프로세서가 실제 값을 쓴다
		{
			FScopeLock Lock(&GLabPredLock);
			GLabPreds.Reset();
			GLabPredFrame = GFrameCounter;
		}
		HighResEntityQuery.ParallelForEachEntityChunk(Context, [&](FMassExecutionContext& Ctx)
		{
			const FMassSmoothOrientationParameters& OrientationParams = Ctx.GetConstSharedFragment<FMassSmoothOrientationParameters>();
			const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
			const TConstArrayView<FTransformFragment> LocationList = Ctx.GetFragmentView<FTransformFragment>();
			const TConstArrayView<FMassDesiredMovementFragment> DesiredMovementList = Ctx.GetFragmentView<FMassDesiredMovementFragment>();
			TArray<FLabOrientPred> Local;
			Local.Reserve(Ctx.GetNumEntities());
			for (FMassExecutionContext::FEntityIterator It = Ctx.CreateEntityIterator(); It; ++It)
			{
				const FMassMoveTargetFragment& MoveTarget = MoveTargetList[It];
				FMassDesiredMovementFragment DesiredMovement = DesiredMovementList[It];
				FTransform Transform = LocationList[It].GetTransform();
				LabSmoothHighRes(MoveTarget, DesiredMovement, Transform, OrientationParams, DeltaTime); // Animate면 사본이 그대로다(엔진도 쓰지 않는다)
				FLabOrientPred& P = Local.AddDefaulted_GetRef();
				P.Entity = Ctx.GetEntity(It);
				P.Rotation = Transform.GetRotation();
				P.Facing = DesiredMovement.DesiredFacing;
				P.DesiredVelocity = DesiredMovement.DesiredVelocity;
				P.MoveForward = MoveTarget.Forward;
				P.DesiredSpeed = MoveTarget.DesiredSpeed.Get();
				P.DistanceToGoal = MoveTarget.DistanceToGoal;
				P.Action = uint8(MoveTarget.GetCurrentAction());
				P.Intent = uint8(MoveTarget.IntentAtGoal);
			}
			FScopeLock Lock(&GLabPredLock);
			GLabPreds.Append(Local);
		});
		LowResEntityQuery_Conditional.ParallelForEachEntityChunk(Context, [&](FMassExecutionContext& Ctx)
		{
			const TConstArrayView<FTransformFragment> LocationList = Ctx.GetFragmentView<FTransformFragment>();
			const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
			TArray<FLabOrientPred> Local;
			Local.Reserve(Ctx.GetNumEntities());
			for (FMassExecutionContext::FEntityIterator It = Ctx.CreateEntityIterator(); It; ++It)
			{
				const FMassMoveTargetFragment& MoveTarget = MoveTargetList[It];
				FTransform Transform = LocationList[It].GetTransform();
				LabSmoothLowRes(MoveTarget, Transform);
				FLabOrientPred& P = Local.AddDefaulted_GetRef();
				P.Entity = Ctx.GetEntity(It);
				P.Rotation = Transform.GetRotation();
				P.MoveForward = MoveTarget.Forward;
				P.bLowRes = true;
			}
			FScopeLock Lock(&GLabPredLock);
			GLabPreds.Append(Local);
		});
		return;
	}
#endif

	// 엔진 MassSmoothOrientationProcessors.cpp:52-114와 같은 순회. 청크 병렬이다(개발 진단 -LabSmoothOrientSerial이면 한 스레드)
	const FMassExecuteFunction HighRes = [&](FMassExecutionContext& Ctx)
	{
		const FMassSmoothOrientationParameters& OrientationParams = Ctx.GetConstSharedFragment<FMassSmoothOrientationParameters>();

		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
		const TArrayView<FTransformFragment> LocationList = Ctx.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassDesiredMovementFragment> DesiredMovementList = Ctx.GetMutableFragmentView<FMassDesiredMovementFragment>();
#if !UE_BUILD_TEST
		NoteChunk(HiEnts, Ctx.GetNumEntities(), &HiChunks);
#endif
		for (FMassExecutionContext::FEntityIterator EntityIt = Ctx.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			LabSmoothHighRes(MoveTargetList[EntityIt], DesiredMovementList[EntityIt], LocationList[EntityIt].GetMutableTransform(), OrientationParams, DeltaTime);
		}
	};
	// 같은 파일 120-133
	const FMassExecuteFunction LowRes = [&](FMassExecutionContext& Ctx)
	{
		const TArrayView<FTransformFragment> LocationList = Ctx.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
#if !UE_BUILD_TEST
		NoteChunk(LoEnts, Ctx.GetNumEntities(), nullptr);
#endif
		for (FMassExecutionContext::FEntityIterator EntityIt = Ctx.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			LabSmoothLowRes(MoveTargetList[EntityIt], LocationList[EntityIt].GetMutableTransform());
		}
	};

#if !UE_BUILD_TEST
	if (bSerial)
	{
		HighResEntityQuery.ForEachEntityChunk(Context, HighRes);
		LowResEntityQuery_Conditional.ForEachEntityChunk(Context, LowRes);
	}
	else
#endif
	{
		HighResEntityQuery.ParallelForEachEntityChunk(Context, HighRes);
		LowResEntityQuery_Conditional.ParallelForEachEntityChunk(Context, LowRes);
	}

#if !UE_BUILD_TEST
	// 개발 진단: 5초마다 프레임 평균 실행 시간, 첫 청크까지(쿼리 자원 기다림), 청크·개체 수
	const uint64 EndCycles = FPlatformTime::Cycles64();
	const uint64 First = FirstChunkCycles.load();
	LogExecCycles += EndCycles - StartCycles;
	LogWaitCycles += First > 0 ? First - StartCycles : EndCycles - StartCycles;
	LogExecMaxMs = FMath::Max(LogExecMaxMs, FPlatformTime::ToMilliseconds64(EndCycles - StartCycles));
	LogHiChunks += HiChunks.load();
	LogHiEnts += HiEnts.load();
	LogLoEnts += LoEnts.load();
	++LogFrames;
	const UWorld* World = Context.GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.;
	if (LogSince <= 0.)
	{
		LogSince = Now;
	}
	else if (Now - LogSince >= 5.)
	{
		UE_LOG(LogLab, Log, TEXT("lab_smoothori t=%.1f mode=%s frames=%d exec_ms=%.3f exec_max_ms=%.3f wait_ms=%.3f hi_chunks=%.1f hi_ents=%.0f lo_ents=%.0f"),
			Now, bSerial ? TEXT("serial") : TEXT("parallel"), LogFrames, FPlatformTime::ToMilliseconds64(LogExecCycles) / LogFrames, LogExecMaxMs,
			FPlatformTime::ToMilliseconds64(LogWaitCycles) / LogFrames, double(LogHiChunks) / LogFrames, double(LogHiEnts) / LogFrames, double(LogLoEnts) / LogFrames);
		LogSince = Now;
		LogFrames = 0;
		LogExecCycles = LogWaitCycles = 0;
		LogExecMaxMs = 0.;
		LogHiChunks = LogHiEnts = LogLoEnts = 0;
	}
#endif
}

// 개발 검사 ----------------------------------------------------------------------

ULabSmoothOrientationCheckProcessor::ULabSmoothOrientationCheckProcessor()
	: HighResEntityQuery(*this)
	, LowResEntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false; // 개발 검사(-LabSmoothOrientCheck)일 때만 게임 모듈이 켠다
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(UMassSmoothOrientationProcessor::StaticClass()->GetFName());
	// 겹침 방지가 바라는 속도를 고치기 전에 견준다(LabMassProcessors.cpp ULabSeparationProcessor::Execute)
	ExecutionOrder.ExecuteBefore.Add(ULabSeparationProcessor::StaticClass()->GetFName());
}

void ULabSmoothOrientationCheckProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	HighResEntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	HighResEntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadOnly);
	HighResEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	HighResEntityQuery.AddTagRequirement<FMassOffLODTag>(EMassFragmentPresence::None);
	HighResEntityQuery.AddConstSharedRequirement<FMassSmoothOrientationParameters>(EMassFragmentPresence::All);

	LowResEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	LowResEntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	LowResEntityQuery.AddTagRequirement<FMassOffLODTag>(EMassFragmentPresence::All);
}

void ULabSmoothOrientationCheckProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
#if !UE_BUILD_TEST
	TMap<FMassEntityHandle, int32> Index;
	{
		FScopeLock Lock(&GLabPredLock);
		if (GLabPredFrame != GFrameCounter)
		{
			return; // 이 프레임에 예측이 돌지 않았다
		}
		Index.Reserve(GLabPreds.Num());
		for (int32 I = 0; I < GLabPreds.Num(); ++I)
		{
			Index.Add(GLabPreds[I].Entity, I);
		}
	}
	int64 HiCompared = 0, HiMismatch = 0, LoCompared = 0, LoMismatch = 0, Changed = 0, Missing = 0, ChangedVel = 0, ChangedTarget = 0;
	const UWorld* World = Context.GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.;
	HighResEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
		const TConstArrayView<FTransformFragment> LocationList = Ctx.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassDesiredMovementFragment> DesiredMovementList = Ctx.GetFragmentView<FMassDesiredMovementFragment>();
		for (FMassExecutionContext::FEntityIterator It = Ctx.CreateEntityIterator(); It; ++It)
		{
			const int32* Found = Index.Find(Ctx.GetEntity(It));
			if (!Found || GLabPreds[*Found].bLowRes)
			{
				++Missing;
				continue;
			}
			const FLabOrientPred& P = GLabPreds[*Found];
			const FMassMoveTargetFragment& MoveTarget = MoveTargetList[It];
			const FMassDesiredMovementFragment& DesiredMovement = DesiredMovementList[It];
			const bool bVelChanged = !LabSameBits(P.DesiredVelocity, DesiredMovement.DesiredVelocity);
			const bool bTargetChanged = !LabSameBits(P.MoveForward, MoveTarget.Forward)
				|| !LabSameBits(P.DesiredSpeed, MoveTarget.DesiredSpeed.Get()) || !LabSameBits(P.DistanceToGoal, MoveTarget.DistanceToGoal)
				|| P.Action != uint8(MoveTarget.GetCurrentAction()) || P.Intent != uint8(MoveTarget.IntentAtGoal);
			if (bVelChanged || bTargetChanged)
			{
				++Changed;
				ChangedVel += bVelChanged ? 1 : 0;
				ChangedTarget += bTargetChanged ? 1 : 0;
				if (bTargetChanged && ReportedChanged < 5 && Now > 30.)
				{
					++ReportedChanged;
					UE_LOG(LogLab, Log, TEXT("lab_smoothori_changed t=%.2f entity=%d fwd=%s/%s speed=%g/%g dist=%g/%g action=%d/%d intent=%d/%d"), Now, Ctx.GetEntity(It).Index,
						*P.MoveForward.ToString(), *MoveTarget.Forward.ToString(), P.DesiredSpeed, MoveTarget.DesiredSpeed.Get(), P.DistanceToGoal, MoveTarget.DistanceToGoal,
						int32(P.Action), int32(MoveTarget.GetCurrentAction()), int32(P.Intent), int32(MoveTarget.IntentAtGoal));
				}
				continue;
			}
			++HiCompared;
			const FQuat Rotation = LocationList[It].GetTransform().GetRotation();
			if (!LabSameBits(P.Rotation, Rotation) || !LabSameBits(P.Facing, DesiredMovement.DesiredFacing))
			{
				++HiMismatch;
				if (Reported < 10)
				{
					++Reported;
					UE_LOG(LogLab, Warning, TEXT("lab_smoothori_mismatch t=%.2f hi entity=%d pred_rot=%s eng_rot=%s pred_facing=%s eng_facing=%s action=%d"),
						Now, Ctx.GetEntity(It).Index, *P.Rotation.ToString(), *Rotation.ToString(), *P.Facing.ToString(), *DesiredMovement.DesiredFacing.ToString(), int32(P.Action));
				}
			}
		}
	});
	LowResEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Ctx.GetFragmentView<FMassMoveTargetFragment>();
		const TConstArrayView<FTransformFragment> LocationList = Ctx.GetFragmentView<FTransformFragment>();
		for (FMassExecutionContext::FEntityIterator It = Ctx.CreateEntityIterator(); It; ++It)
		{
			const int32* Found = Index.Find(Ctx.GetEntity(It));
			if (!Found || !GLabPreds[*Found].bLowRes)
			{
				++Missing; // 가변 틱이 돌지 않은 청크는 예측도 엔진도 건드리지 않는다
				continue;
			}
			const FLabOrientPred& P = GLabPreds[*Found];
			if (!LabSameBits(P.MoveForward, MoveTargetList[It].Forward))
			{
				++Changed;
				continue;
			}
			++LoCompared;
			const FQuat Rotation = LocationList[It].GetTransform().GetRotation();
			if (!LabSameBits(P.Rotation, Rotation))
			{
				++LoMismatch;
				if (Reported < 10)
				{
					++Reported;
					UE_LOG(LogLab, Warning, TEXT("lab_smoothori_mismatch t=%.2f lo entity=%d pred_rot=%s eng_rot=%s"), Now, Ctx.GetEntity(It).Index, *P.Rotation.ToString(), *Rotation.ToString());
				}
			}
		}
	});
	++TotFrames;
	++LogFrames;
	TotHiCompared += HiCompared;
	TotHiMismatch += HiMismatch;
	TotLoCompared += LoCompared;
	TotLoMismatch += LoMismatch;
	TotChanged += Changed;
	TotMissing += Missing;
	TotChangedVel += ChangedVel;
	TotChangedTarget += ChangedTarget;
	if (LogSince <= 0.)
	{
		LogSince = Now;
	}
	else if (Now - LogSince >= 5.)
	{
		// 누계다. 실행이 끝날 때의 마지막 줄이 전체다
		UE_LOG(LogLab, Log, TEXT("lab_smoothori_check t=%.1f frames=%lld hi_compared=%lld hi_mismatch=%lld lo_compared=%lld lo_mismatch=%lld changed=%lld changed_vel=%lld changed_target=%lld missing=%lld"),
			Now, TotFrames, TotHiCompared, TotHiMismatch, TotLoCompared, TotLoMismatch, TotChanged, TotChangedVel, TotChangedTarget, TotMissing);
		LogSince = Now;
		LogFrames = 0;
	}
#endif
}
