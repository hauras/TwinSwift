# 맞았을 때 화면 가장자리 빨간 번쩍
# 1) UI 재질 M_HUD_HitVignette: 화면 가운데는 투명, 가장자리로 갈수록 빨갛게 (UV에서 가운데까지 거리)
# 2) WBP_TwinHUD 맨 아래(다른 HUD 뒤)에 화면 전체 이미지 HitFlash. 투명도는 C++(USTwinHUDWidget::OnPlayerDamaged)가 조절
# 다시 실행해도 됨
import unreal

if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError("PIE 중에는 에셋을 못 고침. PIE를 끄고 다시 실행")

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT_DIR = "/Game/UI/Materials"
MAT_NAME = "M_HUD_HitVignette"
HUD = "/Game/UI/WBP_TwinHUD"

# ---------- 재질 ----------
path = f"{MAT_DIR}/{MAT_NAME}"
if eal.does_asset_exist(path):
    mat = unreal.load_asset(path)
    MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

# 가운데(0.5, 0.5)에서 거리: 가장자리 0.5, 모서리 0.71
uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
center = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -900, 120)
center.set_editor_property("r", 0.5)
center.set_editor_property("g", 0.5)
dist = MEL.create_material_expression(mat, unreal.MaterialExpressionDistance, -700, 40)
MEL.connect_material_expressions(uv, "", dist, "A")
MEL.connect_material_expressions(center, "", dist, "B")

# (거리 - 0.28) × 3.5 → 0~1로 자름 → 1.6제곱 (가운데 넓게 비우고 가장자리만 진하게)
sub = MEL.create_material_expression(mat, unreal.MaterialExpressionSubtract, -500, 40)
sub.set_editor_property("const_b", 0.28)
MEL.connect_material_expressions(dist, "", sub, "A")
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 40)
mul.set_editor_property("const_b", 3.5)
MEL.connect_material_expressions(sub, "", mul, "A")
sat = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -200, 40)
MEL.connect_material_expressions(mul, "", sat, "")
power = MEL.create_material_expression(mat, unreal.MaterialExpressionPower, -60, 40)
power.set_editor_property("const_exponent", 1.6)
MEL.connect_material_expressions(sat, "", power, "Base")
MEL.connect_material_property(power, "", unreal.MaterialProperty.MP_OPACITY)

red = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, -150)
red.set_editor_property("constant", unreal.LinearColor(0.9, 0.02, 0.02, 1.0))
MEL.connect_material_property(red, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
eal.save_loaded_asset(mat)
print("재질:", path)

# ---------- HUD 이미지 ----------
hud = unreal.load_asset(HUD)
lib = unreal.TSWidgetEditLibrary
image = lib.add_child_widget(hud, "CanvasPanel_0", unreal.Image, "HitFlash")
brush = image.get_editor_property("brush")
brush.set_editor_property("resource_object", mat)
image.set_editor_property("brush", brush)
image.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
# 에디터 미리 보기에서도 안 보이게 (게임에서는 맞을 때만 C++가 올림)
image.set_editor_property("render_opacity", 0.0)

# 화면 전체로 늘리고, 다른 HUD 뒤에 그림
slot = image.get_editor_property("slot")
slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
slot.set_z_order(-10)

lib.mark_modified(hud)
unreal.BlueprintEditorLibrary.compile_blueprint(hud)
eal.save_loaded_asset(hud)
print("HUD:", [l for l in lib.dump_widget_tree(hud).splitlines() if "HitFlash" in l])
