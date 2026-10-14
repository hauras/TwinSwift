#include "SGameplayTags.h"

namespace SGameplayTags
{
	// 입력
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_LMB, "InputTag.LMB", "좌클릭: 기본 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Q, "InputTag.Q", "전사 스킬 1: 돌진 베기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_W, "InputTag.W", "전사 스킬 2: 내려찍기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_E, "InputTag.E", "마법사 스킬 1 (전사일 때 누르면 교체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_R, "InputTag.R", "마법사 스킬 2 (전사일 때 누르면 교체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Space, "InputTag.Space", "회피");

	// 능력
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_Combo, "Ability.Warrior.Combo", "전사 3타 콤보");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_DashSlash, "Ability.Warrior.DashSlash", "전사 돌진 베기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_GroundSlam, "Ability.Warrior.GroundSlam", "전사 내려찍기 (균열 표식)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_SpinAssist, "Ability.Warrior.SpinAssist", "전사 교체 어시스트: 회전 베기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "회피 (전사 구르기, 마법사 순간이동)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Swap, "Ability.Swap", "캐릭터 교체");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact, "Ability.HitReact", "피격 반응");

	// 몽타주 이벤트
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_Hit, "Event.Montage.Hit", "타격 판정 프레임");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_ComboWindow, "Event.Montage.ComboWindow", "다음 콤보 입력을 받는 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Montage_CancelWindow, "Event.Montage.CancelWindow", "스킬·회피로 끊을 수 있는 구간");

	// 표식
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Crack, "Mark.Crack", "균열: 전사가 걸고 마법사가 터뜨림");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Frost, "Mark.Frost", "빙결: 마법사가 걸고 전사가 터뜨림");

	// 상태
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "공격 중 (이동 입력 막기)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invincible, "State.Invincible", "무적 (회피, 타이밍 교체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stun, "State.Stun", "경직·기절");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망");

	// 캐릭터
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Warrior, "Character.Warrior", "지금 전사를 조작 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Mage, "Character.Mage", "지금 마법사를 조작 중");

	// 쿨다운
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Warrior_DashSlash, "Cooldown.Warrior.DashSlash", "돌진 베기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Warrior_GroundSlam, "Cooldown.Warrior.GroundSlam", "내려찍기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dodge, "Cooldown.Dodge", "회피 쿨다운");

	// 데미지
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DamageMultiplier, "SetByCaller.DamageMultiplier", "스킬 데미지 배율 (공격력 × 배율)");
}
