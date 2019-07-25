import bpy
import bgl
import gpu

from gpu_extras.batch import batch_for_shader
from bpy.types import Operator

addon_keymaps = []

class flxScene(bpy.types.PropertyGroup):
    Enabled : bpy.props.BoolProperty(False)
    SceneQuadTreeSize : bpy.props.IntProperty(1000)

class flxDrawSceneOperator(Operator):
    bl_idname = "object.draw_op"
    bl_label = "Toggle Draw Scene Quad Tree"
    bl_description = "Operator for toggling rendering of scene Quad Tree"
    bl_options = {'REGISTER'}

    def __init__(self):
        self.draw_handle = None
        self.draw_event = None
        self.widgets = []

    def pool(self, context):
        return False

    def invoke(self, context, event):
        if not hasattr(context.active_object, "flxScene") and context.active_object.flxScene.Enabled:
            self.report({'INFO'}, "Please Select a flxScene first!")
            return {"CANCELLED"}

        self.create_batch()

        args = (self, context)
        self.register_handlers(args, context)
        
        return {"RUNNING_MODAL"}

    def register_handlers(self, args, context):
        self.draw_handle = bpy.types.SpaceView3D.draw_handler_add(
            self.draw_callback, args, "WINDOW", "POST_VIEW")

        self.draw_event = context.window_manager.event_timer_add(0.1, window=context.window)

    def unregister_handlers(self, context):
        context.window_manager.event_timer_remove(self.draw_event)
        bpy.types.SpaceView3D.draw_handler_remove(self.draw_handle, "WINDOW")

        self.draw_handle = None
        self.draw_event = None

    def modal(self, context, event):
        if context.area:
            context.area.tag_redraw()
        
        if event.type in {"ESC"}:
            self.unregister_handlers(context)
            return {"CANCELLED"}

    def finish(self, context):
        self.unregister_handlers(context)
        return {"FINISHED"}

    def create_batch(self):
        vertices = [(0, 3, 3), (0, 4, 4), (0, 6, 2), (0, 3, 3)]

        self.shader = gpu.shader.from_builtin("3D_UNIFORM_COLOR")
        self.batch = batch_for_shader(self.shader, 'LINE_STRIP', {"pos" : vertices})
    
    def draw_callback(self, op, context):
        bgl.glLineWidth(4)
        self.shader.bind()
        self.shader.uniform_float("color", (1, 1, 1))
        self.batch.draw(self.shader)


class flxMateral(bpy.types.PropertyGroup):
    assetID : bpy.props.StringProperty(name="assetID")
    assetGUID : bpy.props.IntProperty(name="assetGUID")
    #textures : bpy.props.CollectionProperty(name="textures")

    def draw(self, layout, context):
        layout.label(text="flxMaterial")
        obj = context.object
        column = layout.column()

        #column.prop(obj, "name")
    def GetMetaDataString(self):
        if  self.enabled:
            strOut = "" + str(self.assetID) + "{}};\n"
            return strOut
        else:
            return ""


class flxCreateSceneOperator(bpy.types.Operator):
    bl_idname = "scene.flx_create_flx_scene"
    bl_label = "create scene"   
    
    def execute(self, context):
        bpy.ops.object.empty_add(type="PLAIN_AXES", location=(0, 0, 0))
        context.active_object.flxScene.Enabled = True

        context.active_object.name = "Scene"

        context.active_object.lock_location[0] = True
        context.active_object.lock_location[1] = True
        context.active_object.lock_location[2] = True

        context.active_object.lock_rotation[0] = True
        context.active_object.lock_rotation[1] = True
        context.active_object.lock_rotation[2] = True

        context.active_object.lock_scale[0] = True
        context.active_object.lock_scale[1] = True
        context.active_object.lock_scale[2] = True

        #context.active_object.hide_viewport = True

        return {'FINISHED'}


class flxExportMetaOperator(bpy.types.Operator):
    bl_idname = "scene.flx_export"
    bl_label = "export meta"   
    
    def execute(self, context):
        MetaDataOut = ""

        for obj in context.selectable_objects:
            if obj.flxCollider.enabled:
                MetaDataOut = MetaDataOut + obj.flxCollider.GetMetaDataString()
            
            if obj.flxObject.enabled:
                MetaDataOut = MetaDataOut + obj.flxObject.GetMetaDataString()


        print(MetaDataOut)

        return {'FINISHED'}

class flxScenePanel(bpy.types.Panel):
    """Creates a Panel in the object context of the properties editor"""
    bl_label = "FlexKit Scene Properties"
    bl_idname = "SCENE_PT_layout"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"
    
    def draw(self, context):
        layout = self.layout
        
        row = layout.row()
        row.operator("scene.flx_create_flx_scene")
        row.operator("scene.flx_export")
        row.operator("object.draw_op")

        #if bpy.context.active_object != None:
        #    if(bpy.context.active_object.flxScene.Enabled):
        #        print("Hello Scene!")


def register():
    bpy.utils.register_class(flxDrawSceneOperator)
    bpy.utils.register_class(flxCreateSceneOperator)
    bpy.utils.register_class(flxExportMetaOperator)
    bpy.utils.register_class(flxMateral)
    bpy.utils.register_class(flxScenePanel)
    bpy.utils.register_class(flxScene)

    bpy.types.Object.flxScene = bpy.props.PointerProperty(type=flxScene)
    bpy.types.Object.flxMateral = bpy.props.PointerProperty(type=flxMateral)

def unregister():
    bpy.utils.unregister_class(flxScene)
    bpy.utils.unregister_class(flxScenePanel)
    bpy.utils.unregister_class(flxMateral)
    bpy.utils.unregister_class(flxExportMetaOperator)
    bpy.utils.unregister_class(flxCreateSceneOperator)
    bpy.utils.unregister_class(flxDrawSceneOperator)


#/**********************************************************************
#
#Copyright (c) 2019 Robert May
#
#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the "Software"),
#to deal in the Software without restriction, including without limitation
#the rights to use, copy, modify, merge, publish, distribute, sublicense,
#and/or sell copies of the Software, and to permit persons to whom the
#Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#**********************************************************************/