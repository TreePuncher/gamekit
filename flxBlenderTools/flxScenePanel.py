import bpy


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


class flxExportMetaOperator(bpy.types.Operator):
    bl_idname = "scene.flx_export"
    bl_label = "export meta"   
    
    def execute(self, context):
        MetaDataOut = ""

        for obj in context.selectable_objects:
            if hasattr(obj, "flxCollider"):
                MetaDataOut = MetaDataOut + obj.flxCollider.GetMetaDataString()

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
        row.operator("scene.flx_export")


def register():
    bpy.utils.register_class(flxExportMetaOperator)
    bpy.utils.register_class(flxMateral)
    bpy.utils.register_class(flxScenePanel)

def unregister():
    bpy.utils.unregister_class(flxScenePanel)
    bpy.utils.unregister_class(flxMateral)
    bpy.utils.unregister_class(flxExportMetaOperator)


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