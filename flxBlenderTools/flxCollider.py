import bpy
import random
from enum import Enum

class flxCollider(bpy.types.PropertyGroup):
    halfDimensions : bpy.props.FloatVectorProperty(name="BoundingBox half width", default=(1.0, 1.0, 1.0))
    enabled : bpy.props.BoolProperty(name="enabled")
    sceneID : bpy.props.StringProperty(name="sceneID") 
    assetID : bpy.props.StringProperty(name="assetID")
    assetGUID : bpy.props.IntProperty(name="assetGUID")
    sourceID : bpy.props.StringProperty(name="sourceID")

    colliderType : bpy.props.EnumProperty(items=[
                                        ("NO_COLLIDER", "NONE", "Do not generate collider", 0), 
                                        ("COLLIDER_SELF", "SELF", "Uses current meshes geometry as a collider", 1), 
                                        ("BOXCOLLIDER", "BOX", "Uses bounding box dimensions as a collider, or user defined cube", 2),
                                        ("MESHCOLLIDER", "MESHCOLLIDER", "Uses the specified asset as a triangle mesh collider", 3)],
                                        name="collidertype")
    
    otherMeshID : bpy.props.StringProperty(name="colliderid")
    
    def draw(self, layout, context):
        layout.label(text="flxCollider")
        obj = context.object
        column = layout.column()
        column.prop(obj.flxCollider, "sceneID")
        column.prop(obj.flxCollider, "assetID")
        column.prop(obj.flxCollider, "assetGUID")
        column.prop_menu_enum(obj.flxCollider, "colliderType")

        if obj.flxCollider.colliderType == 'BOXCOLLIDER':
            column.prop(obj.flxCollider, "halfDimensions")
        elif obj.flxCollider.colliderType == 'MESHCOLLIDER':
            column.prop(obj.flxCollider, "sourceID")
        
    
    def GetMetaDataString(self):
        if  self.enabled:
            strOut = "" + str(self.sceneID) + " : MeshCollider =\n{ \n" + "\tassetID  \t: " + str(self.assetID) + "\n" + "\tassetGUID\t: " + str(self.assetGUID) + "\n" + "};"
            return strOut
        else:
            return ""
            

    
class flxToggleColliderOperator(bpy.types.Operator):
    bl_idname = "object.flx_togglecollidertag"
    bl_label = "Collider"   
    
    def execute(self, context):       
        for object in bpy.context.selected_objects:
            if(hasattr(object, "flxCollider")):
                object.flxCollider.enabled = not object.flxCollider.enabled
                object.flxCollider.assetID = object.name
                object.flxCollider.sceneID = object.name
                object.flxCollider.sourceID = object.name
                object.flxCollider.assetGUID = random.randint(1, 1000000000)
            
        return {'FINISHED'}


def register():
    bpy.utils.register_class(flxCollider)
    bpy.utils.register_class(flxToggleColliderOperator)
    
    bpy.types.Object.flxCollider = bpy.props.PointerProperty(type=flxCollider)


def unregister():
    bpy.utils.unregister_class(flxCollider)
    bpy.utils.unregister_class(flxToggleColliderOperator)


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