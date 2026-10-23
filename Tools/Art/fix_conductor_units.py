# 탄막 지휘자 리깅 FBX 단위 고치기 (블렌더에서 실행: blender -b Conductor_Rig.blend --python fix_conductor_units.py -- <출력.fbx>)
# 문제: 미터 단위 그대로 내보내면 언리얼에서 뿌리 뼈에 크기 100이 붙음 → 리타깃 애니는 뿌리 크기를 1로 써서 몸이 1/100로 줄어듦
# 해결: 장면 단위를 cm(0.01)로, 모델을 100배 키워 적용 → 뼈 위치가 cm 값, 뿌리 크기 1
import sys

import bpy

out_path = sys.argv[-1]
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 0.01

arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
meshes = [o for o in bpy.data.objects if o.type == 'MESH' and o.parent == arm]

bpy.ops.object.mode_set(mode='OBJECT') if bpy.context.object and bpy.context.object.mode != 'OBJECT' else None
bpy.ops.object.select_all(action='DESELECT')
for o in [arm] + meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = arm

# 원점 기준 100배 → 크기 적용 (뼈 길이·정점이 cm 값이 됨)
arm.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

bpy.ops.export_scene.fbx(
    filepath=out_path,
    use_selection=True,
    object_types={'ARMATURE', 'MESH'},
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    add_leaf_bones=False,
    bake_anim=False,
    mesh_smooth_type='FACE',
    path_mode='COPY',
    embed_textures=True,
)
print("EXPORTED", out_path, "bones", len(arm.data.bones), "height", round(max(v.co.z for m in meshes for v in m.data.vertices), 1))
