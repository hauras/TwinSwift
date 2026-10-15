# TwinSwift — UE5 GAS 태그 액션 보스 러시

## 한 줄
전사·마법사 두 캐릭터를 실시간으로 바꿔 가며(태그) 싸우는 3D 액션 보스 러시. 모든 보스가 "교체를 잘해야 이기게" 설계됨.

- 엔진: Unreal Engine 5.7 · C++ · GAS
- 저장소: https://github.com/hauras/TwinSwift (에셋은 Git LFS)
- 1차 프로젝트: `D:\Unreal Projects\SProject` (https://github.com/hauras/S_Project). 1차를 측정해 한계를 찾고 재설계한 2차 프로젝트
- 지원 목표: 언리얼 클라이언트 프로그래머. 4~5주 안에 완성 후 지원

## 지금 할 일
`Docs/계획.md`를 먼저 읽을 것. 컨셉, 단계 순서, 미정 사항, 1차에서 가져올 것이 정리돼 있음.

## 진행 방식
- **한 번에 옮기지 않음.** 단계 단위로 "옮기거나 작성 → 빌드 → 실행해서 확인 → 다음". 단계마다 게임이 실행되는 상태 유지
- 1차 코드는 `D:\Unreal Projects\SProject\Source\SProject`에서 읽어 옴. 2026-10-12 세션에서 고친 버그가 반영된 상태 (1차 폴더는 지우지 말 것)
- **C++ 클래스 이름은 1차와 같게 유지** (`USGameplayAbility`, `ASPlayerController` 등). 1차 에셋을 Migrate하면 `CoreRedirects`(`/Script/SProject` → `/Script/TwinSwift`)로 부모 클래스를 찾음
- 에디터 작업: Python 원격 실행이 켜져 있음 (`DefaultEngine.ini`의 `bRemoteExecution`). 엔진의 `remote_execution.py`로 실행 중인 에디터에 접속해 에셋을 읽거나 고칠 수 있음. **에셋 수정은 무엇을 바꿀지 먼저 보여 주고 확인받은 뒤**

## 1차에서 배운 것 (같은 실수 하지 않기)
- 서버/클라 경계: 확률·피해·상호작용은 서버에서. 상호작용은 Server RPC로
- 몽타주 능력은 `OnBlendOut`에서 종료, 취소·실패 경로마다 `EndAbility`
- 시전 시 `StopMovementImmediately` 금지 (뚝 끊김). 입력만 막고 자연 감속
- 전신 모션을 상체 슬롯으로만 재생하지 않기 (서 있으면 전신, 움직이면 상체)
- 초기화(`InitAbilityActorInfo`)가 두 번 실행되지 않게. Loose 태그가 쌓이지 않게
- 무거운 에셋(Paragon 8K 텍스처) 사용 안 함. 믹사모 사용
- 팀 판정은 함수 하나로 (`IsFriend`)

## 확인 방법
- 혼자: PIE Net Mode `Play Standalone`
- 멀티 검증(마지막 단계): `Play As Listen Server`, 플레이어 2명, 콘솔 `Net PktLag=500`
- 성능: `stat unit`, Unreal Insights (`-trace=cpu,frame,gpu`)
- 애니메이션: Rewind Debugger

## 작업 규칙 (사용자 선호)
- 대화·주석·문서는 한국어. 쉬운 말로
- **빌드는 사용자가 한다.** Claude는 무엇을 빌드·실행할지 안내
- 코드는 읽기 쉽게: `UPROPERTY`와 선언은 줄을 나눔, 주석은 코드 위에, 긴 인자는 한 줄에 하나
- 고칠 곳을 말할 때는 `파일:줄`과 현재 코드 한 줄을 같이 적음
- 커밋하라고 하면 **푸시까지**. 메시지 `[add] …` / `[fix] …` 한국어, **Co-Authored-By 넣지 않음**
- README에 과장 표현("완벽히", "원천 차단", "100%") 쓰지 않음. 숫자로 말하기
- 개선한 것은 "문제 → 원인 → 해결 → 확인"으로 기록 (README 재료)
