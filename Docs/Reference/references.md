# 참고 자료

각 항목에 무엇을 얻는지 한 줄. 엔진 소스 경로는 [engine-notes.md](engine-notes.md)에 확인한 것만 적는다.

## 영상

> **개인 학습 중** 표시: 사용자가 프로젝트와 별도로 보며 공부하는 영상. 나중에 포스팅([post-material.md](../post-material.md))이나 프로젝트 문서를 쓸 때 근거·설명 자료로 다시 인용한다.

1. **Unlearning OOP: Data-Oriented Gameplay Design with ECS and Mass Entity** — Jared Shabel, Unreal Fest Chicago 2026. https://www.youtube.com/watch?v=UXA8axmAb-U — **개인 학습 중.** Mass를 OOP가 아닌 데이터 중심으로 설계하는 법(전투 위치 잡기 시스템을 매니저 패턴에서 ECS로 바꾸는 사례). 메모리 효율 논리, 이벤트 대신 엔티티, 태그로 분기, 작은 프로세서·프래그먼트, Mass Debugger. 자막 분석 완료(2026-11-09): [talk-notes.md](talk-notes.md).
2. **언리얼 엔진의 매스 프레임워크와 대규모 씬 처리를 위한 기능들** — 윤준기(에픽게임즈 코리아), 언리얼 페스트 서울 2026. https://www.youtube.com/watch?v=yhtj9xuAu_c — **개인 학습 중.** Mass 프레임워크 구조와 대규모 장면 처리 기능 전반을 한국어로 정리한 세션. 액터 vs Mass 단일 스레드 비교, Spawner·Trait·EntityConfig, 아키타입·청크, Representation LOD와 ISKM, Agent Component. 자막 분석 완료(2026-11-09): [talk-notes.md](talk-notes.md).
3. Mike Acton, **Data-Oriented Design and C++** (CppCon 2014).
4. 포럼 **AnimToTexture Plugin, How to Use it to make Vertex Animation Textures for Crowds** — M2 베이크 절차.

## Epic 문서

- MassEntity Overview — https://dev.epicgames.com/documentation/unreal-engine/overview-of-mass-entity-in-unreal-engine
- MassGameplay Overview — https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-mass-gameplay-in-unreal-engine
- Mass Entity 허브(Avoidance, Debugger, Simplified Processor/Query API) — https://dev.epicgames.com/documentation/en-us/unreal-engine/mass-entity-in-unreal-engine
- Animation Budget Allocator — https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-budget-allocator-in-unreal-engine
- Unreal MCP in Unreal Editor — https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor
- UE 5.8 Release Notes — Mass 절, MetaHuman Crowd, ISKM.
- Tom Looman, **Unreal Engine 5.8 Performance Highlights** — https://tomlooman.com/unreal-engine-5-8-performance-highlights/
- vrealmatic, **crowds in UE5** — https://vrealmatic.com/unreal-engine/crowds — VAT·ISKM·Mass·Animation Sharing 선택지 개요.

## 샘플 프로젝트(`../unreal-horde-references/`, 읽기 전용, git 밖)

```
../unreal-horde-references/
  UnrealEngine/            소스 빌드 Engine/ 심볼릭 링크(또는 5.8.x-release 스파스 클론)
  CitySample/              Fab, Epic. Content/Crowds: VAT 머티리얼, AnimToTexture 데이터, Representation 설정
  MetaHumanCrowdSample/    Fab, Epic, 5.8. Mass + ISKM + AnimSequenceTransformProvider, 액터 ↔ ISKM 동기화.
                           플러그인 MetaHuman Crowd·MetaHuman Crowd Content, MAP_Plaza, BP_CrowdSpawner, DA_MHMassCrowdConfig
  AssetHost/               런처 5.8 엔진의 빈 프로젝트. Fab "프로젝트에 추가" 대상. Content/ParagonMinions가 원본 팩(4.8GB)
```

런처는 소스 빌드를 엔진으로 인식하지 못해 Fab 에셋을 소스 빌드 프로젝트에 직접 추가할 수 없다. 그래서 원본은 AssetHost에 받고, 쓰는 것만 `Scripts/migrate-assets.py`로 HordeOptLab에 옮긴다([commands.md](../Guides/commands.md) "에셋 마이그레이션"). MetaHuman Crowd Sample은 프로젝트가 아니라 콘텐츠 플러그인으로 받았다: 런처 엔진 `G:\Epic Games\UE_5.8\Engine\Plugins\Marketplace\MetaHumanCrowdSample_5.8\`(`MetaHumanCrowdContent.uplugin`). 엔진 플러그인 `MetaHumanCrowd`는 소스 빌드에도 있다(`Engine/Plugins/MetaHuman/MetaHumanCrowd/`).

저장소 안이 아니라 형제 디렉터리에 두는 이유: 라이선스 자료가 커밋될 위험을 없애고, 수십 GB가 git 작업 트리 밖에 있어야 상태 확인이 느려지지 않는다. Claude Code는 `--add-dir ..\unreal-horde-references`로 읽는다.

## 에셋

- **Paragon: Minions** (Fab, Epic, 무료) — https://www.fab.com/listings/039ea035-9360-4e76-ad06-5d3a92da6f65 — 애니 376, Epic Skeleton 스케일, UE 5.0\~5.8. 스탠다드 라이선스, PARAGON 상표 사용 금지, 재배포 금지. 종류별 스켈레톤이 다르다(`Minion_Lane_Core_Skeleton`, `Minion_Lane_Siege_Skeleton`). 피직스 에셋 기본값으로 배치 직후 늘어진다(바디 Physics Type → Default).
- 대체: **Paragon animations retargeted to Manny** (Fab, Kingboars, 무료, UE5 Manny).

## 포럼 스레드(상태 확인용)

- "AnimBank & InstancedSkinnedMeshComponent" (2025-12, Epic 답변: 5.8 목표는 VAT 기능 동등, 트랙 할당 API)
- "AnimBank and Instanced Skinned Mesh Component" (2026-06, Epic: 5.8부터 Mass + ISKM + AnimSequenceTransformProvider 권장)
- "Mass crowds using SkinnedMeshInstance leak GPU memory" (2026-06 — 5.8.4 소스에서도 미수정. engine-notes.md 3절)
- "Paragon Minions Ragdoll Immediately", "Paragon Minion animation assets cannot be changed"

## 도구

- **Unreal Engine Skills for Claude Code** — Epic 저장소 `epicgames/unreal-engine-skills-for-claude-code-plugin`. 30+ 툴셋, UE 관례 주입.
- Unreal Insights, Mass Debugger(에디터, Development), Gameplay Debugger, Visual Logger.

## 형제 프로젝트

- [UE5 Dedicated Server 최적화 실험실](https://github.com/hon454/ue-dedicated-server-optimization-lab) — 같은 측정 철학(고정 시드, 30초 제외 60초, 3회 중앙값과 변동 폭, 한 번에 하나). 이 저장소의 AGENTS.md·STATUS.md·ADR·Scripts 구조의 원형.
