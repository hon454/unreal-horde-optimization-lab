# 클론에서 측정까지

이 저장소를 클론한 PC에서 에셋을 갖추고, 빌드하고, 작은 실행으로 확인하고, Test 패키지로 재는 순서다. 명령은 저장소 루트에서 PowerShell로 친다. 스크립트 인자의 자세한 뜻은 [commands.md](commands.md), 실패했을 때는 [troubleshooting.md](troubleshooting.md)를 본다.

## 필요한 것

| 무엇 | 쓰는 곳 | 비고 |
| --- | --- | --- |
| Windows, Git, [Git LFS](https://git-lfs.com/) | 클론 | `.uasset`·`.umap`·이미지가 LFS다(`.gitattributes`) |
| UE 5.8.4 소스 빌드 | 빌드, 파생 에셋 만들기, 패키징 | Test 구성 패키지가 필요해 소스 빌드를 쓴다([ADR-0001](../Decisions/0001-test-package-for-measurement.md)) |
| Epic Games 런처의 UE 5.8(또는 5.7, 아래 "팩 받기") | 에셋 호스트 프로젝트 | 런처가 소스 빌드를 엔진으로 인식하지 못해 Fab 팩을 이 프로젝트에 바로 추가할 수 없다. 팩은 런처 엔진의 빈 프로젝트에 받고, 쓰는 것만 옮긴다 |
| Fab [Paragon: Minions](https://www.fab.com/listings/039ea035-9360-4e76-ad06-5d3a92da6f65) (Epic Games, 무료) | 미니언 메시·애니메이션 | 에셋 호스트에 받는다 |
| Fab [Niagara Examples Pack](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600) (Epic Games, 무료) | 오브의 번개·피해 범위·타워 | 에셋 호스트에 받는다 |
| Python 3(`py -3`) | 측정 요약과 그래프 | 표준 라이브러리만 쓴다 |

디스크(작성자 PC에서 잰 값): 에셋 호스트에 받은 팩이 Paragon: Minions 4.8GB, Niagara Examples Pack 1.2GB다. 이 프로젝트로 옮긴 뒤에는 `Content/ParagonMinions/` 292MB, `Content/NiagaraExamples/` 30MB이고, 만든 파생 에셋을 합친 `Content/Lab/`은 15MB다. 엔진 소스 빌드와 그 산출물은 따로 든다.

## 1. 클론

```bash
git lfs install
```

```bash
git clone <이 저장소 주소> unreal-horde-optimization-lab
```

성공하면 `Content/Lab/Maps/L_Bench.umap`이 수백 바이트의 LFS 포인터가 아니라 실제 맵 파일이다. 포인터로 남았으면 저장소 안에서 `git lfs pull`을 친다.

## 2. 엔진 등록

1. UE 5.8.4 소스를 받아 `Setup.bat`, `GenerateProjectFiles.bat`을 돌리고 `UnrealEditor` Win64 Development를 빌드한다.
2. 탐색기에서 `HordeOptLab.uproject`를 우클릭해 "Switch Unreal Engine version"으로 그 소스 빌드를 고른다. 소스 빌드가 `HKCU\Software\Epic Games\Unreal Engine\Builds`에 등록되고 `.uproject`의 `EngineAssociation`이 그 값으로 바뀐다.

스크립트는 모두 `Scripts/common.ps1`로 이 등록에서 엔진을 찾는다. 확인은 3절의 `restore-assets.ps1 -DryRun`이 첫 줄에 `OK  project engine: <엔진 폴더>`를 찍는 것으로 한다.

## 3. 팩 받기

1. 런처의 UE 5.8로 빈 블루프린트 프로젝트를 하나 만든다(이 문서에서는 "에셋 호스트"). C++ 프로젝트는 쓰지 않는다. 다른 엔진으로 컴파일된 C++ 모듈이 있으면 이전 커맨드렛이 로그 없이 종료 코드 1로 끝난다([troubleshooting.md](troubleshooting.md)의 AssetHost 줄).
2. 위 표의 두 Fab 페이지에서 각 팩을 라이브러리에 추가하고, 런처의 Fab 라이브러리에서 "프로젝트에 추가"로 에셋 호스트에 넣는다. 팩은 에셋 호스트의 `Content/ParagonMinions/`, `Content/NiagaraExamples/`에 놓인다. 스크립트는 이 두 폴더 이름을 찾는다.
3. Niagara Examples Pack의 Fab 페이지는 지원 버전을 5.7로 적는다. 런처가 5.8 프로젝트에 넣지 못하면 에셋 호스트를 런처의 UE 5.7로 만든다. Paragon: Minions는 5.0\~5.8을 지원해 같은 호스트에 들어간다. 스크립트는 호스트 `.uproject`의 버전으로 런처 엔진을 고르므로 따로 할 일이 없다.

에셋 호스트를 이 저장소 옆 `../unreal-horde-references/AssetHost/AssetHost.uproject`에 두면 스크립트가 인자 없이 찾는다. 다른 곳이면 4절에서 `-AssetHost`로 준다.

## 4. 에셋 복구

먼저 검사와 계획만 본다. 아무것도 실행하지 않는다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/restore-assets.ps1 -DryRun -AssetHost "D:/UE/LabAssetHost/LabAssetHost.uproject"
```

검사가 모두 `OK`이고 마지막 줄이 `DRY RUN: checks passed.`면 실제로 돌린다. `MISSING:` 줄이 있으면 그 줄이 무엇을 어디에 두라고 하는지 따른다. 에디터와 게임은 모두 닫는다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/restore-assets.ps1 -AssetHost "D:/UE/LabAssetHost/LabAssetHost.uproject"
```

스크립트는 아래 단계를 차례로 부른다. 단계마다 `OK   <단계> (<초>s)`를 찍고, 산출물이 이미 있는 단계는 `SKIP`으로 건너뛴다.

| 단계 | 스크립트(엔진) | 만드는 것 |
| --- | --- | --- |
| `migrate-minion`, `migrate-siege`, `migrate-orbfx` | `migrate-assets.py`(에셋 호스트의 런처 엔진) | 두 팩에서 쓰는 에셋과 그 의존성을 `Content/ParagonMinions/`, `Content/NiagaraExamples/`로 옮긴다 |
| `static-small`, `static-large` | `make-static-minion.py`(소스 빌드) | `Content/Lab/Derived/`의 정적 메시 |
| `vat-small`, `vat-large` | `bake-vat.py`(소스 빌드) | `Content/Lab/VAT/Baked/`의 VAT 텍스처·메시·머티리얼 |
| `iskm`, `iskm-verify` | `create-iskm.py`(소스 빌드) | `Content/Lab/ISKM/Baked/`의 ISKM 메시·애니 제공자·머티리얼, 그리고 새 프로세스에서 다시 읽어 확인 |

끝에서 `Scripts/restore-assets.manifest.txt`의 파일 목록과 실제 파일을 대조한다. 성공하면 `Manifest: 145 files, missing 0 (required 0)`과 `DONE: assets restored.`가 찍히고 종료 코드가 0이다. 게임이 경로로 읽는 필수 파일이 빠지면 `MISSING (required)`와 함께 종료 코드 1로 끝난다. 의존성 파일만 빠지면 `missing (dependency)`로 알리고 성공으로 끝난다. 팩 판이 바뀌면 의존성 파일의 이름과 위치가 달라질 수 있다.

- 로그는 `Saved/Logs/Restore/<단계>.log`다. 단계가 실패하면 그 로그의 `LAB…`·`Error:` 줄을 화면에 찍고 멈춘다. 고친 뒤 같은 명령을 다시 치면 끝난 단계는 건너뛴다.
- `-HostEngineRoot <런처 엔진 폴더>`는 호스트 엔진을 찾지 못한다고 할 때 준다.
- `-Force`는 파생 에셋을 다시 만든다. `Content/Lab/Derived/`, `Content/Lab/VAT/Baked/`, `Content/Lab/ISKM/Baked/`를 `Saved/RestoreBackup/<시각>/`로 옮긴 뒤 만든다. 팩 이전 단계는 다시 돌아도 이미 있는 에셋을 덮어쓰지 않는다.

AnimBP, Niagara 시스템, 맵 `L_Bench`, 바닥 머티리얼은 저장소에 커밋돼 있어 만들지 않는다.

## 5. 빌드와 작은 실행

```bash
powershell -ExecutionPolicy Bypass -File Scripts/build.ps1
```

UBT가 `HordeOptLabEditor Win64 Development`를 빌드하고 종료 코드 0으로 끝난다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-dev.ps1 -Small -Stage M5 -Variant q4final
```

미니언 100개로 1280x720 창을 20초 띄운다. 창이 닫힌 뒤 로그의 `lab_config`·`lab_assets`·`lab_summary` 줄과 `EXIT: 0`이 찍히면 에셋과 빌드가 맞다. 이 실행은 Development 빌드라 수치를 비교에 쓰지 않는다. 단계(`-Stage M0`\~`M5`)와 변형은 [commands.md](commands.md)에 있다.

## 6. 패키징과 측정

```bash
powershell -ExecutionPolicy Bypass -File Scripts/package.ps1
```

`Packaged/Test/Windows/`에 Test 패키지가 생기고 `PACKAGED: … commit=<해시>`와 `ELAPSED:`가 찍힌다. 작성자 PC에서 첫 패키징은 26분 걸렸다.

```bash
powershell -ExecutionPolicy Bypass -File Scripts/run-full.ps1 -DryRun
```

조건 수, 실행 순서, 예상 시간을 보여 준다. 실제 측정은 `-DryRun` 없이 돌리고(몇 시간), 측정 중에는 에디터·브라우저를 띄우지 않고 PC를 쓰지 않는다. 묶음 이름과 요약·그래프 명령은 [commands.md](commands.md) "총괄 묶음(Full1) 러너·검사·요약·촬영", 측정 규약은 [measurement.md](measurement.md)에 있다.
