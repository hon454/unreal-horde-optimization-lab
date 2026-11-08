#pragma once

#include "CoreMinimal.h"
#include "MassUpdateISMProcessor.h"
#include "LabVatProcessor.generated.h"

/**
 * M2 VAT ISM 갱신. 엔진 UMassUpdateISMProcessor를 상속해 한 반복에서 위치와 애니 프레임(인스턴스 커스텀 데이터)을 함께 쓴다.
 * 커스텀 데이터는 위치와 같은 순서로 쌓여야 하므로 같은 반복에서 쓴다(MassVisualizationComponent.cpp:566-571).
 * 구조는 City Sample MassCrowdUpdateISMVertexAnimationProcessor와 같다(../unreal-horde-references/CitySample/Plugins/CitySampleMassCrowd).
 *
 * 엔진 프로세서와 이 프로세서 중 하나만 돈다. 기본은 꺼져 있고, 게임 모듈 시작 때 -LabStage가 M2 이상이면
 * 엔진 것을 끄고 이것을 켠다(HordeOptLab.cpp, plan.md "M2", 2026-11-08 사용자 결정).
 *
 * 애니 상태는 로직의 태그와 남은 시간에서 읽는다. 걷기는 일련번호로 위상을 흩은 루프, 피격·사망은 상태에 들어간 뒤 지난 시간이다.
 * 프레임은 여기서 계산해 커스텀 데이터 0 = Frame, 1 = PrevFrame으로 넘긴다(VAT 머티리얼 AutoPlay 끔, Scripts/bake-vat.py).
 */
UCLASS()
class ULabVatISMProcessor : public UMassUpdateISMProcessor
{
	GENERATED_BODY()
public:
	ULabVatISMProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};

/** VAT 머티리얼이 읽는 인스턴스 커스텀 데이터(GetFrameSwitch, AutoPlay 끔). */
struct FLabVatFrameData
{
	float Frame = 0.f;
	float PrevFrame = 0.f;
};
