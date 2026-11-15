#pragma once

#include "CoreMinimal.h"
#include "LabUnitAssets.h" // LabUnitTypeMax

/**
 * M6 수정 A: 맵 밖 무리(ADR-0020, Docs/Worklog/V2.md "M6 수정의 상세 계획안"). -LabFarHandoff=1(기본)일 때 스포너가 가진다.
 * 기록 하나는 (자리 X·Y, 마지막 이동 시각 T0, 일련번호)다. 종류(일련번호 10의 배수면 대형)와 걷기 위상(LabWalkPhase)은 일련번호의 결정적 값이다.
 * - 처음: 맵 중심 500~1,000m 고리에 N개, 밀도 1/r(반지름 균등). 스포너 스트림과 다른 스트림(시드 + 1)에서 뽑는다.
 * - 이동: 200m 밖 엔티티(ULabOffMoveProcessor)와 같은 규칙. 약 1초 간격으로 오브의 지금 위치를 향해 3m/s × (지금 − T0)만큼 곧게 옮긴다.
 *   한 프레임에 모두 옮기지 않고 프레임 시간만큼의 몫(개수 × 프레임 초 ÷ 1초)을 차례로 옮긴다.
 * - 꺼내기: 개체가 죽으면 같은 종류의 기록 가운데 오브에 가장 가까운 것을 꺼내 그 지금 자리(외삽값)에서 엔티티를 만든다.
 *   꺼낸 수만큼 1,000m 고리의 무작위 각도에 새 기록(새 일련번호)을 더한다. 그래서 기록 수는 늘 N이다.
 * - 그리기: 단계 7·8에서 ULabCrowdFxGatherProcessor가 엔티티 구동 입자 배열 뒤에 붙인다. 원소 형식이 엔티티와 같다(X, Y, T0, 위상).
 * 게임 스레드에서만 읽고 쓴다(게임 모드 틱, 지연 커맨드 플러시 안의 재생성, 게임 스레드 전용 모으기 프로세서).
 */
struct FLabFarRec
{
	float X = 0.f;
	float Y = 0.f;
	double T0 = 0.; // 이 자리에 있던 월드 시각
	uint32 Serial = 0;
};

class FLabFarStream
{
public:
	static constexpr float InnerCm = 50000.f;  // 맵 반경 500m(FLabScenario::MapRadius)
	static constexpr float OuterCm = 100000.f; // 1,000m(FLabCrowdFx::OuterRadius)
	static constexpr double MoveIntervalSec = 1.0; // FLabSimLod::TickLow와 같은 간격

	/** 처음 배치. FirstSerial부터 N개의 일련번호를 쓰고 다음 일련번호를 돌려준다 */
	uint32 Init(int32 N, int32 Seed, uint32 FirstSerial, float SpeedCms);
	bool IsActive() const { return Recs.Num() > 0; }

	/** 게임 모드 틱이 부른다. 프레임 몫만큼 오브 쪽으로 옮긴다 */
	void Tick(double Now, float DeltaSeconds, const FVector& Orb);

	/**
	 * 종류 Type의 기록 가운데 오브에 가장 가까운 Count개를 꺼내 지금 자리와 일련번호를 Out에 더하고, 같은 수의 새 기록을 1,000m 고리에 더한다(새 일련번호는 NextSerial에서).
	 * 그 종류의 기록이 모자라면 꺼낸 수만 돌려준다(부른 쪽이 나머지를 옛 규칙으로 만든다).
	 */
	int32 Pop(int32 Type, int32 Count, double Now, const FVector& Orb, uint32& NextSerial, TArray<FVector>& OutLocations, TArray<uint32>& OutSerials);

	/** 기록의 지금 자리(오브 쪽 외삽, 오브를 넘지 않는다). 엔티티 구동 입자 식과 같다(Scripts/create-crowd-fx.md 5-5) */
	FVector2D LocationAt(const FLabFarRec& Rec, double Now, const FVector& Orb) const;
	const TArray<FLabFarRec>& GetRecs() const { return Recs; }
	/**
	 * 종류 Type 기록의 입자 원소(X, Y, T0, 걷기 위상). 모으기 프로세서가 엔티티 원소 뒤에 그대로 붙인다.
	 * 기록이 바뀔 때(이동, 꺼내기, 더하기)만 그 원소를 고친다. 원소 순서는 Recs 순서와 다를 수 있다. 입자는 원소마다 그 프레임에만 그려지므로 순서는 화면에 닿지 않는다
	 * (Scripts/create-crowd-fx.md 5절, Docs/Worklog/V2.md "M6 코드 비용 줄이기")
	 */
	TConstArrayView<FVector4f> GetPacked(int32 Type) const { return Packed[Type]; }

	// 집계(게임 스레드). 개발 로그 lab_farstream와 CSV 스탯 Lab/FarFrontM
	int32 PopsTotal = 0;
	int32 PopsShort = 0;      // 그 종류의 기록이 모자라 옛 규칙으로 만든 수(0이어야 한다)
	double SpawnDistSum = 0.; // 꺼낸 자리의 오브 거리 합(cm)
	float SpawnDistMin = TNumericLimits<float>::Max();
	float SpawnDistMax = 0.f;
	float FrontCm = 0.f;      // 마지막으로 잰 맨 앞 기록의 오브 거리(1초마다)
	float FrontCenterCm = 0.f; // 그 기록의 맵 중심 거리
	int32 InsideMap = 0;      // 맵 반경 안에 들어온 기록 수(1초마다)
#if !UE_BUILD_TEST
	// 비용 진단(개발 전용, Docs/Worklog/V2.md "M6 코드 비용 줄이기"). 개발 로그 lab_farstream가 읽고 지운다
	uint64 TickCycles = 0;
	uint64 PopCycles = 0;
	int32 PopCalls = 0;
	/** 원소가 기록과 맞는지(개수, 자리표, 값). 개발 로그 lab_farstream의 packed_ok */
	bool CheckPacked() const;
#endif

private:
	void AddAtRing(double Now, uint32 Serial);
	/** Recs 끝에 Rec을 더하고 원소를 만든다 */
	void AddRec(const FLabFarRec& Rec);
	/** Recs[Index]를 RemoveAtSwap으로 지우고 원소도 지운다 */
	void RemoveRecAtSwap(int32 Index);
	static FVector4f PackOf(const FLabFarRec& Rec);
	TArray<FLabFarRec> Recs;
	// 종류별 입자 원소와 그 자리표: RecSlot[i] = Recs[i]의 원소 자리, SlotOwner[종류][자리] = 그 원소의 Recs 색인
	TArray<FVector4f> Packed[LabUnitTypeMax];
	TArray<int32> SlotOwner[LabUnitTypeMax];
	TArray<int32> RecSlot;
	/**
	 * 후보 목록(Pop이 전수 훑기 대신 훑는다, Docs/Worklog/V2.md "M6 코드 비용 줄이기"). A(기록, R) = |자리 − R| + 속도 × T0라 두면 Pop의 키 = A(지금 오브) − 속도 × 지금이다.
	 * 목록 = 만들 때 A(기준 오브 R) ≤ 문턱인 기록. 이동은 A(R)를 줄이지 않고(|자리' − 자리| ≤ 속도 × (지금 − T0)) 새 기록은 더할 때 A(R)를 문턱과 견주므로 목록 밖 기록은 늘 문턱보다 크다.
	 * |A(오브) − A(R)| ≤ |오브 − R|이라 목록에서 고른 Count번째 키 + 속도 × 지금 + |오브 − R| < 문턱이면 전수 훑기와 같은 결과다. 아니면 R = 지금 오브로 다시 만든다.
	 */
	void RebuildShort(int32 Type, int32 Count, const FVector2D& O);
	double AOf(const FLabFarRec& Rec, const FVector2D& R) const { return FVector2D::Distance(FVector2D(Rec.X, Rec.Y), R) + double(Speed) * Rec.T0; }
	static constexpr double ShortMarginCm = 2000.; // 문턱 = Count번째로 작은 A + 20m. 1만에서 소형 약 360개, 약 4초마다 다시 만든다
	TArray<int32> Short[LabUnitTypeMax];
	TArray<int32> ShortPos; // Recs[i]의 목록 자리(없으면 INDEX_NONE)
	FVector2D ShortRef[LabUnitTypeMax];
	double ShortThr[LabUnitTypeMax] = {};
	bool bShortValid[LabUnitTypeMax] = {};
	TArray<double> ScratchA;
public:
	int32 ShortRebuilds = 0; // 집계(개발 로그 lab_farstream의 rebuilds)
	int32 PickChecks = 0;    // -LabFarStreamCheck: 전수 훑기와 견준 수
	int32 PickMismatch = 0;  // 그중 다른 수(0이어야 한다)
private:
	FRandomStream Stream;
	float Speed = 300.f;
	int32 Cursor = 0;
	double MoveCarry = 0.;
	double NextMeasure = 0.;
};
