
import bpy

class flxModel(bpy.types.PropertyGroup):
    halfDimensions : bpy.props.FloatVectorProperty(name="BoundingBox half width", default=(1.0, 1.0, 1.0))
    enabled : bpy.props.BoolProperty(name="enabled")
    sceneID : bpy.props.StringProperty(name="sceneID") 
    assetID : bpy.props.StringProperty(name="assetID")
    assetGUID : bpy.props.IntProperty(name="assetGUID")
    sourceID : bpy.props.StringProperty(name="sourceID")
    materialID : bpy.props.StringProperty(name="materialID")
    #textures : bpy.props.CollectionProperty(name="textures")

    def draw(self, layout, context):
        layout.label(text="flxModel")
        obj = context.object
        column = layout.column()

        #column.prop(self.textures, "textures")
        
        #column.prop(obj, "name")
    def GetMetaDataString(self):
        if  self.enabled:
            strOut = "" + str(self.sceneID) + " : Model =\n{ \n" + "\tassetID  \t: " + str(self.assetID) + "\n" + "\tassetGUID\t: " + str(self.assetGUID) + "\n" + "};\n"
            return strOut
        else:
            return ""


class flxToggleModelOperator(bpy.types.Operator):
    bl_idname = "object.flx_togglemodeltag"
    bl_label = "Model"   
    
    def execute(self, context):       
        for object in bpy.context.selected_objects:
            if(hasattr(object, "flxModel")):
                object.flxModel.enabled = not object.flxModel.enabled
                object.flxModel.assetID = object.name
                object.flxModel.sceneID = object.name
                object.flxModel.sourceID = object.name
            
        return {'FINISHED'}

def register():
    bpy.utils.register_class(flxModel)
    bpy.utils.register_class(flxToggleModelOperator)
    
    bpy.types.Object.flxModel = bpy.props.PointerProperty(type=flxModel)


def unregister():
    bpy.utils.unregister_class(flxModel)
    bpy.utils.unregister_class(flxToggleModelOperator)


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

