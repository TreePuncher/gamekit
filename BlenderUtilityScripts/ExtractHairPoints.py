import bpy
import os
import re

from bpy.types import Operator
from bpy.props import *

from bpy_extras.io_utils import ExportHelper

class PARTICLES_EXPORT_HAIR_CSV(Operator, ExportHelper):
    bl_idname = "export.export_curves_csv"
    bl_label = "Export Hair CSV"
    filename_ext = ".csv"

                
    def GetHairSystems(self, particle_systems, start = 0):
        hairSystems = []
        
        for system in particle_systems:
            if(system.settings.type == 'HAIR'):
                hairSystems.append(system)
        
        return hairSystems
        
        
    def execute(self, context):
        try:       
            object = bpy.context.object
            
            if(object is None):
                self.report({"WARNING"}, "No Object Selected")
                print(1)
                return {'CANCELLED'} 
            
            eval_ob = bpy.context.evaluated_depsgraph_get().objects.get(object.name)
                        
            if(len(eval_ob.particle_systems) == 0):
                self.report({"WARNING"}, "No Hair Particle System!")
                print(1)
                return {'CANCELLED'} 
            
            hairSystems = self.GetHairSystems(eval_ob.particle_systems);
                        
            for i, system in enumerate(hairSystems):
                hairs = system.particles
                path = self.filepath
                if len(hairSystems) > 1:
                    path = self.filepath.rsplit(".",  1)[0] + ".{}.csv".format(i)
                
                print(path)
                f = open(path, 'wt')
                for i, h in enumerate(hairs):
                    for i, hv in enumerate(h.hair_keys):
                        f.write('({x} {y} {z}), '.format(x=hv.co.x, y=hv.co.y, z=hv.co.z)) 
                        
                    f.write('\n')
                f.close()
        except:
            self.report({"WARNING"}, "Cancelled Export")
            return {'CANCELLED'}    
        
        return {'FINISHED'}

def menu_func_export(self, context):
    self.layout.operator(PARTICLES_EXPORT_HAIR_CSV.bl_idname, text="Hair Export (.csv)")
  
def register():
    bpy.utils.register_class(PARTICLES_EXPORT_HAIR_CSV)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)
    print("Hair Export Registered");
    
def unregister():
    bpy.utils.unregister_class(PARTICLES_EXPORT_HAIR_CSV)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    print("Hair Export Unregistered");

    
if __name__ == "__main__":
    register()