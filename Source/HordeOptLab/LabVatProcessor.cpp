#include "LabVatProcessor.h"
#include "LabCrowdSubsystem.h"
#include "LabMassProcessors.h" // LabLogicGroupName, LabUseOldQueries
#include "LabMassTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassLODFragments.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationSubsystem.h"
#include "MassVisualizationComponent.h"
#include "MassRepresentationProcessor.h"
#include "HordeOptLab.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

CSV_DEFINE_CATEGORY(Lab, true);

namespace
{
	/** 클립 안의 프레임. 루프면 감고, 한 번 도는 클립이면 끝 프레임에 멈춘다. */
	float ClipFrame(const FLabVatClip& Clip, float Seconds, float SampleRate, bool bLoop)
	{
		const float Length = float(Clip.EndFrame - Clip.StartFrame + 1);
		float Local = Seconds * SampleRate;
		Local = bLoop ? FMath::Fmod(FMath::Max(Local, 0.f), Length) : FMath::Clamp(Local, 0.f, Length - 1.f);
		return float(Clip.StartFrame) + Local;
	}
}

ULabVatISMProcessor::ULabVatISMProcessor()
{
	// 기본은 등록하지 않는다. 게임 모듈이 단계를 보고 켠다(HordeOptLab.cpp).
	bAutoRegisterWithProcessingPhases = false;
	if (LabExplicitLogicOrder())
	{
		// A/B m3order만: 로직 그룹 뒤 선언. 기본은 FLabAnimStateFragment 접근으로 정해지는 순서만 쓴다(결정 4). 엔진의 Representation 뒤 선언은 부모 생성자가 더한다(MassUpdateISMProcessor.cpp:20).
		ExecutionOrder.ExecuteAfter.Add(LabLogicGroupName);
	}
}

void ULabVatISMProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::ConfigureQueries(EntityManager);
	EntityQuery.AddConstSharedRequirement<FLabUnitTypeParams>(EMassFragmentPresence::All); // 2종: 청크의 종류로 VAT 클립을 고른다
	if (LabUseOldQueries())
	{
		EntityQuery.AddRequirement<FLabMinionFragment>(EMassFragmentAccess::ReadOnly); // m3qold: 로직 프래그먼트를 읽어 로직 넷과 자원을 다툰다
	}
	else
	{
		EntityQuery.AddRequirement<FLabAnimStateFragment>(EMassFragmentAccess::ReadOnly);
	}
}

void ULabVatISMProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	CSV_SCOPED_TIMING_STAT(Lab, VatIsm); // 병목 분석용(2026-11-09)
	UWorld* World = Context.GetWorld();
	const ULabCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULabCrowdSubsystem>() : nullptr;
	if (!Crowd || !Crowd->IsActive())
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const float HitDuration = Crowd->HitDuration;
	const float DeathDuration = Crowd->DeathDuration;
	const FVector ViewLocation = Crowd->SnipeView.Location;
	const bool bOld = LabUseOldQueries();
	int32 BandCounts[4] = { 0, 0, 0, 0 };
	int32 OffDrawn = 0; // 개발 확인: Simulation LOD가 Off인데 그려진 개체 수(lab_ismband)
#if !UE_BUILD_TEST
	// E3b (8) 화질 캡처(-LabFrameLog): 화면 안 30m에서 아직 VAT 인스턴스로 그리는 개체 수(LabCrowdSubsystem::FrameDiag)
	static const bool bFrameLog = FParse::Param(FCommandLine::Get(), TEXT("LabFrameLog"));
	const FLabSnipeView FrameView = Crowd->SnipeView;
	std::atomic<int32>* NearVatCounter = bFrameLog ? &Crowd->FrameDiag[GFrameCounter & 1].NearVat : nullptr;
#endif
	// 진단(-LabVatTiming): 처리 안쪽을 나눠 잰다. 액터가 있을 때만 이 프로세서가 2.3ms 느려지는 원인 찾기(2026-11-09)
	static const bool bTiming = FParse::Param(FCommandLine::Get(), TEXT("LabVatTiming"));
	uint64 CyXf = 0, CyCustom = 0, CyChunkSetup = 0;
	int32 Chunks = 0, Entities = 0, IsmEntities = 0;

	const uint64 F0 = bTiming ? FPlatformTime::Cycles64() : 0;
	uint64 CyFirstChunk = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const uint64 C0 = bTiming ? FPlatformTime::Cycles64() : 0;
		if (bTiming && Chunks == 0) { CyFirstChunk = C0 - F0; } // 첫 청크 람다까지 걸린 시간(질의 준비)
		++Chunks;
		Entities += Context.GetNumEntities();
		const FLabVatClips& Clips = Crowd->VatClipsByType[Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType]; // 종류별 VAT(2종). 1종은 [0]
		UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		check(RepresentationSubsystem);
		FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
		// 새 구조: 표현 상태만 읽는다. m3qold: 로직 프래그먼트와 아키타입 태그로 상태를 안다.
		const TConstArrayView<FLabAnimStateFragment> AnimStates = bOld ? TConstArrayView<FLabAnimStateFragment>() : Context.GetFragmentView<FLabAnimStateFragment>();
		const TConstArrayView<FLabMinionFragment> Minions = bOld ? Context.GetFragmentView<FLabMinionFragment>() : TConstArrayView<FLabMinionFragment>();
		const bool bHit = bOld && Context.DoesArchetypeHaveTag(*FLabHitTag::StaticStruct());
		const bool bDead = bOld && Context.DoesArchetypeHaveTag(*FLabDeadTag::StaticStruct());
#if !UE_BUILD_TEST
		const bool bSimOff = Context.DoesArchetypeHaveTag(*FMassOffLODTag::StaticStruct());
#endif
		const float DeltaSeconds = Context.GetDeltaTimeSeconds();
		if (bTiming) { CyChunkSetup += FPlatformTime::Cycles64() - C0; }

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FTransformFragment& TransformFragment = TransformList[EntityIt];
			const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIt];
			FMassRepresentationFragment& Representation = RepresentationList[EntityIt];

			// 액터 준비를 기다리는 동안(bWaitForActorVisualReadiness) 엔진은 ISM을 떼지 않는다. 그동안도 위치와 프레임을 계속 써서 VAT 인스턴스가 멈추지 않게 한다.
			const bool bWaitingForActor = Representation.IsWaitingForActorVisualReadiness() && Representation.PrevRepresentation == EMassRepresentationType::StaticMeshInstance;
			if (Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance || bWaitingForActor)
			{
				const int32 ISMInfoIndex = Representation.StaticMeshDescHandle.ToIndex();
				if (ensureMsgf(ISMInfo.IsValidIndex(ISMInfoIndex), TEXT("Invalid handle index %u for ISMInfosView"), ISMInfoIndex))
				{
					++IsmEntities;
					const uint64 X0 = bTiming ? FPlatformTime::Cycles64() : 0;
					UpdateISMTransform(Context.GetEntity(EntityIt), ISMInfo[ISMInfoIndex], TransformFragment.GetTransform(), Representation.PrevTransform,
						RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);
					if (bTiming) { CyXf += FPlatformTime::Cycles64() - X0; }

					// 상태와 재생 시각. 새 구조는 표현 프래그먼트 하나에서, m3qold는 태그 + 로직의 남은 시간에서 구한다.
					ELabMinionState State;
					float T;
					if (bOld)
					{
						const FLabMinionFragment& Minion = Minions[EntityIt];
						State = bDead ? ELabMinionState::Dead : bHit ? ELabMinionState::Hit : ELabMinionState::Walk;
						T = bDead ? DeathDuration - Minion.StateTimeLeft : bHit ? HitDuration - Minion.StateTimeLeft : Now + LabWalkPhase(Minion.Serial);
					}
					else
					{
						const FLabAnimStateFragment& Anim = AnimStates[EntityIt];
						State = Anim.State;
						T = Now - Anim.StateStartTime;
					}
					const FLabVatClip& Clip = State == ELabMinionState::Dead ? Clips.Death : State == ELabMinionState::Hit ? Clips.Hit : Clips.Walk;
					const bool bLoop = State == ELabMinionState::Walk;
					FLabVatFrameData Data;
					// Q1b: 사망 클립이 DeathDuration에 맞춰 끝나도록 클립 시각에 배율을 곱한다(시즈 < 1)
						const float Rate = State == ELabMinionState::Dead ? Crowd->DeathRateVat[Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType] : 1.f;
						Data.Frame = ClipFrame(Clip, T * Rate, Clips.SampleRate, bLoop);
					Data.PrevFrame = ClipFrame(Clip, (T - DeltaSeconds) * Rate, Clips.SampleRate, bLoop);
#if !UE_BUILD_TEST
						{
							// Q1b 개발 확인: 종류마다 죽는 동안 클립 프레임이 끝 프레임에 처음 닿는 시각(초)을 한 번씩 찍는다. 이 시각이 DeathDuration과 같아야 한다.
							static bool bLogged[LabUnitTypeMax] = {};
							const int32 Ty = Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType;
							if (State == ELabMinionState::Dead && !bLogged[Ty] && Data.Frame >= float(Clip.EndFrame) - 0.01f)
							{
								bLogged[Ty] = true;
								UE_LOG(LogLab, Log, TEXT("lab_death_check path=vat type=%d reached_end_at=%.3f death_duration=%.3f rate=%.4f"), Ty, T, DeathDuration, Rate);
							}
						}
#endif
					const uint64 D0 = bTiming ? FPlatformTime::Cycles64() : 0;
					ISMInfo[ISMInfoIndex].AddBatchedCustomData<FLabVatFrameData>(Data, RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);
					if (bTiming) { CyCustom += FPlatformTime::Cycles64() - D0; }

#if !UE_BUILD_TEST
					if (UE::Mass::Representation::IsValidActorRepresentation(Representation.PrevRepresentation) && Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance)
					{
						LabLogScreenEvent(TEXT("dehydrate"), Crowd->SnipeView, TransformFragment.GetTransform().GetLocation(),
							Context.GetConstSharedFragment<FLabUnitTypeParams>().UnitType == LabUnitLarge ? 1.f : 0.f);
					}
#endif
					// 표현 축(measurement.md): 거리 밴드별로 그린 개체 수. 밴드 경계는 확정값 30/80/200m
					const float Dist = FVector::Dist2D(TransformFragment.GetTransform().GetLocation(), ViewLocation);
					++BandCounts[Dist < 3000.f ? 0 : Dist < 8000.f ? 1 : Dist < 20000.f ? 2 : 3];
#if !UE_BUILD_TEST
					if (bSimOff) { ++OffDrawn; }
					if (NearVatCounter && Dist < 3000.f && FrameView.Contains(TransformFragment.GetTransform().GetLocation() + FVector(0.f, 0.f, 90.f))) { ++*NearVatCounter; }
#endif
				}
			}
			Representation.PrevTransform = TransformFragment.GetTransform();
			Representation.PrevLODSignificance = RepresentationLOD.LODSignificance;
		}
	});

	if (bTiming)
	{
		CSV_CUSTOM_STAT(Lab, VatForEachMs, float(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - F0)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatFirstChunkMs, float(FPlatformTime::ToMilliseconds64(CyFirstChunk)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatXfMs, float(FPlatformTime::ToMilliseconds64(CyXf)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatCustomMs, float(FPlatformTime::ToMilliseconds64(CyCustom)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatChunkSetupMs, float(FPlatformTime::ToMilliseconds64(CyChunkSetup)), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatChunks, Chunks, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatEntities, Entities, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(Lab, VatIsmEntities, IsmEntities, ECsvCustomStatOp::Set);
	}
	CSV_CUSTOM_STAT(Lab, IsmBand0, BandCounts[0], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand1, BandCounts[1], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand2, BandCounts[2], ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(Lab, IsmBand3, BandCounts[3], ECsvCustomStatOp::Set);
#if !UE_BUILD_TEST
	// 개발 확인(-LabSimLodLog): 1초마다 그 프레임에 그린 인스턴스 수를 카메라 거리 구간(30/80/200m)별로 남긴다. off_drawn은 Simulation LOD가 Off인 채 그려진 수다(m4offmove에서 0이어야 한다).
	static const bool bBandLog = FParse::Param(FCommandLine::Get(), TEXT("LabSimLodLog"));
	if (bBandLog)
	{
		static double Since = 0.;
		static int32 OffDrawnMax = 0;
		static int64 OffDrawnSum = 0;
		OffDrawnMax = FMath::Max(OffDrawnMax, OffDrawn);
		OffDrawnSum += OffDrawn;
		if (Now - Since >= 1.)
		{
			UE_LOG(LogLab, Log, TEXT("lab_ismband t=%.1f b0=%d b1=%d b2=%d b3=%d off_drawn_max=%d off_drawn_sum=%lld"), Now, BandCounts[0], BandCounts[1], BandCounts[2], BandCounts[3], OffDrawnMax, (long long)OffDrawnSum);
			Since = Now; OffDrawnMax = 0; OffDrawnSum = 0;
		}
	}
#endif
}
