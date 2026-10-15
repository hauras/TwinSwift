# TwinSwift 전투 HUD

- 에셋: `/Game/UI/WBP_TwinHUD`. `BP_PlayerController.HUDWidgetClass`에 연결.
- 하단 중앙: 현재 캐릭터 카드와 체력, 기본공격 / 스킬 1 / 스킬 2 / 회피.
- 오른쪽 작은 카드: 대기 캐릭터 체력, 상대 스킬 입력 두 개, 교체 어시스트 설명.
- 교체하면 현재 캐릭터의 이름·아이콘·설명·실제 키 표시가 바뀜.
- 키는 IMC_Default와 DA_AbilityInputConfig에서 읽음. 현재 키 1·2 / 3·4를 임의 변경하지 않음.
- 체력은 두 ASC의 AttributeChanged 알림으로 갱신.
- 스킬은 0.1초 타이머로 지급 여부와 실제 GAS 쿨타임을 조회. 쿨타임 GE가 없는 스킬의 숫자를 가짜로 세지 않음.
- 미지급 능력은 미구현으로 흐리게 표시. 표의 효과·배율·거리 등은 툴팁에 반영.
- 전사 기본공격은 기존 2타 구성을 유지. 실제 미구현 스킬과 어시스트 전투 로직을 추가한 작업은 아님.
- 아이콘: `Content/UI/Icons`, 원본: `Tools/Art/T_UI_SkillAtlas.png`. 숨겨진 참조를 통해 마법사 아이콘도 패키징에 포함.
- 재생성 스크립트: `Tools/setup_twin_hud.py`. WBP와 컨트롤러 최초 상태는 `Content/Backups/HUD_BeforeSkillLayout`에 보관.

## 적용과 확인

C++ 수정본은 TwinSwiftEditor / Development Editor / Win64 빌드가 필요하다.
CLAUDE.md의 '빌드는 사용자가 한다'에 따라 에이전트는 빌드를 실행하지 않았다.
빌드 후 실행하면 태그 교체에 따라 카드 역할과 스킬 칸이 바뀌어야 한다.
현재 환경은 GUI 에디터가 페이징 파일 부족으로 종료되어 명령줄 에디터로 에셋을 생성했다.
실제 화면과 신규 C++ 실행 검증은 빌드 후 남아 있다.
