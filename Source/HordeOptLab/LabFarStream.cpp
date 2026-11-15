#include "LabFarStream.h"
#include "LabUnitAssets.h"
#include "LabMassTypes.h" // LabWalkPhase
#include "ProfilingDebugging/CsvProfiler.h"
#include "Misc/ScopeExit.h"

CSV_DECLARE_CATEGORY_EXTERN(Lab);

uint32 FLabFarStream::Init(int32 N, int32 Seed, uint32 FirstSerial, float SpeedCms)
{
	// 스포너 스트림(시드)과 다른 스트림이다. 스포너가 뽑는 순서를 건드리지 않는다
	Stream.Initialize(Seed + 1);
	Speed = SpeedCms;
	Recs.Reset(N);
	RecSlot.Reset(N);
	ShortPos.Reset(N);
	for (int32 Type = 0; Type < LabUnitTypeMax; ++Type)
	{
		Packed[Type].Reset();
		SlotOwner[Type].Reset();
		Short[Type].Reset();
		bShortValid[Type] = false; // 첫 Pop이 만든다
	}
	uint32 Serial = FirstSerial;
	for (int32 i = 0; i < N; ++i)
	{
		// 반지름 균등 = 밀도 ∝ 1/r. 흐름 = 밀도 × 2πr × 속도 = N / 500m × 3m/s(1만이면 초당 60)로 반지름과 무관하다
		const float Radius = Stream.FRandRange(InnerCm, OuterCm);
		const float Angle = Stream.FRandRange(0.f, UE_TWO_PI);
		FLabFarRec Rec;
		Rec.X = Radius * FMath::Cos(Angle);
		Rec.Y = Radius * FMath::Sin(Angle);
		Rec.T0 = 0.;
		Rec.Serial = Serial++;
		AddRec(Rec);
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
#if !UE_BUILD_TEST
	const uint64 StartCycles = FPlatformTime::Cycles64();
	ON_SCOPE_EXIT { TickCycles += FPlatformTime::Cycles64() - StartCycles; };
#endif
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
		const int32 Index = Cursor++;
		FLabFarRec& Rec = Recs[Index];
		const FVector2D P = LocationAt(Rec, Now, Orb);
		Rec.X = float(P.X);
		Rec.Y = float(P.Y);
		Rec.T0 = Now;
		Packed[FMath::Min(FLabUnitTypes::TypeOf(Rec.Serial), LabUnitTypeMax - 1)][RecSlot[Index]] = PackOf(Rec);
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
	FLabFarRec Rec;
	Rec.X = OuterCm * FMath::Cos(Angle);
	Rec.Y = OuterCm * FMath::Sin(Angle);
	Rec.T0 = Now;
	Rec.Serial = Serial;
	AddRec(Rec);
}

FVector4f FLabFarStream::PackOf(const FLabFarRec& Rec)
{
	// 엔티티 원소와 같은 형식(LabCrowdFxProcessor.cpp). T0는 float 월드 초, 위상은 엔티티가 될 때와 같은 LabWalkPhase(일련번호)
	return FVector4f(Rec.X, Rec.Y, float(Rec.T0), LabWalkPhase(Rec.Serial));
}

void FLabFarStream::AddRec(const FLabFarRec& Rec)
{
	const int32 Type = FMath::Min(FLabUnitTypes::TypeOf(Rec.Serial), LabUnitTypeMax - 1);
	RecSlot.Add(Packed[Type].Num());
	SlotOwner[Type].Add(Recs.Num());
	Packed[Type].Add(PackOf(Rec));
	// 후보 목록: 새 기록(1,000m 고리)의 A가 문턱 안이면 넣는다. 보통은 밖이다
	if (bShortValid[Type] && AOf(Rec, ShortRef[Type]) <= ShortThr[Type])
	{
		ShortPos.Add(Short[Type].Num());
		Short[Type].Add(Recs.Num());
	}
	else
	{
		ShortPos.Add(INDEX_NONE);
	}
	Recs.Add(Rec);
}

void FLabFarStream::RebuildShort(int32 Type, int32 Count, const FVector2D& O)
{
	// 1) 이 종류 기록의 A(지금 오브)와 Count번째로 작은 값. 2) 문턱 = 그 값 + 여유. 그 종류 기록이 Count개 이하면 모두 넣는다
	ScratchA.SetNumUninitialized(Recs.Num(), EAllowShrinking::No);
	TArray<double, TInlineAllocator<8>> Smallest;
	for (int32 i = 0; i < Recs.Num(); ++i)
	{
		if (FLabUnitTypes::TypeOf(Recs[i].Serial) != Type)
		{
			continue;
		}
		const double A = AOf(Recs[i], O);
		ScratchA[i] = A;
		if (Smallest.Num() < Count)
		{
			Smallest.Add(A);
		}
		else
		{
			int32 Worst = 0;
			for (int32 k = 1; k < Smallest.Num(); ++k) { if (Smallest[k] > Smallest[Worst]) { Worst = k; } }
			if (A < Smallest[Worst]) { Smallest[Worst] = A; }
		}
	}
	double Kth = TNumericLimits<double>::Max();
	if (Smallest.Num() == Count)
	{
		Kth = Smallest[0];
		for (const double A : Smallest) { Kth = FMath::Max(Kth, A); }
	}
	const double Thr = Kth < TNumericLimits<double>::Max() ? Kth + ShortMarginCm : Kth;
	Short[Type].Reset();
	for (int32 i = 0; i < Recs.Num(); ++i)
	{
		if (FLabUnitTypes::TypeOf(Recs[i].Serial) != Type)
		{
			continue;
		}
		if (ScratchA[i] <= Thr)
		{
			ShortPos[i] = Short[Type].Num();
			Short[Type].Add(i);
		}
		else
		{
			ShortPos[i] = INDEX_NONE;
		}
	}
	ShortRef[Type] = O;
	ShortThr[Type] = Thr;
	bShortValid[Type] = true;
}

#if !UE_BUILD_TEST
bool FLabFarStream::CheckPacked() const
{
	int32 Total = 0;
	for (int32 Type = 0; Type < LabUnitTypeMax; ++Type)
	{
		if (Packed[Type].Num() != SlotOwner[Type].Num())
		{
			return false;
		}
		Total += Packed[Type].Num();
	}
	if (Total != Recs.Num() || RecSlot.Num() != Recs.Num())
	{
		return false;
	}
	for (int32 i = 0; i < Recs.Num(); ++i)
	{
		const int32 Type = FMath::Min(FLabUnitTypes::TypeOf(Recs[i].Serial), LabUnitTypeMax - 1);
		const int32 Slot = RecSlot[i];
		if (!Packed[Type].IsValidIndex(Slot) || SlotOwner[Type][Slot] != i || Packed[Type][Slot] != PackOf(Recs[i]))
		{
			return false;
		}
	}
	return true;
}
#endif

void FLabFarStream::RemoveRecAtSwap(int32 Index)
{
	// 1) 원소: 지울 자리에 그 종류의 마지막 원소를 옮긴다
	const int32 Type = FMath::Min(FLabUnitTypes::TypeOf(Recs[Index].Serial), LabUnitTypeMax - 1);
	const int32 Slot = RecSlot[Index];
	const int32 LastSlot = Packed[Type].Num() - 1;
	const int32 Moved = SlotOwner[Type][LastSlot];
	Packed[Type][Slot] = Packed[Type][LastSlot];
	SlotOwner[Type][Slot] = Moved;
	RecSlot[Moved] = Slot;
	Packed[Type].Pop(EAllowShrinking::No);
	SlotOwner[Type].Pop(EAllowShrinking::No);
	// 2) 후보 목록: 지울 기록이 목록에 있으면 그 자리에 목록의 마지막 색인을 옮긴다
	if (const int32 Pos = ShortPos[Index]; Pos != INDEX_NONE)
	{
		const int32 MovedRec = Short[Type].Last();
		Short[Type][Pos] = MovedRec;
		ShortPos[MovedRec] = Pos;
		Short[Type].Pop(EAllowShrinking::No);
		ShortPos[Index] = INDEX_NONE;
	}
	// 3) 기록: Recs의 마지막 기록이 Index로 옮긴다. 그 원소와 목록 자리의 주인 색인을 고친다
	const int32 Last = Recs.Num() - 1;
	if (Last != Index)
	{
		const int32 LastType = FMath::Min(FLabUnitTypes::TypeOf(Recs[Last].Serial), LabUnitTypeMax - 1);
		RecSlot[Index] = RecSlot[Last];
		SlotOwner[LastType][RecSlot[Last]] = Index;
		ShortPos[Index] = ShortPos[Last];
		if (ShortPos[Last] != INDEX_NONE)
		{
			Short[LastType][ShortPos[Last]] = Index;
		}
	}
	RecSlot.Pop(EAllowShrinking::No);
	ShortPos.Pop(EAllowShrinking::No);
	Recs.RemoveAtSwap(Index, EAllowShrinking::No);
}

int32 FLabFarStream::Pop(int32 Type, int32 Count, double Now, const FVector& Orb, uint32& NextSerial, TArray<FVector>& OutLocations, TArray<uint32>& OutSerials)
{
	if (Count <= 0)
	{
		return 0;
	}
	CSV_SCOPED_TIMING_STAT(Lab, FarStream);
#if !UE_BUILD_TEST
	const uint64 StartCycles = FPlatformTime::Cycles64();
	ON_SCOPE_EXIT { PopCycles += FPlatformTime::Cycles64() - StartCycles; ++PopCalls; };
#endif
	// 오브까지의 지금 거리 = 저장된 거리 − 속도 × (지금 − T0)(모든 기록이 같은 속도로 오브를 향하므로 순서가 외삽 거리와 같다). 가장 작은 Count개를 고른다
	struct FPick { double Key; int32 Index; };
	TArray<FPick, TInlineAllocator<8>> Picks;
	const FVector2D O(Orb.X, Orb.Y);
	auto Consider = [&](int32 i)
	{
		const FLabFarRec& Rec = Recs[i];
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
	};
	// 후보 목록(FLabFarStream.h "후보 목록")만 훑는다. 목록 밖 기록이 더 가까울 수 없음을 확인하지 못하면 목록을 다시 만들고(전수 훑기) 다시 고른다. 고른 결과는 전수 훑기와 같다
	bool bPicked = false;
	if (bShortValid[Type])
	{
		for (const int32 i : Short[Type])
		{
			Consider(i);
		}
		if (Picks.Num() == Count)
		{
			double KStar = Picks[0].Key;
			for (const FPick& Pick : Picks) { KStar = FMath::Max(KStar, Pick.Key); }
			// 목록 밖 기록의 키 ≥ A(기준 오브) − |오브 − 기준 오브| − 속도 × 지금 > 문턱 − |오브 − 기준 오브| − 속도 × 지금. 부동소수 오차 몫 1cm를 둔다
			bPicked = KStar + double(Speed) * Now + FVector2D::Distance(O, ShortRef[Type]) + 1. < ShortThr[Type];
		}
		if (!bPicked)
		{
			Picks.Reset();
		}
	}
	if (!bPicked)
	{
		RebuildShort(Type, Count, O);
		++ShortRebuilds;
		for (const int32 i : Short[Type])
		{
			Consider(i);
		}
	}
#if !UE_BUILD_TEST
	static const bool bCheck = FParse::Param(FCommandLine::Get(), TEXT("LabFarStreamCheck"));
	if (bCheck)
	{
		// 개발 진단: 전수 훑기의 결과와 견준다(고른 색인 집합). 다르면 PickMismatch를 센다. 결과는 후보 목록의 것을 쓴다
		TArray<FPick, TInlineAllocator<8>> Full;
		for (int32 i = 0; i < Recs.Num(); ++i)
		{
			const FLabFarRec& Rec = Recs[i];
			if (FLabUnitTypes::TypeOf(Rec.Serial) != Type) { continue; }
			const double Key = FVector2D::Distance(FVector2D(Rec.X, Rec.Y), O) - double(Speed) * FMath::Max(Now - Rec.T0, 0.);
			if (Full.Num() < Count) { Full.Add({ Key, i }); continue; }
			int32 Worst = 0;
			for (int32 k = 1; k < Full.Num(); ++k) { if (Full[k].Key > Full[Worst].Key) { Worst = k; } }
			if (Key < Full[Worst].Key) { Full[Worst] = { Key, i }; }
		}
		bool bSame = Full.Num() == Picks.Num();
		for (const FPick& F : Full)
		{
			bSame = bSame && Picks.ContainsByPredicate([&F](const FPick& P) { return P.Index == F.Index; });
		}
		PickChecks += 1;
		PickMismatch += bSame ? 0 : 1;
	}
#endif
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
		RemoveRecAtSwap(Pick.Index);
	}
	for (int32 k = 0; k < Picks.Num(); ++k)
	{
		AddAtRing(Now, NextSerial++);
	}
	PopsTotal += Picks.Num();
	PopsShort += Count - Picks.Num();
	return Picks.Num();
}
