# <pep8 compliant>
#
# 1.0

import os
import time
import struct
import math

import bpy
import bpy_extras.io_utils
import mathutils
import random

from array import *
from collections import namedtuple
from mathutils import Vector
from random import randint

mesh_vao_start = 0
mesh_vbo_start = 0

def dump(obj):
 for attr in dir(obj):
  if hasattr( obj, attr ):
   print( "obj.%s = %s" % (attr, getattr(obj, attr)))
   
def mesh_triangulate( me ):
 import bmesh
 bm = bmesh.new()
 bm.from_mesh( me )
 bmesh.ops.triangulate( bm, faces = bm.faces )
 bm.to_mesh( me )
 bm.free()

def write_bloc_size( file, offset, bloc_start ):
 current_position = file.tell()
 file.seek( offset, 0 )
 file.write( struct.pack( 'I', ( current_position - bloc_start ) ) )
 file.seek( current_position, 0 )
 
def write_bloc_offset( file, offset ):
 current_position = file.tell()
 file.seek( offset, 0 )
 file.write( struct.pack( 'I', current_position ) )
 file.seek( current_position, 0 )
 
def write_entity_count( file, offset, count ):
 current_position = file.tell()
 file.seek( offset, 0 )
 file.write( struct.pack( 'I', count ) )
 file.seek( current_position, 0 )
 
def write_padding( file ):	
 while file.tell() % 16 != 0:
  file.write( struct.pack( 'B', 0xFF ) )
   
def write_header( file, version ):
 file.write( bytes( version ) )
 file.write( struct.pack( 'B', 0x0 ) ) # TODO: features bitfield
 file.write( struct.pack( 'I', 0 ) ) # data start offset
 file.write( struct.pack( 'I', 0 ) ) # vbo size
 file.write( struct.pack( 'I', 0 ) ) # ibo size
 write_padding( file )

def write_material( external_file, materialObj, out_path ):
 mat_flags = []

 if materialObj.material.use_shadeless:
  mat_flags.append( "shadeless," )
 
 mat_file = open( external_file, 'w' )
 mat_file.write( "name: " + materialObj.name + "\n" )
 mat_file.write( "type: opaque\n" ) # TODO: find a nice way to handle mattype
 mat_file.write( "reflectivity: { %f, %f, %f }\n" % ( materialObj.material.specular_color[0], materialObj.material.specular_color[1], materialObj.material.specular_color[2] ) )
 mat_file.write( "diffuse: { %f, %f, %f }\n" % ( materialObj.material.diffuse_color[0], materialObj.material.diffuse_color[1], materialObj.material.diffuse_color[2] ) )
 mat_file.write( "emissivity: %f\n" % materialObj.material.emit )

 mat_file.write( "\n" )
 
 for mtex_slot in materialObj.material.texture_slots:       
  if mtex_slot:
   rel_tex_path = bpy.path.relpath( mtex_slot.texture.image.filepath, out_path ).replace( "//", "" )
   dump(mtex_slot)
   
   if mtex_slot.use_map_color_diffuse:
    mat_file.write( "albedo: %s\n" % rel_tex_path )
    mat_flags.append( "has_albedo," )
   elif mtex_slot.use_map_normal:
    mat_file.write( "normal: %s\n" % rel_tex_path )
    mat_flags.append( "has_normal," )
   elif mtex_slot.use_map_ambient:
    mat_file.write( "ao: %s\n" % rel_tex_path )
    mat_flags.append( "has_ao," )
   elif mtex_slot.use_map_specular:
    mat_file.write( "metalness: %s\n" % rel_tex_path )
    mat_flags.append( "has_metalness," )
   elif mtex_slot.use_map_hardness:
    mat_file.write( "roughness: %s\n" % rel_tex_path )
    mat_flags.append( "has_roughness," )
  
 mat_file.write( "flags: [" )
 mat_file.write( "".join( f for f in mat_flags ) )
 mat_file.write( "]\n\n" )
 mat_file.close()

def write_matlib( file, path ):
 file.write( bytearray( 'MATL', 'utf-8' ) )
 matlib_size_offset = file.tell()
 file.write( struct.pack( 'I', 0 ) )
 write_padding( file )
 
 matlib_start_offset = file.tell()
 
 mat_list = []
 for obj in bpy.context.scene.objects:
  for mat_slot in obj.material_slots:
   matHash = hash( mat_slot.name ) % ( 10 ** 8 )
   
   if matHash in mat_list:
    continue
	
   mat_list.append( matHash )
   
   write_material( path + mat_slot.name + ".mrf", mat_slot, path )
   
   file.write( struct.pack( 'I', matHash ) )
   file.write( bytearray( mat_slot.name + ".mrf", 'utf-8' ) )
   file.write( struct.pack( 'B', 0x0 ) )

 write_bloc_size( file, matlib_size_offset, matlib_start_offset )
 write_padding( file )

def write_mesh( file, global_matrix ):
 global mesh_vao_start
 global mesh_vbo_start
 
 mainMesh = None
 
 vbo = array( 'f' )
 ibo = array( 'I' )
 indice = 0
 indice_tracking = 0
 
 file.write( bytearray( 'SUBM', 'utf-8' ) )
 submesh_size_offset = file.tell()
 file.write( struct.pack( 'I', 0 ) )
 write_padding( file )
   
 mesh_list = []
 submesh_start_offset = file.tell()
 for obj in bpy.context.scene.objects:
  meshHash = hash( obj.name ) % ( 10 ** 8 )
  
  if meshHash in mesh_list:
   continue
   
  mesh_list.append( meshHash )
  
  bpy.context.scene.objects.active = obj
  obj.select = True
  if obj.type == 'MESH' and obj.entity == 'Mesh':
   if mainMesh == None:
    mainMesh = obj
    print( "Main mesh set to %s" % mainMesh.name )
   elif obj.parent != mainMesh:
    print( "%s => Parent %s != %s" %( obj.name, obj.parent.name, mainMesh.name ) )
    continue
	
   mesh = obj.to_mesh( bpy.context.scene, True, 'PREVIEW', calc_tessface=True )
   transform_matrix = global_matrix * obj.matrix_world
   mesh.transform( transform_matrix ) # pretransform the model since blender matrices are a total mess to deal with in the app
   
   mesh_triangulate( mesh )
   mesh.calc_tessface()
   mesh.calc_tangents()
  # mesh.flip_normals() # required for dx
   
   for mat_slot in obj.material_slots:
    if mat_slot.name:
     matHash = hash( mat_slot.name ) % ( 10 ** 8 )
     break
   
   file.write( struct.pack( 'I', int( len( vbo ) ) ) )
   file.write( struct.pack( 'I', int( len( ibo ) ) ) )
   indiceOffset = file.tell()
   file.write( struct.pack( 'I', 0 ) )
   file.write( struct.pack( 'I', matHash ) )
   write_padding( file )

   indice_tracking = 0
   for face in mesh.polygons:
    for loop_index in face.loop_indices:
     ibo.append( indice )
 
     vertex = mesh.vertices[mesh.loops[loop_index].vertex_index]

     vbo.append( vertex.co.x )
     vbo.append( vertex.co.y )
     vbo.append( vertex.co.z )
 
     vbo.append( vertex.normal.x )
     vbo.append( vertex.normal.y )
     vbo.append( vertex.normal.z )
 
     vbo.append( mesh.uv_layers.active.data[loop_index].uv[0] )
     vbo.append( 1.0 - mesh.uv_layers.active.data[loop_index].uv[1] )
        
     vbo.append( mesh.loops[loop_index].tangent.x )
     vbo.append( mesh.loops[loop_index].tangent.y )
     vbo.append( mesh.loops[loop_index].tangent.z )
	 
     bitangent = mesh.loops[loop_index].bitangent_sign * vertex.normal.cross( mesh.loops[loop_index].tangent )

     vbo.append( bitangent.x )
     vbo.append( bitangent.y )
     vbo.append( bitangent.z )
 
     indice += 1
     indice_tracking += 1
   
   write_entity_count( file, indiceOffset, indice_tracking )
   
 write_bloc_size( file, submesh_size_offset, submesh_start_offset )
 
 write_bloc_offset( file, 4 )
 
 mesh_vbo_start = file.tell()
 for x in vbo:
  file.write( struct.pack( 'f', x ) )    
 write_bloc_size( file, 8, mesh_vbo_start )
 
 mesh_vao_start = file.tell()
 for x in ibo:
  file.write( struct.pack( 'I', x ) )    
 write_bloc_size( file, 12, mesh_vao_start )
 
def save(filepath, global_matrix, version):					
 bpy.ops.object.mode_set( mode='OBJECT' )
 file = open( filepath, 'w+b' )
 path = os.path.dirname( filepath ) + '\\'
 
 #=============================================================================
 
 write_header( file, version )
 write_matlib( file, path ) 
 write_mesh( file, global_matrix )

 #=============================================================================

 file.close()
 return {'FINISHED'}

