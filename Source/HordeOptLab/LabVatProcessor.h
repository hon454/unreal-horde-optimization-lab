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
 * 엔진 것을 끄고 이것을 켠다(HordeOptLab.cpp, plan.md "M2").
 *
 * 애니 상태는 표현 전용 FLabAnimStateFragment(상태, 들어간 시각)에서 읽는다. 재생 시각 = 월드 시각 − 들어간 시각. 걷기는 위상을 음수 시각으로 둔 루프다.
 * 로직 프래그먼트와 태그는 읽지 않는다(M3b 정비 1, Docs/Worklog/M3b.md). 그룹 LabLogic 뒤에 돈다는 선언은 변형 m3order에서만 한다(LabVatProcessor.cpp). 기본은 로직과의 선후가 없다(LabMassProcessors.h LabExplicitLogicOrder).
 * 진단 변형 m3qold는 정비 전처럼 FLabMinionFragment와 태그를 읽고 순서 선언이 없다.
 * 프레임은 여기서 계산해 커스텀 데이터 0 = Frame, 1 = PrevFrame으로 넘긴다(VAT 머티리얼 AutoPlay 끔, Scripts/bake-vat.py).
 * 걷기 루프 끝(End + 0.99)이 다음 클립 첫 행과 섞이지 않는 근거: 머티리얼 함수 BoneIndexUVs가 int(Frame)으로 행을 자르고 텍스처 필터가 Nearest다(engine-notes.md 12절).
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

#if !UE_BUILD_TEST
#include "LabSnipe.h"
#include "HordeOptLab.h"
/**
 * 개발용 전환 진단: 사건 하나를 프레임 번호와 화면 좌표(1280x720 창 기준)로 로그에 남긴다. 연속 캡처(-LabShotBurst)와 맞춰 본다.
 * 사건 이름: hydrate·shown·dehydrate, snipe_small·snipe_large, die(피격 → 사망), removed. 앞쪽 필드(frame, u, v)는 Scripts/transition-*.py가 읽으므로 바꾸지 않는다.
 * wx·wy는 월드 좌표다. 저격당한 개체는 멈추므로 snipe·die·removed를 이 값으로 잇는다.
 * x는 사건별 덧붙임 값이다(hydrate·shown·dehydrate: 대형이면 1, die: 피격이 끝난 시각보다 늦게 넘어간 초).
 * m4offmove + -LabSimLodLog: offenter·offleave(Simulation LOD Off 태그가 붙고 떨어질 때, LabMassProcessors.cpp). 카메라 앞의 개체만 남는다.
 */
inline void LabLogScreenEvent(const TCHAR* What, const FLabSnipeView& View, const FVector& World, float Extra = 0.f)
{
	const FVector V = World + FVector(0.f, 0.f, 90.f) - View.Location;
	const float Depth = FVector::DotProduct(V, View.Forward);
	if (Depth <= 1.f) { return; }
	const float X = FVector::DotProduct(V, View.Right) / Depth / View.TanHalfH;
	const float Y = FVector::DotProduct(V, View.Up) / Depth / (View.TanHalfH * 9.f / 16.f);
	UE_LOG(LogLab, Log, TEXT("lab_event %s frame=%llu u=%.0f v=%.0f dist=%.0f wx=%.0f wy=%.0f x=%.3f"), What, (unsigned long long)GFrameCounter, (X + 1.f) * 640.f, (1.f - Y) * 360.f, V.Size2D(), World.X, World.Y, Extra);
}
#endif

/** VAT 머티리얼이 읽는 인스턴스 커스텀 데이터(GetFrameSwitch, AutoPlay 끔). */
struct FLabVatFrameData
{
	float Frame = 0.f;
	float PrevFrame = 0.f;
};
