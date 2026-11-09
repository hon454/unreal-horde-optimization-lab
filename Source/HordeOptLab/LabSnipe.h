#pragma once

#include "CoreMinimal.h"
#include "LabConfig.h"

/**
 * 오브 저격 대상 선택 규칙(모든 단계 공통, 2026-11-07 사용자 결정).
 * 화면(카메라 시야) 안에 있는 산 개체만 후보이고, 거리 구간마다 정해진 수만큼 해시(일련번호, 저격 틱)가 가장 작은 개체를 고른다.
 * 카메라 경로는 시간으로만 정해지므로 같은 시드·같은 시각이면 같은 규칙으로 고른다.
 */
struct FLabSnipeView
{
	FVector Location = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	float TanHalfH = 1.f; // 수평 FOV 절반의 tan
	float TanHalfV = 1.f; // 수직(종횡비로 계산)
	bool bValid = false;

	static FLabSnipeView Make(const FVector& InLocation, const FRotator& Rotation, float FovDegrees, float AspectRatio)
	{
		FLabSnipeView View;
		const FRotationMatrix M(Rotation);
		View.Location = InLocation;
		View.Forward = M.GetUnitAxis(EAxis::X);
		View.Right = M.GetUnitAxis(EAxis::Y);
		View.Up = M.GetUnitAxis(EAxis::Z);
		View.TanHalfH = FMath::Tan(FMath::DegreesToRadians(FovDegrees * 0.5f));
		View.TanHalfV = View.TanHalfH / FMath::Max(AspectRatio, 0.01f);
		View.bValid = true;
		return View;
	}

	/** 점이 시야(앞쪽 사각뿔) 안에 있는가. 몸통 높이를 보려고 호출자가 z를 올려 넘긴다. */
	bool Contains(const FVector& Point) const
	{
		const FVector D = Point - Location;
		const float Depth = FVector::DotProduct(D, Forward);
		if (Depth <= 0.f)
		{
			return false;
		}
		return FMath::Abs(FVector::DotProduct(D, Right)) <= Depth * TanHalfH
			&& FMath::Abs(FVector::DotProduct(D, Up)) <= Depth * TanHalfV;
	}
};

struct FLabSnipeBands
{
	/**
	 * 오브 중심에서의 수평 거리 구간 경계(cm). 거리 밴드 확정값(30/80/200m)과 같다. 30m 안은 쏘지 않는다: 몇 초 안에 피해 범위에 들어와
	 * 거기서 죽기 때문이다(2026-11-07 사용자 의견). 근거리 밴드의 사망은 피해 범위가 만든다.
	 */
	static constexpr float Edges[4] = { 3000.f, 8000.f, 20000.f, FLabScenario::MapRadius + 1.f };
	/** 구간마다 고르는 수(30~80m, 80~200m, 200m~). 합이 FLabScenario::SnipeCount(5)다. */
	static constexpr int32 Counts[3] = { 1, 2, 2 };
	static constexpr int32 Num = 3;

	/** 구간 번호. 범위 밖이면 INDEX_NONE. */
	static int32 BandOf(float Distance2D)
	{
		for (int32 i = 0; i < Num; ++i)
		{
			if (Distance2D >= Edges[i] && Distance2D < Edges[i + 1])
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
};

/** 구간별로 해시가 가장 작은 K개를 모은다. T는 대상(액터 포인터나 엔티티 핸들). */
template<typename T>
struct TLabSnipePicker
{
	struct FPick { uint32 Hash; T Target; FVector Location; };
	TArray<FPick, TInlineAllocator<2>> Bands[FLabSnipeBands::Num];

	void Offer(int32 Band, uint32 Hash, const T& Target, const FVector& Location)
	{
		auto& Best = Bands[Band];
		if (Best.Num() < FLabSnipeBands::Counts[Band])
		{
			Best.Add({ Hash, Target, Location });
			return;
		}
		int32 Worst = 0;
		for (int32 i = 1; i < Best.Num(); ++i)
		{
			if (Best[i].Hash > Best[Worst].Hash) { Worst = i; }
		}
		if (Hash < Best[Worst].Hash)
		{
			Best[Worst] = { Hash, Target, Location };
		}
	}

	template<typename F>
	void ForEach(F&& Func) const
	{
		for (const auto& Best : Bands)
		{
			for (const FPick& Pick : Best)
			{
				Func(Pick);
			}
		}
	}
};

/**
 * 2종 저격 규칙(E안, 2026-11-09 사용자 결정, plan.md "M3b" "저격 대상 규칙"). 저격 수·간격·구간 배분(1/2/2)은 그대로다.
 * 대형은 예약된 발에서만 쏘고 나머지 발은 소형만 고른다. 저격 틱이 4의 배수면 200m 밖 구간(2)의 1발, 4로 나눈 나머지가 2면 80~200m 구간(1)의 1발이
 * 대형 전용이다(그 구간 화면 안 대형 중 해시 최소). 홀수 틱은 예약이 없다. 예약 구간 화면 안에 대형이 없으면 그 발은 소형으로 떨어진다.
 * 1종(대형 없음)에서는 예약 발이 늘 소형으로 떨어져 TLabSnipePicker만 쓸 때와 같은 대상이 같은 순서로 나온다(같은 시드에서 kills·snipes가 같다).
 */
inline int32 LabLargeReservedBand(uint32 SnipeTick)
{
	return SnipeTick % 4u == 0u ? 2 : SnipeTick % 4u == 2u ? 1 : INDEX_NONE;
}

template<typename T>
struct TLabSnipeSelector
{
	using FPick = typename TLabSnipePicker<T>::FPick;
	TLabSnipePicker<T> Small;   // 소형 후보(구간별 Counts개). 1종에서는 모든 개체
	FPick LargeBest{};          // 예약 구간의 대형 중 해시 최소
	bool bHasLarge = false;
	int32 ReservedBand = INDEX_NONE;

	explicit TLabSnipeSelector(uint32 SnipeTick) : ReservedBand(LabLargeReservedBand(SnipeTick)) {}

	void Offer(int32 Band, uint32 Hash, const T& Target, const FVector& Location, bool bLarge)
	{
		if (!bLarge)
		{
			Small.Offer(Band, Hash, Target, Location);
		}
		else if (Band == ReservedBand && (!bHasLarge || Hash < LargeBest.Hash))
		{
			LargeBest = { Hash, Target, Location };
			bHasLarge = true;
		}
	}

	/** 구간 순서로 대상을 넘긴다. 예약 구간에 대형이 있으면 그 발을 대형이 가져가고 소형 후보 중 해시가 가장 큰 하나를 뺀다. */
	template<typename F>
	void ForEach(F&& Func) const
	{
		for (int32 Band = 0; Band < FLabSnipeBands::Num; ++Band)
		{
			const auto& Best = Small.Bands[Band];
			int32 Skip = INDEX_NONE;
			if (Band == ReservedBand && bHasLarge)
			{
				Func(LargeBest);
				if (Best.Num() >= FLabSnipeBands::Counts[Band])
				{
					Skip = 0;
					for (int32 i = 1; i < Best.Num(); ++i) { if (Best[i].Hash > Best[Skip].Hash) { Skip = i; } }
				}
			}
			for (int32 i = 0; i < Best.Num(); ++i)
			{
				if (i != Skip) { Func(Best[i]); }
			}
		}
	}
};

/** 화면 판정에 쓰는 몸통 높이(cm). M0 액터는 캡슐 중심(지면 위 88)이고 Mass 엔티티는 발밑(지면)이라 지면 기준으로 맞춘다. */
constexpr float LabSnipeAimHeight = 100.f;
