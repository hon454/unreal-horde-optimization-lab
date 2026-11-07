# Unreal 호드 최적화 실험실

Night of the Dead에서 측정 없이 액터로 했던 좀비 호드 최적화를, UE5 Mass Entity로 다시 지어 거리별로 비용을 덜어내며 단계마다 수치로 확인하는 포트폴리오다. 액터로 멈춘 N에서 Mass로 닿은 N까지를 한 그래프로 보인다. 마일스톤 M0\~M5가 필수고, 그 뒤는 [backlog.md](Docs/backlog.md)의 순서다. 엔진 프로젝트는 `HordeOptLab`, 모듈과 클래스·인자·폴더의 접두사는 `Lab`이다.

## 세션을 시작하면

1. [Docs/STATUS.md](Docs/STATUS.md)를 읽는다. 현재 마일스톤, 다음 할 일, 매번 쓰는 명령, 막힌 것이 있다.
2. STATUS.md에 끝나고 커밋된 작업의 기록이 남아 있으면, 다음 작업 전에 정리안(옮길 항목, 옮길 파일, 남길 한 줄)을 채팅에 보여 주고 승인을 받아 정리하고 따로 커밋한다. 기준은 아래 "STATUS.md에는 지금 유효한 것만 둔다".
3. STATUS.md의 "다음 할 일" 첫 항목을 고른다. 비어 있으면 [plan.md](Docs/plan.md)의 다음 마일스톤을 사용자에게 확인한다.
4. 그 작업에 필요한 문서만 "문서 지도"에서 찾아 연다. 결정을 바꾸고 싶어지면 먼저 [ADR 목록](Docs/Decisions/README.md)을 본다.

## 문서 지도

| 알고 싶은 것 | 볼 곳 |
| --- | --- |
| 현재 마일스톤, 다음 할 일, 매번 쓰는 명령, 진행 중 묶음의 수치, 막힌 것, 사용자에게 요청한 일 | [Docs/STATUS.md](Docs/STATUS.md) |
| 마일스톤 M0\~M5의 목표, 바꾸는 변수, 실험, 완료 조건 | [Docs/plan.md](Docs/plan.md) |
| 미룬 주제(M6 이후, 잘라낸 실험), 글 대기열 | [Docs/backlog.md](Docs/backlog.md) |
| 확정된 결정과 이유, 버린 대안 | [Docs/Decisions/](Docs/Decisions/README.md)(ADR). ADR이 없는 초기 결정은 그 README의 표 |
| 측정 규약: 시나리오 고정값, 빌드 구성, 벤치마크 절차, 지표 이름, 차이 판정, 한계 | [measurement.md](Docs/Guides/measurement.md) |
| 빌드·패키징·벤치마크·그래프 명령과 인자, 측정 절차 | [commands.md](Docs/Guides/commands.md). 매번 쓰는 것만 STATUS.md "명령"에 |
| 빌드나 실행이 실패했을 때의 증상별 대처 | [troubleshooting.md](Docs/Guides/troubleshooting.md). 문제가 생겼을 때만 연다 |
| 글의 독자, 분량, 틀, 문장 규칙, 본문과 측정 기록의 구분 | [posting.md](Docs/Guides/posting.md) |
| 엔진 소스에서 확인한 사실(경로, 기본값, 매크로, 알려진 문제) | [engine-notes.md](Docs/Reference/engine-notes.md). 소스 위치가 필요하면 먼저 여기를 본다 |
| 영상·문서·샘플·에셋·포럼 스레드 | [references.md](Docs/Reference/references.md) |
| 측정 PC 사양과 엔진 버전 | [pc-specs.md](Docs/Reference/pc-specs.md) |
| 끝낸 마일스톤의 경위, 실패한 실행 | `Docs/Worklog/M<N>.md`. 세션 시작 때 읽지 않고 검색만 한다 |
| 글 본문과 이미지 | `Posts/NN-이름/README.md`, `Posts/NN-이름/images/` |
| 글 수치의 근거(실행 라벨, 세 실행의 값, 계산식, 캡처, 소스 위치) | `Posts/NN-이름/measurements.md` |
| 마일스톤별 측정 결과 | `Measurements/M<N>/summary.csv`(조건별 중앙값·범위), `runs.csv`(실행별 한 행), `shots/` |
| 공개 소개, 결과 한눈에 보기, 글 표 | [README.md](README.md) |
| 참고 프로젝트(City Sample, MetaHuman Crowd Sample, Fab 원본 팩) | 형제 디렉터리 `../unreal-horde-references/`(git 밖, 읽기 전용). 위치는 references.md |
| 실행 산출물(git 밖) | 프레임 CSV `Saved/Profiling/CSV/`, 트레이스 `Saved/Traces/`, 스크린샷 `Saved/Screenshots/Lab/`, 패키지 `Packaged/` |

새 문서를 만들거나 옮기면 이 표를 함께 고친다.

## 규칙

### 진행 방식

- **구현 트랙이 마일스톤을 끝낸다. 글과 학습은 뒤따른다.** 마일스톤의 완료 조건은 코드, 자동 측정(`Measurements/M<N>/`), git 태그 `M<N>`, STATUS 갱신, Worklog 한 절까지다. 글·그래프 해석·영상·트레이스 분석은 완료 조건이 아니다. 마일스톤을 끝내면 [backlog.md](Docs/backlog.md) "글 대기열"에 항목을 더하고 다음 마일스톤으로 간다. 글은 대기열에서 꺼내 그 태그를 체크아웃해 쓴다. 배울 개념은 막혔을 때 그 부분만 읽는다. 앞 프로젝트에서 글과 학습이 구현을 막았다(2026-11-06 사용자 결정).
- **한 마일스톤에서 바꾸는 최적화 변수는 하나다.** 시나리오(스폰 분포, 개체 종류와 비율, 오브 동작, 거리 밴드, 카메라 프리셋)는 M0에서 고정하고 바꾸지 않는다. 바뀐 변수가 둘 이상인 비교는 "참고"로 적고 그래프에 잇지 않는다. 시나리오를 바꿔야 하면 "새 기준선" 마일스톤으로 분리해 이전 단계를 모두 다시 잰다(M3b).
- **수치는 Test 패키지에서만 잰다.** Development 에디터 빌드(`-game`)는 개발, 동작 확인, 디버그 오버레이 스크린샷에 쓰고 그 수치는 비교에 쓰지 않는다([ADR-0001](Docs/Decisions/0001-test-package-for-measurement.md)).
- **측정 인프라는 M0 끝에 동결한다.** 스포너, 벤치마크 러너, 시드, 카메라 프리셋, CSV 스키마가 대상이다. 바꿔야 하면 그날 모든 단계를 다시 재고 그 사실을 적는다. 글의 수치는 그 시점 태그의 것으로 두고 고치지 않는다. 최종 비교표는 마지막에 최신 빌드 하나에서 전 단계를 한 날 연달아 잰다.
- **단계는 한 빌드 안의 실행 모드다.** 마일스톤의 구현을 지우지 않고 `-LabStage=`로 고르는 새 단계로 더한다. 이전 단계를 재려고 태그를 체크아웃하지 않는다([ADR-0004](Docs/Decisions/0004-stages-as-run-modes.md)).
- **개발 속도가 최우선이다.** 단위 테스트, CI, PR, 외부 이슈 트래커를 쓰지 않는다. main에 직접 커밋한다.
- **STATUS.md에는 지금 유효한 것만 두고 10KB를 넘기지 않는다.** 세션마다 통째로 읽는 파일이다. 끝낸 마일스톤의 경위는 `Docs/Worklog/M<N>.md`로 옮기고, 다음 작업을 제약하는 결과만 한 줄 남긴다. 매번 쓰지 않는 명령은 commands.md에, 확정값은 measurement.md에, 다시 나올 문제는 troubleshooting.md에, 엔진 소스에서 확인한 사실은 engine-notes.md에 둔다.
- **커밋 메시지는 [Conventional Commits](https://www.conventionalcommits.org/)로 쓴다.** `type(scope): summary` 한 줄, 영어 명령형 소문자. type은 `feat`, `fix`, `docs`, `build`, `refactor`, `chore`, `bench`(측정 결과 추가). 측정 조건이나 설정값을 바꾼 커밋은 본문에 이유를 쓴다.
- **저장소 용량을 최소로 둔다.** 커밋하는 것은 소스, `Config/`, `.uproject`, 직접 만든 `Content/Lab/`, `Docs/`, `Posts/`, `Scripts/`, `Measurements/`의 `summary.csv`·`runs.csv`·대표 스크린샷이다. Fab 콘텐츠(`Content/ParagonMinions/`), VAT 베이크 산출물, `Saved/`, `Binaries/`, `Intermediate/`, `DerivedDataCache/`, 패키지는 커밋하지 않는다. 프레임 CSV·트레이스·영상은 저장소 밖에 두고 경로나 링크만 적는다. 이미지(`.png`, `.webp`, `.gif`)와 `.uasset`·`.umap`은 Git LFS다(`.gitattributes`).

### 결정

- **확정된 결정을 다시 열지 않는다.** [ADR 목록](Docs/Decisions/README.md)의 표와 승인된 ADR을 바꾸자고 제안하지 않는다. 예외는 결정의 근거가 사실과 다름을 엔진 소스(파일과 줄)나 실행 결과(라벨)로 확인한 경우 하나다. 이때는 대체 ADR을 "제안됨"으로 쓰고 사용자에게 알린다. 더 나은 방법이 있다는 생각은 backlog.md에 적는다.
- **질문에는 답과 수정안만 낸다.** 사용자가 "\~하는 게 낫지 않아?"처럼 물으면 의견과 수정안(바꿀 파일과 문장)을 채팅에 보여 주고 편집과 커밋은 하지 않는다. "고쳐", "반영해" 같은 지시나 승인 뒤에 구현한다.
- **선택지를 낼 때는 추천안을 먼저 밝힌다.** 이유는 이 프로젝트의 사실(측정값, 엔진 소스, 확정된 결정, 작업량)에 기댄 한두 문장이고, 다른 선택지의 득실도 적는다. 선택은 사용자가 한다.
- **설계가 갈리는 선택은 구현 전에 묻는다.** 사용자가 모르는 개념이 나오면 구현 전에 한 단락으로 설명한다. 이 프로젝트의 절반은 학습이다.

### 구현

- **에디터 작업을 만들지 않는다. Unreal MCP는 예외 경로다.** 액터·프로세서·설정은 C++과 ini로 쓰고, 스포너·오브·카메라 프리셋은 실행 시 코드로 만든다. 에디터가 꼭 필요한 일(Fab 에셋 임포트 확인, 피직스 에셋 수정, AnimToTexture 베이크, EntityConfig 에셋)에만 Unreal MCP(`ModelContextProtocol` + `AllToolsets` 플러그인)를 쓴다. MCP 툴 호출은 게임 스레드에서 직렬이라 겹쳐 부르지 않고, 대량 변경 전후에 저장한다. 새 `UFUNCTION`은 에디터 재시작이 필요하다. MCP가 막히면 Python Editor Script로 우회하고, 그것도 막히면 사용자에게 에디터 작업을 요청한다. MCP 없이도 진행되는 설계를 우선한다.
- **엔진 소스를 추측하지 않는다.** 기본값, 매크로, 클래스 이름은 소스 빌드의 `Engine/Source`·`Engine/Plugins`에서 확인한 뒤 쓰고, 확인한 것은 engine-notes.md에 적는다. 이미 확인한 것이 engine-notes.md에 있다: Mass 코어 경로, `LODSignificance` 범위(0\~3), `mass.FullyParallel`, Test 구성 매크로, ISKM 트랙 풀 문제.
- **소스 빌드 엔진만 쓴다.** `.uproject`의 `EngineAssociation`이 가리키는 소스 빌드로만 열고 빌드한다. 런처 설치본과 섞으면 같은 `Binaries/`에 번갈아 빌드하게 된다.
- **군중 최적화와 무관한 기능을 만들지 않는다.** GAS, 무기·전투, 인벤토리, 메뉴 UI, 네트워크 복제, 세이브, 플레이어 이동, VFX·사운드 제작은 비목표다. 오브의 피해는 체력 프래그먼트 차감과 피격·사망 태그까지다. 오브와 저격 궤적은 구체 하나와 디버그 라인으로 그린다. 새 기능 아이디어는 backlog.md에만 적는다.
- **구조와 이름.** `HordeOptLab.uproject`가 루트에 있고 모듈은 `HordeOptLab` 하나다. 클래스·인자·저장 폴더의 접두사는 `Lab`이다(`ALabOrb`, `ULabApproachProcessor`, `-LabN=`, `Saved/Screenshots/Lab/`). 폴더 이름은 대문자로 시작한다(`Docs/`, `Posts/`, `Scripts/`, `Measurements/`). `Scripts/`의 PowerShell은 UTF-8 BOM으로 저장한다.
- **참고 프로젝트는 읽기만 한다.** `../unreal-horde-references/`의 코드를 옮기면 출처 주석을 단다. Fab 에셋은 재배포 금지라 커밋하지 않는다. PARAGON 상표를 프로젝트 이름·제목에 쓰지 않는다.

### 문서와 글

- **문서에 일정 표현을 쓰지 않는다.** "N일차", "N주 안에"를 README, Posts, Docs 어디에도 쓰지 않는다. 단계는 마일스톤 번호와 태그로 부른다. 레포가 공개다.
- **공개 문서에 면접 대비 의도를 쓰지 않는다.** "면접용", 면접 질문 번호, 회사 이름은 Posts·README·Docs에 쓰지 않는다. 글 끝은 "한 문장으로" 섹션이다. 그런 메모는 사용자의 볼트에 둔다.
- **Posts와 README는 한국어 한다체(-다)로 쓴다.** 틀과 문장 규칙은 [posting.md](Docs/Guides/posting.md)를 따른다. 엔진의 기법·클래스 이름은 번역하지 않는다(Mass, Representation LOD, ISM, VAT, ISKM, Animation Budget Allocator, URO). 한 일은 개념 이름이 아니라 실제로 바꾼 것으로 쓴다("Budget Allocator를 적용했다"가 아니라 "액터 메시를 `USkeletalMeshComponentBudgeted`로 바꾸고 예산을 2ms로 두었다").
- **모든 수치와 설정값에 근거를 적는다.** 측정값(조건 명시), 엔진 소스 확인값(파일과 심볼), 계산값(식) 중 하나다. 글 본문의 수치는 같은 폴더 `measurements.md`의 표에 있어야 한다.
- **문서에서 `~`는 `\~`로 쓴다.** GFM이 한 문단의 `~` 두 개 사이를 취소선으로 그린다. 코드 스팬 안에서는 그대로 둔다.

## 결정 기록(ADR)

- 대안을 비교해 내린 결정이 새로 생기면 [Docs/Decisions/](Docs/Decisions/README.md)에 "제안됨"으로 초안을 쓰고 STATUS.md "사용자에게 요청한 일"에 승인을 요청한다.
- 승인된 ADR의 본문은 고치지 않는다. 결정이 바뀌면 새 ADR로 대체한다.
- 마일스톤의 순서와 실험 선택은 plan.md가 정하므로 ADR로 쓰지 않는다.

## 완료 확인

작업 하나가 끝났다고 말하기 전에 확인한다.

1. Development 에디터 빌드가 성공한다(`Scripts/build.ps1`).
2. 작은 규모 실행이 종료 코드 0으로 끝난다(`Scripts/run-dev.ps1 -Small`). 공용 코드를 고쳤으면 이전 단계도 `-Stage`로 하나씩 돌린다.
3. 마일스톤을 끝낼 때는 Test 패키지(`Scripts/package.ps1`)로 측정 조건을 모두 돌려 `Measurements/M<N>/summary.csv`와 `runs.csv`에 행이 남고, 이전 마일스톤과 비교한 결과를 STATUS.md에 적고, 태그 `M<N>`을 붙인다.

문서만 바꾼 작업은 바꾼 문서의 상대 링크가 실제 파일을 가리키는지 확인한다. 실패한 실행의 수치는 쓰지 않는다. 같은 라벨은 다시 쓰지 않는다.

## 사람에게 넘기는 일

직접 하지 않고, 무엇을 해야 하는지 구체적으로 적어 사용자에게 요청한다.

- Fab 에셋과 샘플 프로젝트 다운로드, `../unreal-horde-references/` 구성.
- 에디터에서만 가능한 작업 중 MCP와 Python Editor Script로 안 되는 것.
- 화면 녹화(무엇을, 어느 실행에서, 몇 초, 어느 파일로 찍는지 적고 승인을 받는다. 승인은 그 목록에만 유효하다).
- ADR 승인. 글에 넣을 시각 자료의 최종 선택. 거리 밴드·시나리오 고정값의 확정(M0에서 표로 보고하고 사용자가 확정한다).
- GitHub 푸시.

## 세션을 끝낼 때

1. STATUS.md를 갱신한다: 끝낸 것, 다음 할 일, 막힌 것, 사용자에게 요청한 일.
2. 새 결정이 생겼으면 ADR 초안을 썼는지, 엔진 소스에서 새로 확인한 사실을 engine-notes.md에 적었는지 확인한다.
3. 문서를 만들거나 옮겼으면 "문서 지도"를 고쳤는지 확인한다.
