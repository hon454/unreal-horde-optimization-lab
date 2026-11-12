# 결정 기록(ADR)

대안을 비교해 내린 결정과 그 이유를 남긴다. 마일스톤 순서와 실험 선택은 [plan.md](../plan.md)에 두고 여기에 두지 않는다. 승인된 ADR의 본문은 고치지 않고, 바뀌면 새 ADR로 대체한다.

## 쓰는 법

파일 이름 `NNNN-영문-slug.md`. 절: 상태(제안됨 / 승인됨 / 대체됨 → NNNN), 날짜, 맥락, 결정, 고려한 대안(표: 대안 | 버린 이유), 결과(실행에서 확인한 것. 나중에 덧붙인다).

| ADR | 결정 | 상태 |
| --- | --- | --- |
| [0001](0001-test-package-for-measurement.md) | 수치는 Test 구성 패키지에서 `csvprofile`과 트레이스로 잰다. Development 빌드는 개발과 오버레이용 | 승인됨 |
| [0002](0002-freeze-measurement-infra.md) | 측정 인프라(시나리오, 스포너, 오브·이펙트, 카메라, 러너, 기록 형식)를 이펙트를 켠 E0 재측정 뒤 동결한다 | 승인됨 |
| [0003](0003-two-camera-presets.md) | 카메라 프리셋을 오브 시점(P1)과 탑뷰(P2) 둘로 줄인다 | 승인됨 |
| [0004](0004-stages-as-run-modes.md) | 최적화 단계를 한 빌드 안의 실행 모드(`-LabStage=`)로 공존시킨다 | 승인됨 |
| [0005](0005-entity-config-in-code.md) | Mass EntityConfig를 에셋 없이 실행 시 C++로 만든다 | 승인됨 |
| [0006](0006-three-measurement-tiers.md) | 측정을 세 층(개발 확인 / 마일스톤 묶음 / 총괄 묶음)으로 나누고 단계마다 60fps 경계 둘레만 잰다 | 승인됨 |
| [0007](0007-keep-default-motion-blur.md) | ISKM 모션 블러 줄무늬가 있어도 모션 블러는 엔진 기본값(켬)으로 두고 측정한다 | 승인됨 |
| [0008](0008-game-scene-baseline-and-target-spec.md) | 게임 장면(건물, 방향장, 비켜 가기, 부위 판정, 래그돌)을 새 기준선으로 삼고 목표 사양(1만, 60fps)에서 프레임을 맞춘다 | 대체됨(0009) |
| [0009](0009-flat-scenario-quality-by-distance.md) | 평지 시나리오로 돌아가 목표 사양 안에서 거리별 품질을 확보한다(하이드레이션, ISKM, Niagara 배경 무리). 사다리와 빼 보기 표로 이점을 보인다 | 승인됨 |
| [0010](0010-final-representation-bands.md) | 최종 구성의 표현을 거리 네 구간으로 나눈다: 액터 0\~40m(상한 1,000), ISKM 40\~80m 그림자 켬, ISKM 80\~200m 그림자 끔, 200m 밖 입자 | 승인됨 |
| [0011](0011-entity-driven-crowd-particles.md) | 200m 밖 배경 무리를 두 겹으로 만든다: 200\~500m는 실제 엔티티 자리에 그리는 입자, 500\~1,000m는 독립 입자 | 승인됨 |
| [0012](0012-ablation-switches-and-quality-metrics.md) | 빼 보기 행은 끄는 인자 하나로 고른다. 전환 점수와 빠른 회전 지표는 Development 참고값으로 낸다 | 승인됨 |
| [0013](0013-full-batch-runner-and-condition-table.md) | 총괄 묶음은 새 러너 `run-full.ps1`로 돌리고 조건 표 21개에는 단계별 기술과 성능 변화만 둔다 | 승인됨 |
| [0014](0014-post-format.md) | 글 본문은 표와 Mermaid 없이 SVG 그림과 산문으로 쓰고, 수치의 표는 측정 기록에 둔다(루트 README 부분은 0015로 대체) | 승인됨 |
| [0015](0015-readme-figures.md) | 루트 README도 비교가 요점인 수치는 SVG 그림으로 보이고, 표는 찾아 읽는 목록에만 쓴다 | 승인됨 |

## ADR 이전의 결정

계획 단계(2026-11-06, 사용자의 볼트 노트와 리뷰)에서 확정한 것이다. 승인된 ADR과 같이 다시 열지 않는다.

| 결정 | 내용 | 이유 |
| --- | --- | --- |
| 진실 원천 | 작업 문서의 원본은 이 저장소다. 볼트 노트는 초안이며 삭제해도 된다 | 두 곳 동기화 비용. 저장소가 공개 포트폴리오라 결정 기록 자체가 산출물 |
| 범위 | M0\~M5 필수. M6 나이아가라 배경, 넉다운, Niagara Examples Pack, 3종째는 backlog | 핵심 축(M0→M3)이 경력과 직접 연결된다. 범위가 핵심 대비 약 2배였다 |
| 개체 종류 | 소형 1종으로 시작. 대형은 M3b "새 기준선"에서 추가 | 2종은 모든 단계의 작업을 2배로 만들지만 쓰이는 실험은 크기 보정 하나 |
| 이름 | 저장소 `unreal-horde-optimization-lab`, 엔진 프로젝트 `HordeOptLab`, 접두사 `Lab`. 한국어 제목 "Unreal 호드 최적화 실험실" | "horde"가 시나리오(목표로 수렴하는 적대 집단)를 가장 정확히 설명한다. Unreal 빌드 도구 Horde와의 이름 충돌을 알고 선택했다. 글에서는 "Mass 기반 crowd optimization"으로 풀어 쓴다 |
| 엔진 | 5.8 소스 빌드(DS 실험실과 같은 설치본) | Mass의 `check` 디버깅과 필요 시 엔진 패치. 런처 빌드와 섞지 않는다 |
| 원거리 표현 순서 | ISM + VAT(AnimToTexture)를 먼저, ISKM + `AnimSequenceTransformProviderData`를 M5에서 비교 | VAT는 City Sample이 검증했다. ISKM은 Epic이 권장하는 경로지만 Experimental이고 5.8.4에도 트랙 풀 문제가 남아 있다 |
| 근거리 구조 | 액터 하이드레이션(`MassRepresentation` HighResActor + 풀) | 업계 표준. "통합 Mass"는 backlog |
| 에셋 | Paragon: Minions(Fab, Epic 무료). 커밋하지 않는다 | 무료, Epic 유지, 애니 376개, 소형·대형이 한 팩. 대체는 UE5 Manny + 리타겟 애니 |
| AI 범위 | 태그 상태 + 커스텀 프로세서로 시작. StateTree + Signal은 M4b 선택. ZoneGraph·NavMesh는 쓰지 않는다. ADR-0008이 방향장으로 바꿨다가 [ADR-0009](0009-flat-scenario-quality-by-distance.md)가 되돌렸다 | 목표가 고정이라 경로 탐색이 필요 없다. 직진 스티어링 + 회피 |
| 오브 | 고정 중심 목표. 근접 소멸 + 주기적 원거리 저격(결정적 해시로 대상 선택). 피해는 체력 프래그먼트 차감과 태그까지. ADR-0008이 바꿨다가 [ADR-0009](0009-flat-scenario-quality-by-distance.md)가 되돌렸다. 체력은 개체마다 다르다 | 모든 거리 밴드에서 피격·사망·표현 전환을 예측 가능하게 일으킨다. GAS·전투 시스템 없음 |
| 카메라 | 고정 프리셋 3종(궤도 / 탑뷰 / 지상), 역할을 정해 동결. 자유 관전은 개발용. **[ADR-0003](0003-two-camera-presets.md)으로 대체**(오브 시점 / 탑뷰 2종) | 프리셋이 다르면 밴드 분포가 달라 비교할 수 없다 |
| 초기 스폰 | 정상 상태 분포(밀도 ∝ 1/r)로 미리 놓는다 | 가장자리에서만 생성하면 30초 워밍업으로 중심에 도달하지 못해 액터 티어가 비어 있다 |
| 시드와 재현성 | 고정 시드는 초기 조건·스폰 위치·저격 순서를 재현한다. 궤적은 비결정적이므로 3회 범위를 함께 보고한다 | 가변 프레임 시간과 Mass 병렬 처리로 같은 시드도 궤적이 다르다 |
| 작업 환경 | Windows PC에서 Claude Code + Unreal MCP. 문서 작업은 저장소 안에서 | 코드, 빌드, 패키징, 실행, 로그를 한곳에서 |
| 글 언어 | 한국어 | 국내 채용 대상 |
