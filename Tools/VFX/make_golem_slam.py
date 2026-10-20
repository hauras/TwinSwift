# 골렘 땅 충격 이펙트: NS_Big_Hammer_Impact(충격파 링·연기·돌 파편·바닥 자국)를 복사해 NS_GolemSlam으로
# 원본은 하늘색 마법 느낌 → 골렘 눈 발광(따뜻한 크림색)에 맞춰 옅은 금색(색상 42°)
# 균열 폭발 NS_CrackBurst(진한 주황 25°)와는 채도·밝기로 구분됨 (이쪽은 원본 채도가 낮아 흰빛에 가까운 금색)
import unreal

SOURCE = "/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX/NS_Big_Hammer_Impact"
TARGET = "/Game/Assets/VFX/Boss/NS_GolemSlam"
HUE = 42.0

eal = unreal.EditorAssetLibrary
if not eal.does_asset_exist(TARGET):
    eal.duplicate_asset(SOURCE, TARGET)

system = unreal.load_asset(TARGET)
print(unreal.TSWeaponTrailLibrary.tint_system_hue(system, HUE, True)[-300:])
eal.save_loaded_asset(system, only_if_is_dirty=False)
print("저장:", TARGET)
