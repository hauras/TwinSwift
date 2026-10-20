#include "SGameplayTags.h"

namespace SGameplayTags
{
	// 입력
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "기본 공격 (지금 키: 좌클릭)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Warrior_Skill1, "InputTag.Warrior.Skill1", "전사 돌진 베기 (지금 키: 1)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Warrior_Skill2, "InputTag.Warrior.Skill2", "전사 내려찍기 (지금 키: 2)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Mage_Skill1, "InputTag.Mage.Skill1", "마법사 냉기탄 (지금 키: 3, 전사일 때 누르면 교체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Mage_Skill2, "InputTag.Mage.Skill2", "마법사 낙뢰 (지금 키: 4, 전사일 때 누르면 교체)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge, "InputTag.Dodge", "회피 (지금 키: Space)");

	// 능력
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_Combo, "Ability.Warrior.Combo", "전사 3타 콤보");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_DashSlash, "Ability.Warrior.DashSlash", "전사 돌진 베기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_GroundSlam, "Ability.Warrior.GroundSlam", "전사 내려찍기 (균열 표식)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior_SpinAssist, "Ability.Warrior.SpinAssist", "전사 교체 어시스트: 회전 베기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Mage_FireBolt, "Ability.Mage.FireBolt", "마법사 기본 공격: 화염탄");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Mage_FrostBolt, "Ability.Mage.FrostBolt", "마법사 냉기탄 (빙결 표식)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Mage_Thunder, "Ability.Mage.Thunder", "마법사 낙뢰 (균열 터뜨림)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge, "Ability.Dodge", "회피 (전사 구르기, 마법사 순간이동)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Swap, "Ability.Swap", "캐릭터 교체");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact, "Ability.HitReact", "피격 반응");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss, "Ability.Boss", "보스 공격 전체 (취소할 때 부모 태그로)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Golem_Slam, "Ability.Boss.Golem.Slam", "골렘 내려치기 (느림, 앞쪽 바닥 원형)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Golem_Punch, "Ability.Boss.Golem.Punch", "골렘 손등 치기 (내려치기보다 빠름, 앞쪽 넓은 부채꼴)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Golem_JumpSlam, "Ability.Boss.Golem.JumpSlam", "골렘 점프 내려찍기 (멀리 있는 플레이어에게 뛰어듦)");

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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PerfectWindow, "State.PerfectWindow", "완벽 회피 판정 중 (교체·회피 직후 짧게). 이때 맞으면 데미지 없음 + 슬로");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stun, "State.Stun", "경직·기절");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "큰 경직: 진행 중인 보스 공격 취소 (작은 경직 State.Stun은 공격 중 무시 = 슈퍼아머)");

	// 캐릭터
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Warrior, "Character.Warrior", "지금 전사를 조작 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Mage, "Character.Mage", "지금 마법사를 조작 중");

	// 쿨다운
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Warrior_DashSlash, "Cooldown.Warrior.DashSlash", "돌진 베기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Warrior_GroundSlam, "Cooldown.Warrior.GroundSlam", "내려찍기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Mage_FireBolt, "Cooldown.Mage.FireBolt", "화염탄 발사 간격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Mage_FrostBolt, "Cooldown.Mage.FrostBolt", "냉기탄 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Mage_Thunder, "Cooldown.Mage.Thunder", "낙뢰 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dodge, "Cooldown.Dodge", "회피 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Golem_Slam, "Cooldown.Boss.Golem.Slam", "골렘 내려치기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Golem_Punch, "Cooldown.Boss.Golem.Punch", "골렘 손등 치기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Boss_Golem_JumpSlam, "Cooldown.Boss.Golem.JumpSlam", "골렘 점프 내려찍기 쿨다운");

	// 데미지
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DamageMultiplier, "SetByCaller.DamageMultiplier", "스킬 데미지 배율 (공격력 × 배율)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Duration, "SetByCaller.Duration", "지속 시간(초): 공통 쿨다운 GE에 능력이 넣음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Shatter, "Damage.Shatter", "산산조각 추가 피해: [빙결]을 전사가 깸");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_CrackBurst, "Damage.CrackBurst", "균열 폭발 피해: [균열]을 마법사 낙뢰가 터뜨림 (연쇄)");
}
