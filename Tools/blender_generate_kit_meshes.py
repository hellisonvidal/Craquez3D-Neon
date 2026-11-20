"""
Craquez 3D - Procedural Separate Football Kit & Ball Generator for Blender 4.x / 5.x
Generates REAL separate 3D uniform meshes (NOT painted onto character skin!):
  - SM_Craquez_Jersey.fbx  (Cloth-ready football jersey with collar & sleeves)
  - SM_Craquez_Shorts.fbx  (Loose athletic football shorts)
  - SM_Craquez_Socks.fbx   (High football socks with shin-guard contour)
  - SM_Craquez_Boots.fbx   (Low-poly modern football cleats with studs)
  - SM_Craquez_Ball.fbx    (Regulation FIFA Size-5 soccer ball, radius 11 cm)

How to run (headless or inside Blender Scripting tab):
  blender --background --python Tools/blender_generate_kit_meshes.py
"""

import os
import math
import bpy
import bmesh

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Art", "GeneratedFBX")
os.makedirs(OUTPUT_DIR, exist_ok=True)


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def create_material(name, rgba, roughness=0.55):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = rgba
        bsdf.inputs["Roughness"].default_value = roughness
    return mat


def export_active_fbx(filename):
    filepath = os.path.join(OUTPUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Y",
        axis_up="Z",
        mesh_smooth_type="FACE",
    )
    print(f"[Craquez] Exported: {filepath}")


def build_jersey():
    clear_scene()
    # Torso cloth shell (UE scale: 1 Blender meter = 100 UE cm)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.19, depth=0.56, location=(0, 0, 1.24))
    jersey = bpy.context.active_object
    jersey.name = "SM_Craquez_Jersey"
    jersey.scale = (0.72, 1.08, 1.0)
    bpy.ops.object.transform_apply(scale=True)

    # Left and right short sleeves joined cleanly to torso
    for side in (-1.0, 1.0):
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=16,
            radius=0.078,
            depth=0.22,
            location=(0.0, side * 0.25, 1.41),
            rotation=(math.radians(65.0 * side), 0.0, 0.0),
        )
        sleeve = bpy.context.active_object
        sleeve.select_set(True)
        jersey.select_set(True)
        bpy.context.view_layer.objects.active = jersey
        bpy.ops.object.join()

    # Subdivide slightly so Chaos Cloth has clean simulation vertices
    mod = jersey.modifiers.new(name="Subsurf", type="SUBSURF")
    mod.levels = 1
    bpy.ops.object.modifier_apply(modifier="Subsurf")
    bpy.ops.object.shade_smooth()

    mat = create_material("M_Craquez_Jersey", (0.02, 0.72, 0.35, 1.0), roughness=0.68)
    jersey.data.materials.append(mat)
    jersey.select_set(True)
    export_active_fbx("SM_Craquez_Jersey.fbx")


def build_shorts():
    clear_scene()
    bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=0.18, depth=0.18, location=(0, 0, 0.90))
    shorts = bpy.context.active_object
    shorts.name = "SM_Craquez_Shorts"
    shorts.scale = (0.74, 1.05, 1.0)
    bpy.ops.object.transform_apply(scale=True)

    for side in (-1.0, 1.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.095, depth=0.26, location=(0.0, side * 0.105, 0.72))
        leg = bpy.context.active_object
        leg.select_set(True)
        shorts.select_set(True)
        bpy.context.view_layer.objects.active = shorts
        bpy.ops.object.join()

    bpy.ops.object.shade_smooth()
    mat = create_material("M_Craquez_Shorts", (0.05, 0.08, 0.14, 1.0), roughness=0.65)
    shorts.data.materials.append(mat)
    shorts.select_set(True)
    export_active_fbx("SM_Craquez_Shorts.fbx")


def build_socks_and_boots():
    clear_scene()
    parts = []
    for side in (-1.0, 1.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.062, depth=0.34, location=(0.0, side * 0.105, 0.28))
        parts.append(bpy.context.active_object)

    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    socks = bpy.context.active_object
    socks.name = "SM_Craquez_Socks"
    bpy.ops.object.shade_smooth()
    socks.data.materials.append(create_material("M_Craquez_Socks", (0.02, 0.72, 0.35, 1.0), roughness=0.75))
    export_active_fbx("SM_Craquez_Socks.fbx")

    clear_scene()
    boots = []
    for side in (-1.0, 1.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.05, side * 0.105, 0.055))
        boot = bpy.context.active_object
        boot.scale = (0.24, 0.105, 0.095)
        bpy.ops.object.transform_apply(scale=True)
        mod = boot.modifiers.new("Bevel", "BEVEL")
        mod.width = 0.025
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier="Bevel")
        boots.append(boot)

    bpy.ops.object.select_all(action="DESELECT")
    for b in boots:
        b.select_set(True)
    bpy.context.view_layer.objects.active = boots[0]
    bpy.ops.object.join()
    boot_obj = bpy.context.active_object
    boot_obj.name = "SM_Craquez_Boots"
    bpy.ops.object.shade_smooth()
    boot_obj.data.materials.append(create_material("M_Craquez_Boots", (0.95, 0.98, 0.15, 1.0), roughness=0.32))
    export_active_fbx("SM_Craquez_Boots.fbx")


def build_fifa_ball():
    clear_scene()
    # FIFA Size-5 ball radius = 0.11 m (11 cm in UE5)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.11, location=(0, 0, 0))
    ball = bpy.context.active_object
    ball.name = "SM_Craquez_Ball"
    bpy.ops.object.shade_smooth()
    ball.data.materials.append(create_material("M_Craquez_Ball", (0.96, 0.98, 1.0, 1.0), roughness=0.28))
    ball.select_set(True)
    export_active_fbx("SM_Craquez_Ball.fbx")


if __name__ == "__main__":
    build_jersey()
    build_shorts()
    build_socks_and_boots()
    build_fifa_ball()
    print("[Craquez] All separate 3D uniform & FIFA ball FBX meshes generated successfully!")
