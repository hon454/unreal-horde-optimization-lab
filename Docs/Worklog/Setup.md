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
