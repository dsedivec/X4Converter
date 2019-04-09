import re
import bpy
from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator
import xml.etree.ElementTree as ET
from X4ConverterBlenderAddon.common import *

    

class ImportAsset(Operator, ImportHelper):
    bl_idname = "import_scene.x4import"
    bl_label = "X4 Foundations Import"
    bl_options = {'REGISTER' }
    filename_ext = ".xml"

    filter_glob: StringProperty(
        default="*.xml",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )



    run_converter: BoolProperty(
        name="Run Converter",
        description="Run Converter on Target File instead of assuming it's already been done",
        default=True,
    )

    delete_lods: BoolProperty(
        name="Delete Lods",
        description="Delete Lower Lods - Hides if false",
        default=False,
    )

    hide_collision: BoolProperty(
        name="Hide Collision Mesh",
        description="Hide Collision Mesh from scene",
        default=True,
    )
    import_animations: BoolProperty(
        name="Import Animations (Experimental)",
        description="Import Animation Data - wip",
        default=True,
    )
    scale: EnumProperty(
        name="Scale",
        description="Desired scale - use smaller values if the ship doesn't show up right",
        default="1",
        items=[("1", "1.00x","No scaling"),
                ("0.1","0.10x","1/10th size"),
                ("0.01","0.01x","1/100th size")]
    )
    def execute(self, context):
        if (self.run_converter):
            call_converter("importxmf",self.filepath)
        base_path = re.sub(r'\.xml$', '', self.filepath)
        self.import_and_tweak_collada(context,base_path)


        if (self.import_animations):
            self.read_animations(base_path)

        ship_macro=base_path.rsplit('/',1)[1]
        ship_macro=ship_macro.replace(".out","")
        # also the root part
        root_part=bpy.data.objects[ship_macro]
        if (self.scale != "1.00"):
            scale_num = float(self.scale)
            root_part.scale = (scale_num, scale_num, scale_num)
        
        return {'FINISHED'}


    def import_and_tweak_collada(self,ctx, base_path):
        dae_path = base_path+ ".dae"
        bpy.ops.wm.collada_import(filepath=dae_path)
        
        if (self.delete_lods):
            bpy.ops.object.select_pattern(pattern="*lod[123456789]*", case_sensitive=True, extend=False)
            bpy.ops.object.delete(use_global=True)
        else:
            for obj in ctx.scene.objects:
                if 'lod0' not in obj.name and 'lod' in obj.name:
                    obj.hide_select = True
                    obj.hide_viewport = True
                    obj.hide_render = True

        if (self.hide_collision):
            for obj in ctx.scene.objects:
                if 'collision' in obj.name:
                    obj.hide_select = True
                    obj.hide_viewport = True
                    obj.hide_render = True


    def read_animations(self,base_path):
        anixml_path = base_path + ".anixml"
        tree = ET.parse(anixml_path)
        root = tree.getroot()
        print("Starting animations")
        for part in root[0]:  # we wrote data first
            part_name = part.attrib['name']
            part_meta = root[1].findall("[@name='"+part_name+"']")
            print(part_meta)
            obj = bpy.data.objects[part_name]
            for cat in part:
                self.handle_category(obj,cat,part_name,part_meta)

        self.read_metadata(root)

    def handle_category(self,obj,cat,part_name,part_meta):
        for anim in cat:

            self.handle_category_anim(obj,cat,part_name,anim,part_meta)


    def handle_category_anim(self,obj,cat,part_name,anim,part_meta):

    
        anim_name = anim.attrib['subname']
        action_name = part_name + anim_name

        anim_meta = part_meta[0].findall("[@subname='"+anim_name+"']")[0]
        frame_offset = anim_meta.find('frames').attrib['start']
        if  obj.animation_data is None:
            obj.animation_data_create()
        for path in anim:
            path_name = path.tag
            for axis in path:
                self.read_frames(axis, obj, path_name,frame_offset)

    def read_frames(self,axis_data, obj, path_name,frame_offset):
        # TODO test/assert assumption that all anims start at 0, data wise
        groupnames = {"location": "Location", "rotation_euler": "Rotation", "scale": "Scaling"}
        axis_name = axis_data.tag
        axes = ["X", "Y", "Z"]
        axis_idx = axes.index(axis_name)
        # TODO offset instead?
        starting_data={"location": obj.location, "rotation_euler": obj.rotation_euler, "scale": obj.scale}
        for f in axis_data:
            frame = int(f.attrib["id"])+frame_offset
            obj.keyframe_insert(data_path=path_name,
                                index=axis_idx,
                                frame=frame,
                                group=groupnames[path_name])
            fc = self.get_fcurve(obj,path_name,axis_idx)
            kf = self.get_keyframe(fc,frame)

            # Ugh converting world handedness bullshit
            if path_name == "location" and axis_name =="X":
                kf.co[1]=starting_data[path_name][axis_idx]-float(f.attrib["value"])
            elif path_name == "rotation_euler" and axis_name == "X":
                kf.co[1]=starting_data[path_name][axis_idx]-float(f.attrib["value"])
            elif path_name == "scale":
                # scale multiplies... I'm an idiot sometimes
                kf.co[1]=starting_data[path_name][axis_idx]*float(f.attrib["value"])
            else:
                kf.co[1]=starting_data[path_name][axis_idx]+float(f.attrib["value"])
            interp = f.attrib["interpolation"]
            if (interp == "STEP"):
                kf.interpolation="CONSTANT"
            else:
                kf.interpolation=interp

            handle_l = f.find("handle_left")
            handle_r = f.find("handle_right")
            kf.handle_left=(float(handle_l.attrib["X"]),float(handle_l.attrib["Y"]))
            kf.handle_right=(float(handle_r.attrib["X"]),float(handle_r.attrib["Y"]))

    def get_fcurve(self,obj,path,idx):
        for fc in obj.animation_data.action.fcurves:
            if fc.data_path==path and fc.array_index == idx:
                return fc

        return None
    def get_keyframe(self,fc,frame):
        for kf in fc.keyframe_points:
            if (abs(frame-kf.co[0])<0.1):
                return kf
        return None

    def read_metadata(self,root):
        print("Starting metadata")
        for conn in root[1]:  # then we wrote metadta
            tgt_name = conn.attrib["name"]
            for anim in conn:
                anim_name = anim.attrib["subname"]
                #TODO fixme
                scene = bpy.data.scenes['Scene']
                timeline_markers = scene.timeline_markers
                frame_data = anim.find("frames")
                start = int(frame_data.attrib["start"])
                end = int(frame_data.attrib["end"])
                state_name = anim_name.split("_", 1)[1]
                # TODO reverse lookup by time and concatenate?
                if timeline_markers.get(state_name):
                    continue
                elif start == end:
                    timeline_markers.new(state_name, frame=start)
                    continue
                if not timeline_markers.get(state_name + "Start"):
                    # TODO if is there check if same/warn
                    timeline_markers.new(state_name + "Start", frame=start)
                if not timeline_markers.get(state_name + "End"):
                    # TODO if is there check if same/warn
                    timeline_markers.new(state_name + "End", frame=end)

        print("Done with metadata")
