# 클론 준비와 에셋 복구

깨끗한 클론에서 Fab 두 팩만 받으면 스크립트 하나로 git 밖 에셋이 복구되게 하는 작업의 기록이다. 가이드는 [Guides/setup.md](../Guides/setup.md).

## 에셋 복구 스크립트와 가이드(2026-11-12)

- 조사 끝(성공). 복구 대상과 만드는 스크립트:
  - `Content/ParagonMinions/`(61개), `Content/NiagaraExamples/`(47개): `migrate-assets.py` 세트 `minion`·`siege`·`orbfx`. 시드 15개의 하드·소프트 의존성을 따라 옮기고 대상에 있는 패키지는 건너뛴다. 호스트 엔진의 커맨드렛으로 돈다.
  - `Content/Lab/Derived/`(3): `make-static-minion.py` `LAB_UNIT=small`(`SM_LabMinion`), `large`(`SM_LabMinionLarge`, `M_LabMinionLargeStatic`).
  - `Content/Lab/VAT/Baked/`(28): `bake-vat.py` 소형·대형(종류마다 액터 머티리얼 1, Bone·Vertex 모드마다 DA·M·MI·SM 4, 텍스처 5).
  - `Content/Lab/ISKM/Baked/`(6): `create-iskm.py`(`LAB_UNIT` 비움 = 두 종류). `LAB_ISKM_VERIFY=1`로 새 프로세스에서 확인.
  - 순서: 이전 → 정적 메시 → VAT → ISKM(각 헤더의 "미니언 이전 뒤", "bake-vat.py 뒤"). 커밋된 `MI_LabCrowdSmall`·`MI_LabCrowdLarge`·`NS_LabCrowd`가 VAT 산출물을 경로로 참조한다(`create-crowd-fx.md`).
  - 에디터 수작업으로만 만든 git 밖 에셋은 없다. MCP 절차(`create-anim-bp-large.md`, `bind-anim-start.md`, `create-crowd-fx.md`, `create-bolt-fx.md`)와 `create-anim-bp.py`·`create-band-floor.py`·`create-bench-map.py`의 산출물은 모두 커밋된 8개 파일 안에 있다.
  - 팩 위치: 런처가 소스 빌드를 엔진으로 인식하지 못해 Fab 팩을 HordeOptLab에 직접 추가할 수 없다(references.md). 팩은 런처 엔진 프로젝트(에셋 호스트)에 "프로젝트에 추가"로 받고, 스크립트는 호스트의 `Content/ParagonMinions/`·`Content/NiagaraExamples/`를 본다. 작성자 PC의 `../unreal-horde-references/AssetHost/Content/`에 이 두 폴더 이름으로 있다(ParagonMinions 4.8GB, NiagaraExamples 1.2GB). 팩 통째로 HordeOptLab에 넣으면 `DefaultGame.ini`의 `DirectoriesToAlwaysCook`이 두 폴더 전체를 쿡하므로 이전이 필요하다.
  - 커맨드렛 종료 코드: `PythonScriptCommandlet.cpp`가 스크립트 예외면 -1, 아니면 0. `create-iskm.py`는 예외를 잡아 `LABISKM result failed=...`로만 알리고, `make-static-minion.py`는 실패를 로그로만 남긴다. 그래서 스크립트는 종료 코드와 산출물 파일, ISKM은 로그 줄까지 본다.
- 스크립트 끝(성공). `Scripts/restore-assets.ps1`(UTF-8 BOM)과 대조 목록 `Scripts/restore-assets.manifest.txt`(작성자 PC의 git 밖 파일 145개). 호스트 엔진은 호스트 `.uproject`의 `EngineAssociation`으로 찾는다(버전 문자열이면 `LauncherInstalled.dat`, GUID면 HKCU 소스 빌드 등록). 작성자 PC의 AssetHost는 GUID `{31E80386…}`가 등록돼 있지 않아 `-HostEngineRoot "G:\Epic Games\UE_5.8"`이 필요하다. `-Force`는 세 산출물 폴더를 `Saved/RestoreBackup/<시각>/`로 옮긴 뒤 다시 만든다(이전 단계는 덮어쓰지 않는다). 기존 스크립트는 바꾸지 않았다.
- 건식 확인 끝(성공, 에디터 없이). 이 체크아웃의 `-DryRun`: 아홉 단계 모두 SKIP, 종료 코드 0. `-DryRun -Force`: 아홉 단계 모두 RUN, 호스트 엔진 GUID 미등록을 MISSING으로 보고. 스크래치 패드의 가짜 클론(`.uproject`와 스크립트만, `Content/` 비움): `-HostEngineRoot`를 주면 검사 통과·아홉 단계 RUN, 안 주면 호스트 없음 MISSING. 같은 가짜 클론의 실제 실행은 검사 단계에서 `FAIL: 2 check(s) failed. Nothing was changed.`, 종료 코드 1, 아무것도 만들지 않음. 실제 복구 실행은 하지 않았다(에디터를 띄우지 않는 범위).
- 검증 문의(대기). 코디네이터에 실제 복구 실행의 검증 방법(세 산출물 폴더를 `-Force`로 백업 폴더에 옮기고 다시 만든 뒤 파일 목록·크기 비교, `build.ps1`과 `run-dev.ps1 -Small -Stage M5 -Variant q4final` 종료 코드 0)과 에디터를 띄워도 되는 시점을 물었다. 팩 이전 단계는 이 PC에서 이미 있는 것을 건너뛰므로 팩 폴더까지 옮겨야 검증된다(따로 승인 필요).
- 링크 확인 끝(성공). 내장 브라우저로 열어 제목과 게시자를 확인했다(로그인 없이 읽히는 상품 페이지만).
  - Paragon: Minions, Epic Games, https://www.fab.com/listings/039ea035-9360-4e76-ad06-5d3a92da6f65 (references.md의 주소와 같다).
  - Niagara Examples Pack, Epic Games, https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600. 페이지의 언리얼 엔진 버전은 5.7만 적혀 있다(2026-01-29 게시, 스탠다드 라이선스, 무료). 작성자 PC에서 이 팩을 5.8 AssetHost에 어떻게 넣었는지는 기록이 없다. 가이드에는 런처가 5.8 프로젝트에 넣지 못하면 호스트를 5.7로 만들라고 적었다(스크립트는 호스트 `.uproject`의 버전으로 런처 엔진을 찾는다). 5.7 호스트로 이전한 적은 없다.
- 가이드 끝(성공). `Docs/Guides/setup.md`(필요한 것, 클론, 엔진 등록, 팩 받기, 에셋 복구, 빌드와 작은 실행, 패키징과 측정). 디스크는 작성자 PC에서 잰 값만, 시간은 첫 패키징 26분(Worklog/M0.md)만 적었다. 복구 단계의 소요 시간은 실제로 돌린 적이 없어 적지 않았다. `commands.md` "처음 준비"는 가이드를 가리키는 한 줄로 줄였다.
- 코디네이터용: Releases에서 패키지를 받아 실행만 하는 길은 가이드에 아직 넣지 않았다. 릴리스를 올린 뒤 `setup.md` 제목 아래 첫 문단 다음에 한 줄("빌드 없이 실행만 하려면 GitHub Releases의 Test 패키지 zip을 받아 …")과 링크를 넣는다.
- README·스크립트 목록 끝(성공). README "실행" 절의 전제 2·3번과 첫 문장의 링크만 고쳤다(두 팩과 확인된 링크, `restore-assets.ps1`, `setup.md`). `Scripts/README.md` "에셋을 다시 만드는 것" 맨 위에 한 줄.
- 남은 일: 작업 중 다른 세션이 `bake-vat.py`에 입자 전용 저폴리 굽기(`LAB_VAT_LOD=N`, 산출물 `*_Lod<N>`)를 더하고 있다(커밋 전). `NS_LabCrowd`가 그 산출물을 쓰게 되면 `restore-assets.ps1`에 `vat-lod-small`·`vat-lod-large` 단계(LOD0 굽기 뒤)와 대조 목록 줄을 더해야 한다.
- 굽기 표 정리 끝(성공, 코디네이터 지시). `restore-assets.ps1`의 프로젝트 엔진 단계(정적 메시, VAT, ISKM, ISKM 확인)를 표 `$BakeSteps` 한 곳에 모았다. 입자 전용 저폴리 굽기는 `vat-small`·`vat-large` 뒤에 한 줄씩 더하고 산출물을 대조 목록에 더하면 된다. 이 체크아웃과 가짜 클론의 `-DryRun` 결과는 바꾸기 전과 같다(9단계 SKIP / 9단계 RUN, 종료 코드 0). 실제 복구 실행은 검증하지 않았다. 입자 메시 작업(`crowdfx-lowpoly`)이 끝난 뒤 코디네이터가 알리면 한다. 팩 폴더까지 옮기는 검증은 사용자 OK를 받은 뒤에 한다.

### 팩까지 포함한 복구 검증 절차(준비만, 실행 전)

사용자 결정(2026-11-12, 코디네이터 전달): "A. 팩도 검증". `Content/ParagonMinions`, `Content/NiagaraExamples`까지 옮기고(삭제 금지) 팩 이전부터 끝까지 돌린다. 시작은 `crowdfx-lowpoly`가 끝나고 코디네이터가 에디터를 닫은 뒤 "시작"을 알릴 때다. 입자 LOD 굽기 단계는 그 세션의 커밋 뒤에 `$BakeSteps`와 대조 목록에 더하고 나서 검증한다.

- 호스트 확인(읽기만, 성공). `../unreal-horde-references/AssetHost/AssetHost.uproject`에 두 팩이 있다(시드 15개 모두 있음, `-DryRun` 검사 통과). `EngineAssociation` GUID `{31E80386…}`는 레지스트리에 없어 `-HostEngineRoot "G:\Epic Games\UE_5.8"`을 준다. 호스트의 C++ 모듈은 런처 엔진으로 컴파일돼 있다(`Binaries/Win64/UnrealEditor.modules` BuildId `55116800` = 런처 `UE_5.8`의 BuildId). 사용자가 따로 받을 것은 없다.
- 지금 크기(기준): `Content/ParagonMinions` 292MB(61파일), `Content/NiagaraExamples` 30MB(47파일), `Content/Lab` 16MB. git 밖 파일 145개는 `Scripts/restore-assets.manifest.txt`.

절차(`<시각>`은 한 번 정해 모든 줄에 같이 쓴다):

1. 전제: `UnrealEditor`·`UnrealEditor-Cmd`·`HordeOptLab` 프로세스 없음, `git status`에 `Content/` 변경 없음, `Content/LabTmp/` 같은 다른 세션의 임시 폴더가 남아 있으면 코디네이터에 묻는다.
2. 기준 목록: `Content/ParagonMinions`, `Content/NiagaraExamples`, `Content/Lab/{Derived,VAT/Baked,ISKM/Baked}`의 파일 경로·크기를 스크래치 패드 `before.csv`로 남긴다.
3. 팩 폴더 옮기기(삭제 금지): `Content/ParagonMinions` → `Saved/RestoreBackup/<시각>/ParagonMinions`, `Content/NiagaraExamples` → `Saved/RestoreBackup/<시각>/NiagaraExamples`(`Move-Item`, 같은 드라이브라 이동만 한다).
4. 건식: `restore-assets.ps1 -DryRun -Force -HostEngineRoot "G:\Epic Games\UE_5.8"`. 모든 단계가 RUN이고 검사가 통과하는지 본다.
5. 실행: `restore-assets.ps1 -Force -HostEngineRoot "G:\Epic Games\UE_5.8"`. 스크립트가 `Lab/Derived`, `Lab/VAT/Baked`, `Lab/ISKM/Baked`를 `Saved/RestoreBackup/<스크립트 시각>/`으로 옮기고 이전·굽기를 돌린다. 단계별 초를 Worklog에 적는다(가이드의 소요 시간 근거).
6. 비교: 새 파일의 `after.csv`를 만들어 `before.csv`와 경로·크기를 맞춘다. 경로가 빠지거나 더해진 것, 크기가 다른 것을 적는다(굽기 산출물은 저장 시점의 GUID 등으로 크기가 조금 다를 수 있어 차이만 기록하고 판단은 다음 단계 실행 결과로 한다). 끝줄 `Manifest: 145 files, missing 0 (required 0)`을 확인한다.
7. 실행 확인: `build.ps1`(코드가 바뀌지 않았으면 빠르게 끝난다), `run-dev.ps1 -Small -Stage M5 -Variant q4final`이 `EXIT: 0`이고 로그 `lab_assets`의 값이 모두 1인지. 공용 경로이므로 `-Stage M2 -Variant m2mass`(VAT ISM)와 `-Stage M1 -Variant m1mass`(정적 메시)도 `-Small`로 돌린다. 스크린샷 한 장(`-ShotAt`)으로 미니언이 기본 머티리얼(회색 격자)로 그려지지 않는지 본다.
8. 결과에 따라:
   - 성공: 백업(`Saved/RestoreBackup/`)은 코디네이터에 알린 뒤 사용자 결정으로 지운다(스스로 지우지 않는다). 가이드에 소요 시간을 넣는다.
   - 실패: 아래 되돌리기.

되돌리기(복구가 실패하거나 실행 확인이 깨졌을 때):

1. 프로세스가 모두 꺼졌는지 본다.
2. 새로 만든 것을 지우지 않고 옆으로 옮긴다: `Content/ParagonMinions`, `Content/NiagaraExamples`, `Content/Lab/Derived`, `Content/Lab/VAT/Baked`, `Content/Lab/ISKM/Baked`(있는 것만) → `Saved/RestoreBackup/<시각>-failed/` 같은 상대 경로.
3. 원본을 되돌린다: `Saved/RestoreBackup/<시각>/ParagonMinions`·`NiagaraExamples` → `Content/`, `Saved/RestoreBackup/<스크립트 시각>/Lab/...` → `Content/Lab/...`.
4. `before.csv`와 다시 맞춰 경로·크기가 같은지 보고, `run-dev.ps1 -Small -Stage M5 -Variant q4final`이 `EXIT: 0`인지 본다.
5. 실패 로그(`Saved/Logs/Restore/<단계>.log`)의 원인을 이 절에 적고 코디네이터에 알린다.

### 팩까지 포함한 복구 검증(2026-11-12)

- 입자 LOD4 단계 끝(성공). 코디네이터의 "시작"(입자 메시 커밋 77b597b·bf1a4c8·e21eed4·7f6d128 뒤)에 따라 `$BakeSteps`에 `vat-lod4-small`·`vat-lod4-large`(`LAB_VAT_LOD=4`, LOD0 굽기 뒤)를 더하고 대조 목록에 `*_Bone_Lod4` 12개를 더했다(157개). 커밋된 `NS_LabCrowd`·`MI_LabCrowd*`가 참조하는 것은 LOD4뿐이다(바이너리 문자열 검사). 로컬의 `*_Bone_Lod3` 12개는 비교 때 남은 것이라 복구 대상이 아니다. `-DryRun -Force` 11단계 RUN, 검사 통과.
