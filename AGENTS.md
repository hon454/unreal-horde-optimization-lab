# Unreal 호드 최적화 실험실

액터로 만들면 1천도 60fps를 못 지키는 호드를 UE5 Mass Entity로 다시 만들고, 최적화 기법을 하나씩 적용하며 그때마다 수치로 확인하는 프로젝트다. 지금 단계와 다음 할 일은 [Docs/STATUS.md](Docs/STATUS.md)에 있다. 엔진 프로젝트는 `HordeOptLab`, 모듈과 클래스·인자·폴더의 접두사는 `Lab`이다.

## 세션을 시작하면

1. [Docs/STATUS.md](Docs/STATUS.md)를 읽는다. 지금 단계, 다음 할 일, 진행 중인 세션, 매번 쓰는 명령, 막힌 것이 있다.
2. 작업 세션은 브리프의 작업 단위를 한다. 아래 3과 4는 코디네이터의 일이다.
3. STATUS.md에 끝나고 커밋된 작업의 기록이 남아 있으면, 다음 작업 전에 정리안(옮길 항목, 옮길 파일, 남길 한 줄)을 채팅에 보여 주고 승인을 받아 정리하고 따로 커밋한다.
4. STATUS.md "다음 할 일"의 첫 항목을 고른다. 비어 있으면 사용자에게 다음 작업을 확인한다.
5. 그 작업에 필요한 문서만 "문서 지도"에서 찾아 연다. 결정을 바꾸고 싶어지면 먼저 [ADR 목록](Docs/Decisions/README.md)을 본다.

## 문서 지도

| 알고 싶은 것 | 볼 곳 |
| --- | --- |
| 지금 단계, 다음 할 일, 진행 중인 세션, 매번 쓰는 명령, 최근 묶음의 수치, 막힌 것, 사용자에게 요청한 일 | [Docs/STATUS.md](Docs/STATUS.md) |
| 목표 사양, 사다리, 빼 보기 표, 측정 목록, 끝낸 단계의 목표와 실험 | [Docs/plan.md](Docs/plan.md) |
| 글 대기열 | [Docs/backlog.md](Docs/backlog.md) |
| 확정된 결정과 이유, 버린 대안 | [Docs/Decisions/](Docs/Decisions/README.md)(ADR). ADR이 없는 결정은 그 README의 표 |
| 측정 규약: 시나리오 고정값, 빌드 구성, 절차, 지표 이름, 차이 판정, 한계 | [measurement.md](Docs/Guides/measurement.md) |
| 빌드·패키징·벤치마크·그래프 명령과 인자 | [commands.md](Docs/Guides/commands.md). 매번 쓰는 것만 STATUS.md "명령"에 둔다 |
| 최종 구성에서 한 프레임에 도는 코드, 프로세서 순서, 엔티티 구성 | [code-tour.md](Docs/Guides/code-tour.md). 흐름 그림은 `Scripts/plot-code-tour.py`가 만든다 |
| 빌드나 실행이 실패했을 때의 증상별 대처 | [troubleshooting.md](Docs/Guides/troubleshooting.md). 문제가 생겼을 때만 연다 |
| 글과 README의 독자, 틀, 문장 규칙, 그림 규칙 | [posting.md](Docs/Guides/posting.md) |
| 글 재료: 작업 중 질답에서 나온, 독자에게 설명이 필요한 부분 | [post-material.md](Docs/post-material.md). 사용자가 원인을 물어 답한 것은 여기에 더한다 |
| 글 준비: 핵심 인사이트, 글별 재료 선별, 글 재료의 사실 확인 결과 | [post-review.md](Docs/post-review.md). 글 재료를 쓰기 전에 그 항목의 확인 결과를 먼저 본다. 글을 다 쓰면 지운다 |
| 엔진 소스에서 확인한 사실(경로, 기본값, 매크로, 알려진 문제) | [engine-notes.md](Docs/Reference/engine-notes.md). 소스 위치가 필요하면 먼저 여기를 본다 |
| 영상·문서·샘플·에셋·포럼 스레드 | [references.md](Docs/Reference/references.md) |
| 강연 두 편(Shabel, 윤준기)의 요약과 타임스탬프 | [talk-notes.md](Docs/Reference/talk-notes.md) |
| 측정 PC 사양과 엔진 버전 | [pc-specs.md](Docs/Reference/pc-specs.md) |
| 끝낸 작업의 경위와 실패한 실행 | `Docs/Worklog/M<N>.md`, `Q<N>.md`, 글과 README는 `Posts.md`(목록은 [Worklog/README.md](Docs/Worklog/README.md)). 세션 시작 때 읽지 않고 검색만 한다 |
| 코디네이터가 묻지 않고 내린 결정과 사용자 판단이 필요한 항목 | [autonomous-decisions.md](Docs/Worklog/autonomous-decisions.md) |
| 총괄 묶음의 조건 표와 스크립트 목록 | `Scripts/full-conditions.json`, [Scripts/README.md](Scripts/README.md) |
| 글 본문과 이미지 | `Posts/NN-이름/README.md`, `Posts/NN-이름/images/` |
| 글 수치의 근거(실행 라벨, 세 실행의 값, 계산식, 캡처, 소스 위치) | `Posts/NN-이름/measurements.md` |
| 묶음별 측정 결과 | `Measurements/<묶음>/`(총괄 묶음은 `Full1/`). 안에 `summary.csv`(조건별 중앙값·범위), `runs.csv`(실행별 한 행), `shots/` |
| 그래프(SVG)와 그 수치(CSV) | `Docs/figures/`(`Scripts/plot.py`). README의 그림은 `Docs/figures/readme/`(`Scripts/plot-readme.py`), 단계별 화면은 `Docs/figures/steps/` |
| 공개 소개와 결과 요약 | [README.md](README.md) |
| 참고 프로젝트(City Sample, MetaHuman Crowd Sample, Fab 원본 팩) | 형제 디렉터리 `../unreal-horde-references/`(git 밖, 읽기 전용) |
| 실행 산출물(git 밖) | 프레임 CSV `Saved/Profiling/CSV/`, 트레이스 `Saved/Traces/`, 스크린샷 `Saved/Screenshots/Lab/`, 패키지 `Packaged/` |

새 문서를 만들거나 옮기면 이 표를 함께 고친다.

## 규칙

### 측정과 비교

- **수치는 Test 패키지에서만 잰다.** Development 빌드(`-game`)는 개발, 동작 확인, 디버그 오버레이 스크린샷에 쓰고 그 수치는 비교에 쓰지 않는다([ADR-0001](Docs/Decisions/0001-test-package-for-measurement.md)).
- **비교는 같은 묶음에서 잰 것끼리만 한다.** 글과 README의 수치는 가장 최근 총괄 묶음의 값이고, `measurements.md`에 그 묶음의 빌드 해시와 실행 라벨을 적는다.
- **한 비교에서 바꾸는 변수는 하나다.** 바뀐 변수가 둘 이상인 비교는 "참고"로 적고 그래프에 잇지 않는다. 시나리오(스폰 분포, 개체 종류와 비율, 오브 동작, 카메라 프리셋)는 고정이다.
- **측정 인프라는 동결돼 있다**([ADR-0002](Docs/Decisions/0002-freeze-measurement-infra.md)). 스포너, 러너, 시드, 카메라 프리셋, CSV 스키마나 측정 조건을 바꾸면 전 조건을 새 묶음으로 다시 잰다. 그때 글과 README의 수치도 새 묶음의 값으로 고치고, 바뀐 이유를 글의 측정 기록에 한 줄 남긴다.
- **단계는 한 빌드 안의 실행 모드다.** 구현을 지우지 않고 `-LabStage=`로 고르는 단계로 더한다. 이전 단계를 재려고 태그를 체크아웃하지 않는다([ADR-0004](Docs/Decisions/0004-stages-as-run-modes.md)).
- **실패한 실행의 수치는 쓰지 않는다.** 같은 라벨과 묶음 이름은 다시 쓰지 않는다.

### 진행 방식

- **구현과 측정이 먼저고 글은 뒤따른다.** 글, 그래프 해석, 영상, 트레이스 분석은 작업 단위의 완료 조건이 아니다. 글은 최신 main에서 쓴다.
- **개발 속도가 최우선이다.** 단위 테스트, CI, PR, 외부 이슈 트래커를 쓰지 않는다. main에 직접 커밋한다.
- **커밋 메시지는 [Conventional Commits](https://www.conventionalcommits.org/)로 쓴다.** `type(scope): summary` 한 줄, 영어 명령형 소문자. type은 `feat`, `fix`, `docs`, `build`, `refactor`, `chore`, `bench`(측정 결과 추가). 측정 조건이나 설정값을 바꾼 커밋은 본문에 이유를 쓴다.
- **STATUS.md에는 지금 유효한 것만 두고 10KB를 넘기지 않는다.** 세션마다 통째로 읽는 파일이다. 끝낸 작업의 경위는 Worklog로 옮기고, 다음 작업을 제약하는 결과만 한 줄 남긴다. 매번 쓰지 않는 명령은 commands.md에, 확정값은 measurement.md에, 다시 나올 문제는 troubleshooting.md에, 엔진 소스에서 확인한 사실은 engine-notes.md에 둔다.
- **STATUS.md는 ASD-STE100(Simplified Technical English)의 작성 규칙에 맞춰 쓴다.** 그 규칙을 한국어 문장에 적용한다.
  - 한 문장에 한 가지 내용만 쓴다. 사실, 결정, 할 일을 한 문장에 섞지 않는다.
  - 절차 문장은 20어절, 서술 문장은 25어절을 넘기지 않는다. 한 문단은 여섯 문장까지다.
  - 누가 하는지 밝히고 능동형으로 쓴다. 할 일은 한 줄에 한 동작으로 쓰고, 순서가 있으면 번호를 붙인다.
  - 같은 것은 늘 같은 낱말로 부른다. 묶음, 조건, 단계, 지표의 이름은 measurement.md와 plan.md의 이름을 쓴다. 약어는 처음 나올 때 푼다.
  - 괄호 안에 문장을 넣지 않는다. 괄호에는 근거(커밋, 파일, ADR)와 짧은 값만 둔다.
  - 여러 값을 견줄 때는 표나 목록을 쓴다.
- **저장소 용량을 최소로 둔다.** 커밋하는 것은 소스, `Config/`, `.uproject`, 직접 만든 `Content/Lab/`, `Docs/`, `Posts/`, `Scripts/`, `Measurements/`의 `summary.csv`·`runs.csv`·대표 스크린샷이다. Fab 콘텐츠(`Content/ParagonMinions/`), VAT 베이크 산출물, `Saved/`, `Binaries/`, `Intermediate/`, `DerivedDataCache/`, 패키지는 커밋하지 않는다. 프레임 CSV·트레이스·영상은 저장소 밖에 두고 경로만 적는다. 이미지(`.png`, `.jpg`, `.webp`, `.gif`)와 `.uasset`·`.umap`은 Git LFS다(`.gitattributes`).
- **문서가 근거로 인용하는 스크린샷은 레포에 넣는다.** `Measurements/<묶음>/shots/`에 jpg(품질 90)로 복사해 커밋하고 문서에는 그 경로를 적는다. `Saved/` 경로는 git 밖이라 근거로 적지 않는다. 대량으로 찍은 프레임은 대표 프레임만 옮기고 고른 기준을 Worklog에 적은 뒤 지운다. 이미 커밋된 이미지는 형식을 바꾸지 않는다.

### 여러 세션

코디네이터 세션이 작업 세션을 띄우고, 모두 같은 체크아웃에서 일한다. 워크트리는 쓰지 않는다(`Binaries/`·`Intermediate/`·DDC를 세션마다 다시 만들게 된다).

모든 세션:

- 빌드·패키징은 한 번에 한 세션만 한다. 같은 `Binaries/`를 덮어쓴다.
- Test 패키지 벤치마크나 촬영이 도는 동안 다른 세션은 빌드·에디터·패키징을 하지 않는다. 측정이 오염된다.
- 파일 소유를 나눈다. 코드 세션은 `Source/`·`Config/`, 측정 세션은 `Scripts/`·`Measurements/`, 에셋 세션은 `Content/Lab/`·`Scripts/create-*.md`, 글 세션은 자기 `Posts/NN-이름/`이다. STATUS.md·plan.md·backlog.md는 코디네이터만 고친다.
- 커밋은 자기 경로만 `git add`한다. `git add -A`를 쓰지 않는다.
- 결과는 채팅이 아니라 파일에 남긴다(Worklog의 절, `Measurements/<묶음>/`).

작업 세션:

- 작업 단위 하나만 맡는다. 작업 단위는 빌드·작은 실행·Worklog 절·커밋으로 끝나는 크기다(기능 하나, 묶음 하나, 글 하나).
- 단계(조사, 구현, 빌드, 실행, 커밋)가 끝날 때마다 자기 Worklog 절에 한 줄을 더하고 같은 내용을 코디네이터에 메시지로 보낸다(단계 이름, 성패, 다음 단계). 답을 기다리지 않고 다음 단계로 간다. 메시지를 보낼 수 없으면 Worklog 한 줄과 턴 끝 보고만 한다.
- 막혔거나 설계가 갈리는 선택이 나오면 메시지로 묻고 멈춘다.
- 턴을 끝낼 때는 STATUS.md 대신 자기 Worklog 절을 갱신하고, 보고의 첫머리에 지금 단계와 성패를 적는다. 코디네이터에게는 보고의 앞부분만 전달된다.

코디네이터:

- 세션을 띄우면 STATUS.md "진행 중인 세션"에 세션, 작업 단위, 모델과 추론 수준, 상태를 적는다. 단계 메시지나 보고가 오면 그 줄을 고치고 사용자에게 한 줄로 알린다. 브리프에 위 작업 세션 규칙을 넣는다.
- 모델은 일에 맞춘다. 설계가 갈리는 코드·에셋·글은 Opus high, 정해진 절차를 따르는 일(측정 스크립트, 벤치마크 실행, 문서 맞추기)은 Sonnet medium이 기본이다.
- 다음 작업 단위는 새 세션에 Worklog를 근거로 넘긴다. 한 세션의 맥락이 과하게 차지 않게 한다.
- 작업 단위가 끝나면 결과(커밋, Worklog 절)를 확인한 뒤 세션을 보관 처리하고 "진행 중인 세션"에서 지운다.

### 결정

- **확정된 결정을 다시 열지 않는다.** [ADR 목록](Docs/Decisions/README.md)의 표와 승인된 ADR을 바꾸자고 제안하지 않는다. 예외는 결정의 근거가 사실과 다름을 엔진 소스(파일과 줄)나 실행 결과(라벨)로 확인한 경우 하나다. 이때는 대체 ADR을 "제안됨"으로 쓰고 사용자에게 알린다. 더 나은 방법이 있다는 생각은 채팅으로 알린다.
- **대안을 비교해 내린 결정은 ADR로 남긴다.** "제안됨"으로 초안을 쓰고 STATUS.md "사용자에게 요청한 일"에 승인을 요청한다. 승인된 ADR의 본문은 고치지 않고, 결정이 바뀌면 새 ADR로 대체한다. 단계의 순서와 실험 선택은 plan.md가 정하므로 ADR로 쓰지 않는다.
- **질문에는 답과 수정안만 낸다.** 사용자가 "\~하는 게 낫지 않아?"처럼 물으면 의견과 수정안(바꿀 파일과 문장)을 채팅에 보여 주고 편집과 커밋은 하지 않는다. "고쳐", "반영해" 같은 지시나 승인 뒤에 구현한다.
- **선택지를 낼 때는 추천안을 먼저 밝힌다.** 이유는 이 프로젝트의 사실(측정값, 엔진 소스, 확정된 결정, 작업량)에 기댄 한두 문장이고, 다른 선택지의 득실도 적는다. 한 번에 결정 하나를 묻고, 선택은 사용자가 한다.
- **설계가 갈리는 선택은 구현 전에 묻는다.** 사용자가 모르는 개념이 나오면 구현 전에 한 단락으로 설명한다.

### 구현

- **에디터 작업은 최대한 줄인다. 금지는 아니다.** 액터·프로세서·설정은 되도록 C++과 ini로 쓰고, 스포너·오브·카메라 프리셋·Mass EntityConfig는 실행 시 코드로 만든다([ADR-0005](Docs/Decisions/0005-entity-config-in-code.md)). 에셋이 더 맞는 일(Fab 에셋 임포트 확인, 피직스 에셋 수정, AnimToTexture 베이크, Niagara 시스템)은 에디터 작업으로 하고, 만든 순서를 `Scripts/create-*.md`에 적어 다시 만들 수 있게 한다.
- **에디터 작업은 Unreal MCP를 먼저 쓴다**(`ModelContextProtocol` + `AllToolsets` 플러그인). 코디네이터가 소스 빌드의 `UnrealEditor.exe`로 `HordeOptLab.uproject`를 띄운 뒤에 새 작업 세션을 연다. MCP는 세션이 시작할 때 연결되므로 에디터보다 먼저 시작한 세션에는 붙지 않는다. 다른 세션의 빌드·패키징·벤치마크가 끝난 뒤에 띄우고, 작업이 끝나면 닫는다. 툴 호출은 게임 스레드에서 직렬이라 겹쳐 부르지 않고, 대량 변경 전후에 저장한다. 새 `UFUNCTION`은 에디터 재시작이 필요하다. MCP가 막히면 Python Editor Script로 우회하고, 그것도 막히면 사용자에게 요청한다.
- **엔진 소스를 추측하지 않는다.** 기본값, 매크로, 클래스 이름은 소스 빌드의 `Engine/Source`·`Engine/Plugins`에서 확인한 뒤 쓰고, 확인한 것은 engine-notes.md에 적는다.
- **소스 빌드 엔진만 쓴다.** `.uproject`의 `EngineAssociation`이 가리키는 소스 빌드로만 열고 빌드한다. 런처 설치본과 섞으면 같은 `Binaries/`에 번갈아 빌드하게 된다.
- **군중 최적화와 무관한 기능을 만들지 않는다.** GAS, 무기·전투, 인벤토리, 메뉴 UI, 네트워크 복제, 세이브, 플레이어 이동, VFX·사운드 제작은 비목표다. 오브의 피해는 체력 프래그먼트 차감과 피격·사망 태그까지다. 새 기능 아이디어는 진행 중인 작업에 넣지 않고 채팅으로 알린다. 받아들여지면 plan.md에 넣는다.
- **구조와 이름.** `HordeOptLab.uproject`가 루트에 있고 모듈은 `HordeOptLab` 하나다. 클래스·인자·저장 폴더의 접두사는 `Lab`이다(`ALabOrb`, `ULabApproachProcessor`, `-LabN=`, `Saved/Screenshots/Lab/`). 폴더 이름은 대문자로 시작한다(`Docs/`, `Posts/`, `Scripts/`, `Measurements/`). `Scripts/`의 PowerShell은 UTF-8 BOM으로 저장한다.
- **참고 프로젝트는 읽기만 한다.** `../unreal-horde-references/`의 코드를 옮기면 출처 주석을 단다. Fab 에셋은 재배포 금지라 커밋하지 않는다. PARAGON 상표를 프로젝트 이름·제목에 쓰지 않는다.

### 문서와 글

- **Posts와 README는 한국어 한다체(-다)로 쓴다.** 틀, 문장, 그림 규칙은 [posting.md](Docs/Guides/posting.md)를 따른다. 엔진의 기법·클래스 이름은 번역하지 않는다(Mass, Representation LOD, ISM, VAT, ISKM, Animation Budget Allocator, URO). 한 일은 개념 이름이 아니라 실제로 바꾼 것으로 쓴다("Budget Allocator를 적용했다"가 아니라 "액터 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고 예산을 2ms로 두었다").
- **공개 문서에는 독자가 결과를 읽는 데 필요한 것만 쓴다.** 레포가 공개다. README와 글 본문에는 묶음 이름(`Full1`), 조건 라벨(`L1`), 마일스톤 이름, 다른 프로젝트의 이름, 작업 경위, 사족을 쓰지 않는다. "N일차", "N주 안에" 같은 일정 표현은 어느 문서에도 쓰지 않는다. 글 끝은 "한 문장으로" 절이다.
- **비교가 요점인 수치는 표가 아니라 SVG 그림으로 보인다.** 그림은 CSV에서 스크립트로 만든다. 글 본문에는 표를 쓰지 않고, 수치의 표는 측정 기록에 둔다([ADR-0014](Docs/Decisions/0014-post-format.md), [ADR-0015](Docs/Decisions/0015-readme-figures.md)).
- **모든 수치와 설정값에 근거를 적는다.** 측정값(조건 명시), 엔진 소스 확인값(파일과 심볼), 계산값(식) 중 하나다. 글 본문의 수치는 같은 폴더 `measurements.md`에 있어야 한다.
- **문서에서 `~`는 `\~`로 쓴다.** GFM이 한 문단의 `~` 두 개 사이를 취소선으로 그린다. 코드 스팬 안에서는 그대로 둔다.

## 완료 확인

작업 하나가 끝났다고 말하기 전에 확인한다.

1. 코드를 바꿨으면 Development 에디터 빌드가 성공한다(`Scripts/build.ps1`).
2. 작은 규모 실행이 종료 코드 0으로 끝난다(`Scripts/run-dev.ps1 -Small`). 공용 코드를 고쳤으면 이전 단계도 `-Stage`로 하나씩 돌린다.
3. 측정 묶음을 끝낼 때는 Test 패키지(`Scripts/package.ps1`)로 조건을 모두 돌려 `Measurements/<묶음>/summary.csv`와 `runs.csv`에 행이 남고, 결과를 Worklog에 적는다.
4. 문서만 바꿨으면 바꾼 문서의 상대 링크가 실제 파일을 가리킨다.

## 사람에게 넘기는 일

직접 하지 않고, 무엇을 해야 하는지 구체적으로 적어 사용자에게 요청한다.

- GitHub 푸시.
- ADR 승인, 글에 넣을 시각 자료의 최종 선택, 시나리오 고정값의 변경.
- Fab 에셋과 샘플 프로젝트 다운로드, `../unreal-horde-references/` 구성.
- 에디터에서만 가능한 작업 중 MCP와 Python Editor Script로 안 되는 것.
- 러너가 자동으로 찍지 않는 화면 녹화. 무엇을, 어느 실행에서, 몇 초, 어느 파일로 찍는지 적고 승인을 받는다. 승인은 그 목록에만 유효하다.

## 세션을 끝낼 때

1. 코디네이터는 STATUS.md를 갱신한다(끝낸 것, 다음 할 일, 막힌 것, 사용자에게 요청한 일). 작업 세션은 자기 Worklog 절을 갱신한다.
2. 새 결정이 생겼으면 ADR 초안을 썼는지, 엔진 소스에서 새로 확인한 사실을 engine-notes.md에 적었는지 확인한다.
3. 문서를 만들거나 옮겼으면 "문서 지도"를 고쳤는지 확인한다.
