# 낙뢰 번개(NS_Lightning_Strike) 색을 빨강 → 파랑·흰색으로
# 번개 전용 머티리얼 인스턴스 3개만 바꿈 (다른 이펙트와 같이 쓰는 MI_Colorful·MI_Flare는 건드리지 않음)
#   MI_Lightning_Inst_1/2: 번개 줄기 "Emissive BOT"
#   MI_Rock_Inst_6: 바닥에 튀는 돌 조각 발광
# 외부 팩(깃 제외) 안을 직접 고치므로, 팩을 새로 받으면 이 스크립트를 다시 돌릴 것. 원래 값은 아래 ORIGINAL에 기록
import unreal

MEL = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
D = "/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/Materials/Instance_Materials"

# (머티리얼 인스턴스, 파라미터, 새 색). 밝기(HDR 세기)는 원래와 비슷하게 맞춤
BLUE = [
    ("MI_Lightning_Inst_1", "Emissive BOT", unreal.LinearColor(2.5, 6.0, 15.0, 1.0)),
    ("MI_Lightning_Inst_2", "Emissive BOT", unreal.LinearColor(2.5, 6.0, 15.0, 1.0)),
    ("MI_Rock_Inst_6", "Emissive Color", unreal.LinearColor(0.5, 1.2, 2.5, 1.0)),
    ("MI_Rock_Inst_6", "Emissive Color Extra", unreal.LinearColor(1.2, 2.5, 5.0, 1.0)),
]

# 원래 값 (되돌릴 때)
ORIGINAL = {
    ("MI_Lightning_Inst_1", "Emissive BOT"): (15.0, 2.269915, 1.373961),
    ("MI_Lightning_Inst_2", "Emissive BOT"): (15.0, 2.269915, 1.373961),
    ("MI_Rock_Inst_6", "Emissive Color"): (2.0, 0.556502, 0.476885),
    ("MI_Rock_Inst_6", "Emissive Color Extra"): (5.0, 0.979647, 0.830405),
}

for name, param, color in BLUE:
    mi = unreal.load_asset(f"{D}/{name}")
    MEL.set_material_instance_vector_parameter_value(mi, param, color)
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi, only_if_is_dirty=False)
    print(name, param, MEL.get_material_instance_vector_parameter_value(mi, param))

# ---------- 이펙트 안의 입자 색 (머티리얼이 아니라 Niagara 모듈 색·색 커브로 정하는 부분) ----------
# 섬광·불꽃 머티리얼(MI_Colorful·MI_Flare)은 다른 이펙트 15개와 같이 써서 머티리얼은 못 바꿈 → 이펙트의 입자 색을 돌림
# C++ 도우미 UTSWeaponTrailLibrary.TintSystemHue: 색상만 215도(파랑)로, 밝기·채도 그대로. 흰색·회색은 그대로
# 조건: TintSystemHue가 들어간 C++이 빌드돼 있어야 함
TARGET_HUE = 215.0
FX = "/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX"
for name in ("NS_Lightning_Strike", "NS_Lightning_Owner_Cast"):
    system = unreal.load_asset(f"{FX}/{name}")
    report = unreal.TSWeaponTrailLibrary.tint_system_hue(system, TARGET_HUE, True)
    eal.save_loaded_asset(system, only_if_is_dirty=False)
    print(name)
    print(report)
