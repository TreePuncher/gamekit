
import bpy

class flxObjectPanel(bpy.types.Panel):
    """Creates a Panel in the object context of the properties editor"""
    bl_label = "FlexKit Object Tags"
    bl_idname = "Object_PT_layout"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"
    
    def draw(self, context):
        layout = self.layout
        scene = context.scene
        
        layout.label(text="Add/Remove Tags")

        row = layout.row()
        row.operator("object.flx_togglecollidertag")
        row.operator("object.flx_togglemodeltag")
        
        if hasattr(bpy.context.active_object, "flxModel"):
            if bpy.context.active_object.flxModel.enabled:
                bpy.context.active_object.flxModel.draw(layout, context);

        if hasattr(bpy.context.active_object, "flxCollider"):
            if bpy.context.active_object.flxCollider.enabled:
                bpy.context.active_object.flxCollider.draw(layout, context);


def register():
    bpy.utils.register_class(flxObjectPanel)

def unregister():
    bpy.utils.unregister_class(flxObjectPanel)


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