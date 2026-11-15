#include "LabFarStream.h"
#include "LabUnitAssets.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

uint32 FLabFarStream::Init(int32 N, int32 Seed, uint32 FirstSerial, float SpeedCms)
{
	// 스포너 스트림(시드)과 다른 스트림이다. 스포너가 뽑는 순서를 건드리지 않는다
	Stream.Initialize(Seed + 1);
	Speed = SpeedCms;
	Recs.Reset(N);
	uint32 Serial = FirstSerial;
	for (int32 i = 0; i < N; ++i)
	{
		// 반지름 균등 = 밀도 ∝ 1/r. 흐름 = 밀도 × 2πr × 속도 = N / 500m × 3m/s(1만이면 초당 60)로 반지름과 무관하다
		const float Radius = Stream.FRandRange(InnerCm, OuterCm);
		const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
		FLabFarRec& Rec = Recs.AddDefaulted_GetRef();
		Rec.X = Radius * FMath::Cos(Angle);
		Rec.Y = Radius * FMath::Sin(Angle);
		Rec.T0 = 0.;
		Rec.Serial = Serial++;
	}
	Cursor = 0;
	MoveCarry = 0.;
	NextMeasure = 0.;
	return Serial;
}

FVector2D FLabFarStream::LocationAt(const FLabFarRec& Rec, double Now, const FVector& Orb) const
{
	const FVector2D P(Rec.X, Rec.Y);
	const FVector2D To = FVector2D(Orb.X, Orb.Y) - P;
	const double Dist = To.Length();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return P;
	}
	const double Step = FMath::Min(double(Speed) * FMath::Max(Now - Rec.T0, 0.), Dist);
	return P + To / Dist * Step;
}

void FLabFarStream::Tick(double Now, float DeltaSeconds, const FVector& Orb)
{
	if (Recs.Num() == 0)
	{
		return;
	}
	CSV_SCOPED_TIMING_STAT(Lab, FarStream);
	// 이번 프레임의 몫: 개수 × 프레임 초 ÷ 간격. 소수는 다음 프레임으로 넘긴다. 그래서 기록마다 약 1초에 한 번 옮겨진다
	MoveCarry += double(Recs.Num()) * DeltaSeconds / MoveIntervalSec;
	int32 ToMove = FMath::Min(int32(MoveCarry), Recs.Num());
	MoveCarry -= ToMove;
	while (ToMove-- > 0)
	{
		if (Cursor >= Recs.Num())
		{
			Cursor = 0;
		}
		FLabFarRec& Rec = Recs[Cursor++];
		const FVector2D P = LocationAt(Rec, Now, Orb);
		Rec.X = float(P.X);
		Rec.Y = float(P.Y);
		Rec.T0 = Now;
	}
	// 1초마다 맨 앞 기록의 거리와 맵 안에 들어온 수를 잰다(로그·CSV 스탯용)
	if (Now >= NextMeasure)
	{
		NextMeasure = Now + 1.;
		double BestSq = TNumericLimits<double>::Max();
		FVector2D Best = FVector2D::ZeroVector;
		InsideMap = 0;
		for (const FLabFarRec& Rec : Recs)
		{
			const FVector2D P = LocationAt(Rec, Now, Orb);
			const double Sq = FVector2D::DistSquared(P, FVector2D(Orb.X, Orb.Y));
			if (Sq < BestSq) { BestSq = Sq; Best = P; }
			InsideMap += P.SizeSquared() < double(InnerCm) * InnerCm ? 1 : 0;
		}
		FrontCm = float(FMath::Sqrt(BestSq));
		FrontCenterCm = float(Best.Size());
	}
	CSV_CUSTOM_STAT(Lab, FarFrontM, FrontCm / 100.f, ECsvCustomStatOp::Set);
}

void FLabFarStream::AddAtRing(double Now, uint32 Serial)
{
	const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
	FLabFarRec& Rec = Recs.AddDefaulted_GetRef();
	Rec.X = OuterCm * FMath::Cos(Angle);
	Rec.Y = OuterCm * FMath::Sin(Angle);
	Rec.T0 = Now;
	Rec.Serial = Serial;
}

int32 FLabFarStream::Pop(int32 Type, int32 Count, double Now, const FVector& Orb, uint32& NextSerial, TArray<FVector>& OutLocations, TArray<uint32>& OutSerials)
{
	if (Count <= 0)
	{
		return 0;
	}
	CSV_SCOPED_TIMING_STAT(Lab, FarStream);
	// 오브까지의 지금 거리 = 저장된 거리 − 속도 × (지금 − T0)(모든 기록이 같은 속도로 오브를 향하므로 순서가 외삽 거리와 같다). 가장 작은 Count개를 고른다
	struct FPick { double Key; int32 Index; };
	TArray<FPick, TInlineAllocator<8>> Picks;
	const FVector2D O(Orb.X, Orb.Y);
	for (int32 i = 0; i < Recs.Num(); ++i)
	{
		const FLabFarRec& Rec = Recs[i];
		if (FLabUnitTypes::TypeOf(Rec.Serial) != Type)
		{
			continue;
		}
		const double Key = FVector2D::Distance(FVector2D(Rec.X, Rec.Y), O) - double(Speed) * FMath::Max(Now - Rec.T0, 0.);
		if (Picks.Num() < Count)
		{
			Picks.Add({ Key, i });
		}
		else
		{
			int32 Worst = 0;
			for (int32 k = 1; k < Picks.Num(); ++k) { if (Picks[k].Key > Picks[Worst].Key) { Worst = k; } }
			if (Key < Picks[Worst].Key) { Picks[Worst] = { Key, i }; }
		}
	}
	// 가까운 순으로 내준다(재생성 순서가 실행마다 같게)
	Picks.Sort([](const FPick& A, const FPick& B) { return A.Key < B.Key; });
	for (const FPick& Pick : Picks)
	{
		const FLabFarRec& Rec = Recs[Pick.Index];
		const FVector2D P = LocationAt(Rec, Now, Orb);
		OutLocations.Emplace(P.X, P.Y, 0.);
		OutSerials.Add(Rec.Serial);
		const float D = float(FVector2D::Distance(P, O));
		SpawnDistSum += D;
		SpawnDistMin = FMath::Min(SpawnDistMin, D);
		SpawnDistMax = FMath::Max(SpawnDistMax, D);
	}
	// 뒤 색인부터 지운다(RemoveAtSwap이 앞 색인을 건드리지 않게)
	Picks.Sort([](const FPick& A, const FPick& B) { return A.Index > B.Index; });
	for (const FPick& Pick : Picks)
	{
		Recs.RemoveAtSwap(Pick.Index, EAllowShrinking::No);
	}
	for (int32 k = 0; k < Picks.Num(); ++k)
	{
		AddAtRing(Now, NextSerial++);
	}
	PopsTotal += Picks.Num();
	PopsShort += Count - Picks.Num();
	return Picks.Num();
}
