#pragma once

#include "CoreMinimal.h"
#include "Algo/BinarySearch.h"

/**
 * 겹친 쌍의 수(v2 W3, Docs/v2-spec.md 3절). 단계 사이의 동등성 지표다: 겹침 방지가 같은 수준으로 도는지 본다.
 * 몸통은 수평면의 원(중심, 반지름)이다. 두 원의 중심 거리가 반지름 합보다 짧으면 "겹친 쌍", 반지름 합의 절반보다 짧으면 "깊게 겹친 쌍"이다.
 * 사망 중인 개체와 풀에 든 개체는 세지 않는다. LabOverlapSampleSec마다 한 번 세고, 그 사이의 프레임은 같은 값을 남긴다(CSV 스탯 Lab/OverlapPairs, Lab/OverlapDeep).
 */
struct FLabBody
{
	float X = 0.f;
	float Y = 0.f;
	float Radius = 0.f;
	uint64 Key = 0; // 격자 칸(LabCountOverlaps가 채운다)
};

inline constexpr double LabOverlapSampleSec = 0.5;

/**
 * 균일 격자로 겹친 쌍을 센다. 칸의 한 변은 가장 큰 반지름의 두 배라서 겹칠 수 있는 쌍은 같은 칸이거나 이웃 칸에 있다.
 * 칸 번호로 정렬한 뒤 칸마다 자기 칸과 "앞쪽" 이웃 넷만 보아 쌍을 한 번씩 센다. Bodies의 순서가 바뀐다.
 */
inline void LabCountOverlaps(TArray<FLabBody>& Bodies, float MaxRadius, int32& OutPairs, int32& OutDeep)
{
	OutPairs = 0;
	OutDeep = 0;
	const float Cell = FMath::Max(2.f * MaxRadius, 1.f);
	constexpr int32 Bias = 1 << 20; // 음수 좌표의 칸 번호를 양수로 옮긴다(맵 반경 1km ÷ 칸 0.8m보다 충분히 크다)
	auto MakeKey = [](int32 CX, int32 CY) { return (uint64(uint32(CX)) << 32) | uint64(uint32(CY)); };
	for (FLabBody& Body : Bodies)
	{
		Body.Key = MakeKey(FMath::FloorToInt32(Body.X / Cell) + Bias, FMath::FloorToInt32(Body.Y / Cell) + Bias);
	}
	Bodies.Sort([](const FLabBody& A, const FLabBody& B) { return A.Key < B.Key; });
	auto Test = [&OutPairs, &OutDeep](const FLabBody& A, const FLabBody& B)
	{
		const float DX = A.X - B.X;
		const float DY = A.Y - B.Y;
		const float Sum = A.Radius + B.Radius;
		const float DistSq = DX * DX + DY * DY;
		if (DistSq < Sum * Sum)
		{
			++OutPairs;
			if (DistSq < 0.25f * Sum * Sum)
			{
				++OutDeep;
			}
		}
	};
	static const int32 Forward[4][2] = { { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 } };
	const int32 Num = Bodies.Num();
	for (int32 Begin = 0; Begin < Num;)
	{
		const uint64 Key = Bodies[Begin].Key;
		int32 End = Begin + 1;
		while (End < Num && Bodies[End].Key == Key)
		{
			++End;
		}
		for (int32 A = Begin; A < End; ++A)
		{
			for (int32 B = A + 1; B < End; ++B)
			{
				Test(Bodies[A], Bodies[B]);
			}
		}
		const int32 CX = int32(Key >> 32);
		const int32 CY = int32(Key & 0xffffffffu);
		for (const int32* Offset : Forward)
		{
			const uint64 Other = MakeKey(CX + Offset[0], CY + Offset[1]);
			for (int32 B = Algo::LowerBoundBy(Bodies, Other, &FLabBody::Key); B < Num && Bodies[B].Key == Other; ++B)
			{
				for (int32 A = Begin; A < End; ++A)
				{
					Test(Bodies[A], Bodies[B]);
				}
			}
		}
		Begin = End;
	}
}
