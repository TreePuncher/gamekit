# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

bl_info = {
    "name" : "flxBlenderTools",
    "author" : "Robert May",
    "description" : "",
    "blender" : (2, 80, 0),
    "location" : "",
    "warning" : "",
    "category" : "Generic"
}

import sys
import os

if "bpy" in locals():
    import importlib
    importlib.reload(flxScenePanel)
    importlib.reload(flxObjectPanel)
    importlib.reload(flxCollider)
    importlib.reload(flxModel)
else:
    from . import flxScenePanel
    from . import flxObjectPanel
    from . import flxCollider
    from . import flxModel


import bpy

modules = [flxScenePanel,  flxObjectPanel, flxCollider, flxModel]

def register():
    pass
    for module in modules:
        module.register()

def unregister():
    pass
    for module in modules:
        module.unregister()