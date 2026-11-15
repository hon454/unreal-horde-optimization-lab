#include "LabConfig.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

const FLabConfig& FLabConfig::Get()
{
	static const FLabConfig Config = []
	{
		FLabConfig C;
		const TCHAR* Cmd = FCommandLine::Get();
		FParse::Value(Cmd, TEXT("-LabN="), C.N);
		FParse::Value(Cmd, TEXT("-LabPreset="), C.Preset);
		FParse::Value(Cmd, TEXT("-LabSeed="), C.Seed);
		FParse::Value(Cmd, TEXT("-LabHealthFixed="), C.HealthFixed);
		FParse::Value(Cmd, TEXT("-LabLabel="), C.Label);
		FParse::Value(Cmd, TEXT("-LabDuration="), C.DurationSec);
		FParse::Value(Cmd, TEXT("-LabWarmup="), C.WarmupSec);
		FParse::Value(Cmd, TEXT("-LabShotAt="), C.ShotAtSec);
		FParse::Value(Cmd, TEXT("-LabFullyParallel="), C.FullyParallel);
		FParse::Value(Cmd, TEXT("-LabTypes="), C.Types);
		C.Types = FMath::Clamp(C.Types, 1, 2);
		FParse::Value(Cmd, TEXT("-LabFarM="), C.FarM);
		C.FarM = FMath::Max(C.FarM, 200);
		C.bBench = FParse::Param(Cmd, TEXT("LabBench"));
		C.bHud = FParse::Param(Cmd, TEXT("LabHud")); // Q5: -LabBench와 함께여도 켠다(촬영 실행). CSV 메타데이터 LabHud로 남는다

		// 사다리 단계(Docs/v2-plan.md "사다리", Docs/Worklog/V2.md "W10" 단계별 스위치 표). 옛 이름(M0~M5)과 -LabVariant는 v2 W10에서 지웠다. 받으면 종료 코드 2
		FString Stage;
		if (FParse::Value(Cmd, TEXT("-LabStage="), Stage))
		{
			if (Stage.Len() == 1 && Stage[0] >= TCHAR('1') && Stage[0] <= TCHAR('8'))
			{
				C.Step = Stage[0] - TCHAR('0');
			}
			else
			{
				C.bStageValid = false;
			}
		}
		FString Variant;
		if (FParse::Value(Cmd, TEXT("-LabVariant="), Variant))
		{
			C.bVariantValid = false;
		}

		// 단계 기본값. 한 단계는 앞 단계에서 한 묶음만 바꾼다
		C.bAvoid = C.Step == 2;                          // 2: 엔진 회피(D43). 3부터는 끔
		C.bSeparate = C.Step != 2;                       // 1: 액터 캡슐끼리 막기, 3부터: Lab 분리 프로세서(D45, D47)
		C.bScreenSigBudget = C.Step >= 4;                // 4: 화면 크기 애니 예산
		const int32 DefaultFixMask = C.Step >= 4 ? 31 : 0; // 4: 전환 대책 다섯과 최소 유지 시간(VAT 인스턴스와 액터 사이)
		C.HydrateFixMask = DefaultFixMask;
		C.RepHoldMs = C.Step >= 4 ? FLabQuality::DefaultRepHoldMs : 0;
		C.bEntityPool = C.Step >= 5;                     // 5: Simulation LOD와 엔티티 풀
		C.bCrowdFx = C.Step >= 7;                        // 7: 배경 무리
		C.ActorCap = C.Step == 8 ? FLabQuality::FinalActorCap : FLabQuality::DefaultActorCap; // 8: 품질 올리기
		C.HydrateM = C.Step == 8 ? FLabQuality::FinalHydrateM : FLabQuality::DefaultHydrateM;
		C.bFarShadow = C.Step != 8;

		// Q5: 단계 기본값과 다르게 준 인자를 모은다(LabOverrides, HUD). 기본값과 같은 값을 준 인자는 넣지 않는다.
		TArray<FString> Overrides;
		auto NoteOverride = [&Overrides](const FString& What) { Overrides.Add(What); };
		// 0/1 스위치: 단계 기본값과 다르면 "<이름>=<값>"으로 적는다
		auto Switch = [Cmd, &NoteOverride](const TCHAR* Key, const TCHAR* Name, bool& Value)
		{
			int32 V = -1;
			if (FParse::Value(Cmd, Key, V) && V >= 0)
			{
				if ((V != 0) != Value) { NoteOverride(FString::Printf(TEXT("%s=%d"), Name, V != 0 ? 1 : 0)); }
				Value = V != 0;
			}
		};
		Switch(TEXT("-LabScreenSigBudget="), TEXT("screensig_budget"), C.bScreenSigBudget);
		// 애니메이션 예산 값(LabConfig.h BudgetMs). 0 이하는 예산 끔이고 -LabScreenSigBudget=0과 같은 길(bScreenSigBudget 거짓)을 쓴다.
		if (FParse::Value(Cmd, TEXT("-LabBudgetMs="), C.BudgetMs))
		{
			if (C.BudgetMs != FLabScenario::BudgetMs) { NoteOverride(TEXT("budget_ms=") + FString::SanitizeFloat(C.BudgetMs, 0)); } // 4 → "4", 2.5 → "2.5"
			if (C.BudgetMs <= 0.f) { C.bScreenSigBudget = false; }
		}
		Switch(TEXT("-LabEntityPool="), TEXT("pool"), C.bEntityPool);
		Switch(TEXT("-LabAvoid="), TEXT("avoid"), C.bAvoid);
		Switch(TEXT("-LabSeparate="), TEXT("separate"), C.bSeparate);
		// v2 W7 표현 전환의 최소 유지 시간(LabConfig.h RepHoldMs). 0이면 끈다
		{
			int32 RepHold = -1;
			if (FParse::Value(Cmd, TEXT("-LabRepHold="), RepHold) && RepHold >= 0)
			{
				if (RepHold != C.RepHoldMs) { NoteOverride(FString::Printf(TEXT("rep_hold=%d"), RepHold)); }
				C.RepHoldMs = RepHold;
			}
		}
		// v2 W8 움직이는 오브용 기법(LabConfig.h bFxOrbFollow, BoundsStepM)
		Switch(TEXT("-LabFxOrbFollow="), TEXT("fx_orb_follow"), C.bFxOrbFollow);
		{
			float BoundsStepM = -1.f;
			if (FParse::Value(Cmd, TEXT("-LabBoundsStepM="), BoundsStepM) && BoundsStepM >= 0.f)
			{
				if (BoundsStepM != C.BoundsStepM) { NoteOverride(FString::Printf(TEXT("bounds_step=%g"), BoundsStepM)); }
				C.BoundsStepM = BoundsStepM;
			}
		}
		// v2 W9 ISKM 번쩍임(LabConfig.h bIskmTrackReserve)
		Switch(TEXT("-LabIskmTrackReserve="), TEXT("iskm_track_reserve"), C.bIskmTrackReserve);
#if !UE_BUILD_TEST
		// 개발 빌드에서만 분리 파라미터를 바꿔 본다(LabSeparation.h). Test 빌드는 기본값이다
		{
			int32 SepVel = -1;
			FParse::Value(Cmd, TEXT("-LabSepIter="), C.Separation.Iterations);
			FParse::Value(Cmd, TEXT("-LabSepRelax="), C.Separation.Relax);
			FParse::Value(Cmd, TEXT("-LabSepMaxStep="), C.Separation.MaxStepFrac);
			FParse::Value(Cmd, TEXT("-LabSepSkin="), C.Separation.Skin);
			if (FParse::Value(Cmd, TEXT("-LabSepVel="), SepVel) && SepVel >= 0) { C.Separation.bVelocity = SepVel != 0; }
			C.Separation.Iterations = FMath::Clamp(C.Separation.Iterations, 1, 16);
		}
#endif
		// Q2 Niagara 배경 무리와 Q5b 바깥 독립 입자(기본 1). 바깥 입자는 배경 무리를 켰을 때만 기본값과 다른 인자로 적는다
		Switch(TEXT("-LabCrowdFx="), TEXT("crowdfx"), C.bCrowdFx);
		{
			int32 CrowdFxOuter = -1;
			if (FParse::Value(Cmd, TEXT("-LabCrowdFxOuter="), CrowdFxOuter) && CrowdFxOuter >= 0)
			{
				C.bCrowdFxOuter = CrowdFxOuter != 0;
				if (!C.bCrowdFxOuter && C.bCrowdFx) { NoteOverride(TEXT("crowdfx_outer=0")); }
			}
		}
		// Q3 빼 보기 스위치(끄기 전용): 0을 줄 때만 그 기법을 뺀다. 다른 값은 단계 기본값 그대로다(LabConfig.h).
		auto OffSwitch = [Cmd](const TCHAR* Key)
		{
			int32 V = -1;
			return FParse::Value(Cmd, Key, V) && V == 0;
		};
		C.bNoHydrate = OffSwitch(TEXT("-LabHydrate="));
		C.bNoSimLod = OffSwitch(TEXT("-LabSimLod="));
		C.bNoIskm = OffSwitch(TEXT("-LabIskm="));
		C.bNoFixedBounds = OffSwitch(TEXT("-LabFixedBounds="));
		C.bNoFarCull = OffSwitch(TEXT("-LabFarCull="));
		if (C.bNoHydrate) { NoteOverride(TEXT("hydrate=0")); }
		if (C.bNoSimLod) { NoteOverride(TEXT("simlod=0")); }
		if (C.bNoIskm) { NoteOverride(TEXT("iskm=0")); }
		if (C.bNoFixedBounds) { NoteOverride(TEXT("fixed_bounds=0")); }
		if (C.bNoFarCull) { NoteOverride(TEXT("far_cull=0")); }
		// 하이드레이션 액터 캡슐 콜리전(LabConfig.h ActorCollision): 기본 Trace(v2 W5). -LabActorCollision=0 끔, =1 Pawn 프로필 켬, =2 Trace
		{
			int32 ActorCol = -1;
			if (FParse::Value(Cmd, TEXT("-LabActorCollision="), ActorCol) && ActorCol >= 0)
			{
				const FLabConfig::EActorCollision Wanted = ActorCol == 0 ? FLabConfig::EActorCollision::Off
					: ActorCol == 1 ? FLabConfig::EActorCollision::On : FLabConfig::EActorCollision::Trace;
				if (Wanted != C.ActorCollision) { NoteOverride(FString::Printf(TEXT("actor_collision=%d"), FMath::Min(ActorCol, 2))); }
				C.ActorCollision = Wanted;
			}
		}
		// v2 W11 액터 비용 후보(LabConfig.h bFixedSkelBounds, bQueryIsolate)
		Switch(TEXT("-LabFixedSkelBounds="), TEXT("fixed_skel_bounds"), C.bFixedSkelBounds);
		Switch(TEXT("-LabQueryIsolate="), TEXT("query_isolate"), C.bQueryIsolate);
		// Q4 근거리 품질: 단계 기본값(8은 1,000·40m), 그다음 인자
		const int32 DefaultCap = C.ActorCap;
		const float DefaultHydrateM = C.HydrateM;
		FParse::Value(Cmd, TEXT("-LabActorCap="), C.ActorCap);
		FParse::Value(Cmd, TEXT("-LabHydrateM="), C.HydrateM);
		C.ActorCap = FMath::Max(C.ActorCap, 0);
		C.HydrateM = FMath::Clamp(C.HydrateM, 1.f, FLabQuality::MaxHydrateM);
		if (C.ActorCap != DefaultCap) { NoteOverride(FString::Printf(TEXT("actor_cap=%d"), C.ActorCap)); }
		if (C.HydrateM != DefaultHydrateM) { NoteOverride(FString::Printf(TEXT("hydrate_m=%.0f"), C.HydrateM)); }
		// Q4b 원거리 그림자: 단계 8은 끔, 그 밖은 켬. -LabFarShadow=0|1이 이긴다(빼 보기 = 단계 8 + -LabFarShadow=1)
		Switch(TEXT("-LabFarShadow="), TEXT("far_shadow"), C.bFarShadow);
		// 시나리오 쪽 인자: 기본값(확정값)과 다르면 적는다
		FParse::Value(Cmd, TEXT("-LabHydrateFixMask="), C.HydrateFixMask);
		if (C.HydrateFixMask != DefaultFixMask) { NoteOverride(FString::Printf(TEXT("fixmask=%d"), C.HydrateFixMask)); }
		if (C.Types != 2) { NoteOverride(FString::Printf(TEXT("types=%d"), C.Types)); }
		if (C.FarM != 200) { NoteOverride(FString::Printf(TEXT("far=%d"), C.FarM)); }
		if (C.Seed != 20261106) { NoteOverride(FString::Printf(TEXT("seed=%d"), C.Seed)); }
		if (C.HealthFixed > 0.f) { NoteOverride(FString::Printf(TEXT("health_fixed=%.0f"), C.HealthFixed)); }
		if (C.FullyParallel == 0) { NoteOverride(TEXT("fully_parallel=0")); }
		// v2 떠다니는 오브(LabConfig.h FLabOrbit). 인자는 m, m/s이고 안에서는 cm다. 주지 않으면 FLabScenario의 궤도다
		{
			float RadiusM = C.Orbit.RadiusCm / 100.f, SpeedMs = C.Orbit.SpeedCms / 100.f, HeightM = C.Orbit.HeightCm / 100.f;
			FParse::Value(Cmd, TEXT("-LabOrbOrbitRadius="), RadiusM);
			FParse::Value(Cmd, TEXT("-LabOrbOrbitSpeed="), SpeedMs);
			FParse::Value(Cmd, TEXT("-LabOrbHeight="), HeightM);
			C.Orbit.RadiusCm = FMath::Max(RadiusM, 0.f) * 100.f;
			C.Orbit.SpeedCms = FMath::Max(SpeedMs, 0.f) * 100.f;
			C.Orbit.HeightCm = FMath::Max(HeightM, 0.f) * 100.f;
			if (!C.Orbit.IsDefault()) { NoteOverride(TEXT("orbit=") + C.Orbit.ToString()); }
		}
		C.Overrides = FString::Join(Overrides, TEXT("|")); // CSV 메타데이터 행은 쉼표로 나뉘므로 | 로 잇는다(LabCVars와 같다)
		C.N = FMath::Max(C.N, 1);
		C.Preset = C.Preset == 2 ? 2 : 1; // 옛 P3(따라가는 오브 시점)은 P1에 합쳤다
		return C;
	}();
	return Config;
}

TArray<FLabConfig::FTech> FLabConfig::TechList() const
{
	auto B = [](bool b) { return FString(b ? TEXT("1") : TEXT("0")); };
	const bool bMass = IsMass();
	const FString Fix = !bMass ? TEXT("0") : HydrateFixMask == 31 ? TEXT("1") : HydrateFixMask == 0 ? TEXT("0") : FString::Printf(TEXT("m%d"), HydrateFixMask);
	return {
		{ TEXT("LabTechInstAnim"), TEXT("anim"), B(UsesVat()) }, // v2 W10: 인스턴스가 움직이는가(단계 4·5 VAT, 6부터 ISKM). 단계 2·3은 엔진 ISM 프로세서와 정지 메시
		{ TEXT("LabTechHydrate"), TEXT("hyd"), B(UsesHydration()) },
		{ TEXT("LabTechAnimBudget"), TEXT("bud"), B(UsesBudgetActors()) },
		{ TEXT("LabTechHydrateFix"), TEXT("fix"), Fix },
		{ TEXT("LabTechPool"), TEXT("pool"), B(bEntityPool) },
		{ TEXT("LabTechSimLod"), TEXT("sim"), B(UsesSimLod()) },
		{ TEXT("LabTechIskm"), TEXT("iskm"), B(UsesIskm()) },
		{ TEXT("LabTechFixedBounds"), TEXT("bnd"), B(Step >= 6 && !bNoFixedBounds) }, // 설정 기준(ISKM을 끈 빼 보기에서도 1). 실제 적용은 UsesFixedBounds와 LabIskmBounds
		{ TEXT("LabTechFarCull"), TEXT("far"), B(CullsFar()) },
		{ TEXT("LabTechCrowdFx"), TEXT("fx"), B(bCrowdFx) },
		{ TEXT("LabTechAvoid"), TEXT("av"), B(UsesAvoidance()) },
		{ TEXT("LabTechSeparate"), TEXT("sep"), B(UsesSeparation() || UsesActorBlocking()) }, // 단계 1은 캡슐끼리 막기(v2 W6), Mass 단계는 분리 프로세서
	};
}

FString FLabConfig::TechString() const
{
	TArray<FString> Parts;
	for (const FTech& T : TechList())
	{
		Parts.Add(FString::Printf(TEXT("%s:%s"), T.LogKey, *T.Value));
	}
	return TEXT("tech=") + FString::Join(Parts, TEXT(",")); // 한 키로 묶는다(로그의 pool= 등 기존 키와 겹치지 않게)
}

const TCHAR* FLabConfig::ActorCollisionString() const
{
	if (IsActorCrowd())
	{
		// v2 W6: 단계 1 액터 캡슐은 늘 켬이다(CharacterMovement가 캡슐로 바닥을 찾는다). Pawn 프로필 + 저격 채널 막음이고, 캡슐끼리는 겹침 방지가 켜지면 막는다(UsesActorBlocking)
		return TEXT("pawn");
	}
	if (!UsesHydration())
	{
		return TEXT("-"); // 하이드레이션 액터가 없다
	}
	switch (ActorCollision)
	{
	case EActorCollision::On: return TEXT("1");
	case EActorCollision::Trace: return TEXT("trace");
	default: return TEXT("0");
	}
}

FLabConfig::FLadder FLabConfig::Ladder() const
{
	// Docs/v2-plan.md "사다리". 이름은 HUD(영어, 엔진 글꼴)에 쓴다
	static const TCHAR* Names[9] = { TEXT(""), TEXT("Actor baseline"), TEXT("Mass, engine path"), TEXT("Lab separation"), TEXT("Animated instances, anim budget"),
		TEXT("Simulation LOD + entity pool"), TEXT("ISKM + following bounds"), TEXT("Niagara background crowd"), TEXT("") };
	return { FString::FromInt(Step), Step == 8 ? FString::Printf(TEXT("Actor cap %d, hydrate %.0f m"), ActorCap, HydrateM) : FString(Names[FMath::Clamp(Step, 0, 8)]) };
}

float FLabConfig::CrowdFxTotalRate() const
{
	return bCrowdFxOuter ? float(N) / FLabScenario::MapRadius * FLabScenario::MoveSpeed : 0.f; // Q5b: -LabCrowdFxOuter=0이면 독립 입자 없음
}

float FLabConfig::CrowdFxLargeRate() const
{
	return Types == 2 ? CrowdFxTotalRate() * FLabCrowdFx::LargeShare : 0.f;
}

float FLabConfig::CrowdFxSmallRate() const
{
	return CrowdFxTotalRate() - CrowdFxLargeRate(); // 1종이면 전부 소형
}

float FLabConfig::CrowdFxLifetime() const
{
	return (FLabCrowdFx::OuterRadius - CrowdFxInnerCm()) / FLabScenario::MoveSpeed;
}

int32 FLabConfig::CrowdFxParticles() const
{
	const float Lifetime = CrowdFxLifetime();
	return bCrowdFx && Lifetime > 0.f ? FMath::RoundToInt(CrowdFxTotalRate() * Lifetime) : 0;
}

FString FLabConfig::SimLodString() const
{
	if (!UsesSimLod())
	{
		return TEXT("-");
	}
	return FString::Printf(TEXT("%.0f/%.0fm tick=0/%.2f/%.2f"), FLabSimLod::MediumDistance / 100.f, FLabSimLod::LowDistance / 100.f, FLabSimLod::TickMedium, FLabSimLod::TickLow);
}

float FLabConfig::HealthFor(uint32 Serial) const
{
	if (HealthFixed > 0.f)
	{
		return HealthFixed;
	}
	// murmur3 마무리 혼합. 시드와 일련번호만 입력이라 같은 실행 조건이면 같은 값이다.
	uint32 H = uint32(Seed) ^ (Serial * 0x9E3779B1u);
	H ^= H >> 16; H *= 0x85EBCA6Bu;
	H ^= H >> 13; H *= 0xC2B2AE35u;
	H ^= H >> 16;
	const float U = float(H >> 8) * (1.f / 16777216.f); // [0, 1)
	return FLabScenario::HealthMin + U * (FLabScenario::HealthMax - FLabScenario::HealthMin);
}

FString FLabConfig::ToLogString() const
{
	// far는 기본(200)이 아닐 때만, health_fixed는 디버그 인자를 줬을 때만, crowdfx는 켰을 때만 붙인다.
	const FString Far = FarM != 200 ? FString::Printf(TEXT(" far=%d"), FarM) : FString();
	const FString Health = HealthFixed > 0.f ? FString::Printf(TEXT(" health_fixed=%.0f"), HealthFixed) : FString();
	const FString Crowd = bCrowdFx ? FString::Printf(TEXT(" crowdfx=1 crowdfx_particles=%d crowdfx_outer=%d"), CrowdFxParticles(), bCrowdFxOuter ? 1 : 0) : FString();
	// tech는 빼 보기 기법의 켬·끔이다(Q3). 모든 실행에 붙여 러너와 사람이 기대값과 대조한다.
	return FString::Printf(TEXT("lab_config stage=%d n=%d preset=P%d seed=%d bench=%d label=%s duration=%.1f types=%d simlod=%s%s pool=%d screensig_budget=%d actor_cap=%d hydrate_m=%.0f far_shadow=%s%s%s %s overrides=%s hud=%d actor_col=%s rep_hold=%d fixmask=%d fx_orb_follow=%s bounds_step=%s iskm_track_reserve=%s fixed_skel_bounds=%s query_isolate=%d"),
		Step, N, Preset, Seed, bBench ? 1 : 0, *Label, DurationSec, Types, *SimLodString(), *Far, bEntityPool ? 1 : 0, bScreenSigBudget ? 1 : 0, ActorCap, HydrateM, FarShadowString(),
		*Health, *Crowd, *TechString(), Overrides.IsEmpty() ? TEXT("-") : *Overrides, bHud ? 1 : 0, ActorCollisionString(), UsesRepHold() ? RepHoldMs : 0, HydrateFixMask,
		*FxOrbFollowString(), *BoundsStepString(), *IskmTrackReserveString(), FixedSkelBoundsString(), bQueryIsolate ? 1 : 0);
}

FString FLabConfig::IskmTrackReserveString() const
{
	return UsesIskm() ? FString(bIskmTrackReserve ? TEXT("1") : TEXT("0")) : FString(TEXT("-"));
}

FString FLabConfig::FxOrbFollowString() const
{
	return UsesCrowdFxEntity() ? FString(bFxOrbFollow ? TEXT("1") : TEXT("0")) : FString(TEXT("-"));
}

FString FLabConfig::BoundsStepString() const
{
	return UsesFixedBounds() ? FString::Printf(TEXT("%g"), BoundsStepM) : FString(TEXT("-"));
}
