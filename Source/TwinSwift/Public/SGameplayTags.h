#pragma once

#include "NativeGameplayTags.h"

/**
 * 게임플레이 태그 모음
 * C++에 선언해 두면 이름 오타를 컴파일할 때 잡고, 에디터 태그 목록에도 자동으로 나옴
 * 마법사 태그는 마법사 스킬을 만들 때마다 추가
 */
namespace SGameplayTags
{
	// ---------- 입력 (역할 기준 이름. 실제 키는 IMC_Default에서 정함 → 키를 바꿔도 코드는 그대로) ----------
	// 지금 키: 기본 공격 좌클릭, 전사 1·2, 마법사 3·4, 회피 Space. 상대 캐릭터 스킬을 누르면 교체
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Warrior_Skill1);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Warrior_Skill2);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Mage_Skill1);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Mage_Skill2);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dodge);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Swap);

	// ---------- 능력 이름표 (서로 막기·취소에 사용) ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Warrior_Combo);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Warrior_DashSlash);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Warrior_GroundSlam);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Warrior_SpinAssist);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Mage_FireBolt);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Mage_FrostBolt);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Mage_Thunder);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Swap);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
	// 보스 공격 (BT가 이 태그로 발동). 부모 태그(Ability.Boss)로 보스 공격 전부를 한 번에 취소
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Golem_Slam);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Golem_Punch);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Golem_JumpSlam);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Golem_Intro);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Golem_EarthRage);

	// ---------- 몽타주 이벤트 (Notify → 능력에 신호) ----------
	// 칼이 닿는 프레임 → 타격 판정
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Montage_Hit);

	// ---------- 표식 (확정 표식, 상대 캐릭터가 터뜨림) ----------
	// 전사 → 마법사 낙뢰가 터뜨림
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Crack);
	// 마법사 → 전사가 산산조각
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Frost);

	// ---------- 상태 ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectWindow);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stun);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	// 큰 경직 (보스 갑옷 파괴 등). 작은 경직(State.Stun)과 달리 진행 중인 보스 공격을 취소
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Armored);

	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Combo);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Dodge);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Swap);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Window_Skill);

	// ---------- 지금 조작 중인 캐릭터 (교체) ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Warrior);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Mage);

	// ---------- 쿨다운 (쿨다운 GE가 이 태그를 붙임) ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Warrior_DashSlash);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Warrior_GroundSlam);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Mage_FireBolt);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Mage_FrostBolt);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Mage_Thunder);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Golem_Slam);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Golem_Punch);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Golem_JumpSlam);

	// ---------- 데미지 (스킬마다 배율을 GE에 넘김) ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DamageMultiplier);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	// ---------- 데미지 종류 (데미지 스펙에 붙여 숫자 색·연출 구분) ----------
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Shatter);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CrackBurst);

	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Conductor_Ring);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Conductor_Ring);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Conductor_Fan);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Conductor_Fan);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Conductor_Spiral);
	TWINSWIFT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_Conductor_Spiral);
}
