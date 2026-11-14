#pragma once

#include "CoreMinimal.h"
#include "Async/ParallelFor.h"
#include "Templates/TypeHash.h"

/**
 * v2 W4 겹침 방지, 직접 구현(Docs/v2-spec.md 3절, Docs/Worklog/V2.md "W4"). 엔진 회피(W3)가 못 한 세 가지를 피한다:
 * (1) 속도 조향이 아니라 위치를 직접 고친다. 뭉친 무리에서도 겹침이 풀린다.
 * (2) 이웃 수에 상한이 없다. 공간 해시 격자에서 겹칠 수 있는 이웃을 모두 본다.
 * (3) 밀어낼 양의 계산을 ParallelFor로 나눈다.
 * 방식: Jacobi 반복. 반복마다 격자를 다시 만들고, 모든 개체가 같은 위치 사본을 읽어 밀어낼 양을 셈한 뒤 한꺼번에 옮긴다. 읽는 쪽과 쓰는 쪽이 갈려 순서와 스레드에 무관하다.
 * 쌍 하나의 겹침 깊이(반지름 합 − 중심 거리)는 몸통 넓이(반지름²)에 반비례해 나눈다. 대형(94cm)이 소형(40cm)을 더 민다.
 * 이 파일은 Mass를 모른다. ULabSeparationProcessor(LabMassProcessors.h)가 개체를 모아 넣고 결과를 위치와 속도에 쓴다.
 */
/**
 * 기본값은 개발 빌드 1만, 최종 구성, 150초 실행으로 골랐다(Docs/Worklog/V2.md "W4" "파라미터 고르기"). 같은 실행의 분리 끔은 겹친 쌍 1,536, 깊게 겹친 쌍 424, 누적 사망 8,528이었다.
 * 반복 2, Skin 0.2, 속도 투영 끔: 겹친 쌍 329, 깊게 겹친 쌍 0, 누적 사망 8,362.
 */
struct FLabSeparationParams
{
	int32 Iterations = 2;     // 한 프레임의 Jacobi 반복 수
	float Relax = 0.5f;       // 반복마다 쌍의 몫 가운데 푸는 비율. 이웃이 많을 때 넘치게 밀지 않게 1보다 작게 둔다
	float MaxStepFrac = 0.5f; // 한 반복에서 개체가 움직이는 최대 거리 = 자기 반지름 × 이 값(새로 생긴 개체가 겹쳐 있어도 튀지 않게)
	/**
	 * 밀린 개체의 바라는 속도에서 밀린 방향의 반대 성분을 뺀다(막힌 개체가 계속 밀고 들어오지 않게). 기본 끔.
	 * 켜면 맞닿은 무리 전체가 느려져 피해 범위에 드는 개체가 줄었다(개발 실행 150초 누적 사망 8,528 → 4,989). 끄면 사망 수가 그대로다.
	 */
	bool bVelocity = false;
	/**
	 * 떼어 놓는 거리 = 반지름 합 × (1 + Skin). 조향이 매 프레임 3m/s × 프레임 시간만큼 밀고 들어오므로 반지름 합까지만 떼면 얕은 겹침이 늘 남아 겹친 쌍으로 세어진다.
	 * 조금 더 떼어 두면 그 밀림이 여유 안에서 끝난다. 겹친 쌍의 정의(반지름 합)는 바꾸지 않는다. 개발 실행 150초 겹친 쌍: 0.05 840, 0.2 329.
	 */
	float Skin = 0.2f;
};

struct FLabSeparationSolver
{
	/** 입력: 개체의 수평 위치(cm)와 몸통 반지름. Solve 뒤 Pos는 고친 위치, Push는 이번 프레임에 옮긴 양이다 */
	TArray<FVector2f> Pos;
	TArray<float> Radius;
	TArray<FVector2f> Push;

	void Reset()
	{
		Pos.Reset();
		Radius.Reset();
		Push.Reset();
		MaxRadius = 0.f;
	}

	void Add(float X, float Y, float R)
	{
		Pos.Emplace(X, Y);
		Radius.Add(R);
		MaxRadius = FMath::Max(MaxRadius, R);
	}

	/** bParallel이 거짓이면 한 스레드에서 돈다(같은 결과) */
	void Solve(const FLabSeparationParams& Params, bool bParallel)
	{
		const int32 Num = Pos.Num();
		Push.Reset();
		Push.SetNumZeroed(Num);
		if (Num < 2 || MaxRadius <= 0.f)
		{
			return;
		}
		Scale = 1.f + FMath::Max(Params.Skin, 0.f);
		Cell = 2.f * MaxRadius * Scale; // 떼어 놓을 두 개체(거리 < 반지름 합 × Scale ≤ 칸)는 같은 칸이거나 이웃 칸에 있다
		InvCell = 1.f / Cell;
		Next.SetNumUninitialized(Num);
		const int32 NumBatches = FMath::DivideAndRoundUp(Num, BatchSize);
		const EParallelForFlags Flags = bParallel ? EParallelForFlags::None : EParallelForFlags::ForceSingleThread;
		for (int32 Iter = 0; Iter < FMath::Max(Params.Iterations, 1); ++Iter)
		{
			Build();
			ParallelFor(NumBatches, [this, &Params, Num](int32 Batch)
			{
				const int32 End = FMath::Min(Num, (Batch + 1) * BatchSize);
				for (int32 I = Batch * BatchSize; I < End; ++I)
				{
					const FVector2f Delta = Resolve(I, Params);
					Next[I] = Pos[I] + Delta;
					Push[I] += Delta;
				}
			}, Flags);
			Swap(Pos, Next);
		}
	}

private:
	static constexpr int32 BatchSize = 128;
	float MaxRadius = 0.f;
	float Scale = 1.f; // 1 + Skin
	float Cell = 1.f;
	float InvCell = 1.f;
	uint32 Mask = 0;
	TArray<FVector2f> Next;
	TArray<int32> CellStart; // 해시 칸마다 Sorted의 시작(크기 = 칸 수 + 1)
	TArray<int32> Sorted;    // 해시 칸 순으로 늘어놓은 개체 번호

	static uint32 HashCell(int32 CX, int32 CY)
	{
		return (uint32(CX) * 73856093u) ^ (uint32(CY) * 19349663u);
	}

	/** 해시 칸으로 계수 정렬한다(O(N + 칸 수)). 칸 수는 개체 수 두 배 이상의 2의 거듭제곱이다. 다른 칸이 같은 해시를 받으면 후보만 늘고 거리 검사가 거른다 */
	void Build()
	{
		const int32 Num = Pos.Num();
		const uint32 TableSize = FMath::RoundUpToPowerOfTwo(uint32(FMath::Max(2 * Num, 64)));
		Mask = TableSize - 1;
		CellStart.Reset();
		CellStart.SetNumZeroed(int32(TableSize) + 1);
		Sorted.SetNumUninitialized(Num);
		for (int32 I = 0; I < Num; ++I)
		{
			++CellStart[HashOf(Pos[I])];
		}
		int32 Sum = 0;
		for (uint32 H = 0; H < TableSize; ++H)
		{
			Sum += CellStart[H];
			CellStart[H] = Sum; // 칸의 끝
		}
		CellStart[TableSize] = Num;
		for (int32 I = Num - 1; I >= 0; --I)
		{
			Sorted[--CellStart[HashOf(Pos[I])]] = I; // 끝에서 채우면 끝이 시작이 된다
		}
	}

	uint32 HashOf(const FVector2f& P) const
	{
		return HashCell(FMath::FloorToInt32(P.X * InvCell), FMath::FloorToInt32(P.Y * InvCell)) & Mask;
	}

	/** 개체 I가 이번 반복에 옮길 양. Pos(사본)만 읽는다 */
	FVector2f Resolve(int32 I, const FLabSeparationParams& Params) const
	{
		const FVector2f P = Pos[I];
		const float R = Radius[I];
		const float R2 = R * R;
		const int32 CX = FMath::FloorToInt32(P.X * InvCell);
		const int32 CY = FMath::FloorToInt32(P.Y * InvCell);
		uint32 Seen[9];
		int32 NumSeen = 0;
		FVector2f Delta(0.f, 0.f);
		for (int32 DY = -1; DY <= 1; ++DY)
		{
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				const uint32 H = HashCell(CX + DX, CY + DY) & Mask;
				bool bSeen = false;
				for (int32 S = 0; S < NumSeen; ++S)
				{
					bSeen |= Seen[S] == H;
				}
				if (bSeen)
				{
					continue; // 이웃 칸 둘이 같은 해시면 한 번만 본다(쌍을 두 번 밀지 않게)
				}
				Seen[NumSeen++] = H;
				for (int32 K = CellStart[H], End = CellStart[H + 1]; K < End; ++K)
				{
					const int32 J = Sorted[K];
					if (J == I)
					{
						continue;
					}
					const FVector2f D = P - Pos[J];
					const float RJ = Radius[J];
					const float SumR = (R + RJ) * Scale;
					const float DistSq = D.SizeSquared();
					if (DistSq >= SumR * SumR)
					{
						continue;
					}
					const float Dist = FMath::Sqrt(DistSq);
					FVector2f Dir;
					if (Dist > 0.01f)
					{
						Dir = D / Dist;
					}
					else
					{
						// 같은 자리: 쌍마다 정해진 방향으로 서로 반대로 민다
						const uint32 Seed = MurmurFinalize32(HashCombineFast(MurmurFinalize32(uint32(FMath::Min(I, J))), uint32(FMath::Max(I, J)))); // engine-notes.md 34절
						const float A = float(Seed & 0xffffu) * (UE_TWO_PI / 65536.f);
						Dir = FVector2f(FMath::Cos(A), FMath::Sin(A)) * (I < J ? 1.f : -1.f);
					}
					const float RJ2 = RJ * RJ;
					Delta += Dir * ((SumR - Dist) * RJ2 / (R2 + RJ2));
				}
			}
		}
		Delta *= Params.Relax;
		const float MaxStep = R * Params.MaxStepFrac;
		const float StepSq = Delta.SizeSquared();
		if (StepSq > MaxStep * MaxStep)
		{
			Delta *= MaxStep / FMath::Sqrt(StepSq);
		}
		return Delta;
	}
};
