# 행동 허용 창(ANS_ActionWindow)을 몽타주에 놓기 + 전사 콤보 재생 속도
# 규칙: 타격(판정·시전) 전에는 끊을 수 없음 → 타격이 끝난 직후부터 몽타주 끝까지 창
#   - 시간은 몽타주 시간 기준 (재생 속도를 바꿔도 같이 줄어듦)
#   - 전사 1·2타는 판정 구간을 재서 정함 (setup_slash_fx.py의 STRIKES와 같은 숫자)
#   - 신호(AN_SendEvent)로 판정하는 몽타주(내려찍기 착지, 마법사 시전)는 그 신호 시각부터
# 다시 실행해도 됨: ActionWindow 트랙을 지우고 다시 만듦
import re
import unreal

AL = unreal.AnimationLibrary
eal = unreal.EditorAssetLibrary

TRACK = "ActionWindow"
ALL = ["Window.Combo", "Window.Dodge", "Window.Swap", "Window.Skill"]
CANCEL = ["Window.Dodge", "Window.Swap", "Window.Skill"]
W = "/Game/Assets/Characters/Warrior/Montages/"
M = "/Game/Assets/Characters/Mage/Montages/"

# (몽타주, 창 시작(초) 또는 "event"(첫 AN_SendEvent 시각), 허용 태그, 이유)
WINDOWS = [
    (W + "AM_Warrior_Combo1", 0.82, ALL, "1타 판정 0.53~0.80 직후"),
    (W + "AM_Warrior_Combo3", 1.15, ALL, "2타 판정 0.73~1.13 직후"),
    (W + "AM_Warrior_DashSlash", 1.62, CANCEL, "돌진 베기 마지막 판정 1.37~1.60 직후"),
    (W + "AM_Warrior_GroundSlam", "event", CANCEL, "착지 신호(약 1.23초)부터"),
    (M + "AM_Mage_FireBolt", "event", CANCEL, "화염탄 발사 신호부터"),
    (M + "AM_Mage_FrostBolt", "event", CANCEL, "냉기탄 발사 신호부터"),
    (M + "AM_Mage_Thunder", "event", CANCEL, "낙뢰 시전 신호부터"),
]

# 전사 콤보 재생 속도 (GA_Warrior_Attack의 ComboSteps 순서대로)
COMBO_GA = "/Game/Abilities/Warrior/GA_Warrior_Attack"
COMBO_RATES = [1.35, 1.25]


def tag_container(names):
    c = unreal.GameplayTagContainer()
    inner = ",".join(f'(TagName="{n}")' for n in names)
    c.import_text(f"(GameplayTags=({inner}))")
    return c


def first_send_event_time(montage):
    for event in AL.get_animation_notify_events(montage):
        notify = event.get_editor_property("notify")
        if notify and "SendEvent" in notify.get_class().get_name():
            return AL.get_anim_notify_event_trigger_time(event)
    return None


for path, start, tags, reason in WINDOWS:
    montage = unreal.load_asset(path)
    if not montage:
        print("없음:", path)
        continue
    length = montage.get_play_length()

    if start == "event":
        start = first_send_event_time(montage)
        if start is None:
            print("신호 없음 → 건너뜀:", montage.get_name())
            continue

    # 이전에 놓은 창 지우고 다시
    if AL.is_valid_anim_notify_track_name(montage, TRACK):
        AL.remove_animation_notify_track(montage, TRACK)
    AL.add_animation_notify_track(montage, TRACK)

    state = AL.add_animation_notify_state_event(montage, TRACK, start, length - start, unreal.ANS_ActionWindow)
    state.set_editor_property("window_tags", tag_container(tags))
    eal.save_loaded_asset(montage)
    print(f"{montage.get_name()}: {start:.2f}~{length:.2f}초 {', '.join(t.split('.')[1] for t in tags)} ({reason})")

# 콤보 재생 속도
ga = unreal.load_asset(COMBO_GA)
cdo = unreal.get_default_object(ga.generated_class())
steps = list(cdo.get_editor_property("combo_steps"))
for step, rate in zip(steps, COMBO_RATES):
    # EditDefaultsOnly라 set_editor_property로 못 바꿈 → 글자로 내보냈다가 PlayRate 숫자만 바꿔 다시 넣음
    step.import_text(re.sub(r"PlayRate=[0-9.]+", f"PlayRate={rate:.6f}", step.export_text()))
cdo.set_editor_property("combo_steps", steps)
unreal.BlueprintEditorLibrary.compile_blueprint(ga)
eal.save_loaded_asset(ga)
print("콤보 재생 속도:", [round(s.get_editor_property("play_rate"), 2) for s in cdo.get_editor_property("combo_steps")])
