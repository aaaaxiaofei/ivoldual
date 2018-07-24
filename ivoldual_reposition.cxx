/// \file ivoldual_reposition.cxx
/// Reposition vertices for mesh quality optimization.

/*
  IJK: Isosurface Jeneration Kode
  Copyright (C) 2017 Rephael Wenger

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ivoldual_compute.h"
#include "ivoldual_reposition.h"

#include "ijktriangulate.txx"

using namespace IJK;
using namespace IVOLDUAL;

void IVOLDUAL::eliminate_non_manifold_grid
(IVOLDUAL_DATA & ivoldual_data, 
 const SCALAR_TYPE isovalue0, 
 const SCALAR_TYPE isovalue1, 
 IVOLDUAL_INFO & dualiso_info) 
{
  int num_changes = 0;
  ivoldual_data.EliminateAmbigFacets(isovalue0, isovalue1, num_changes);
  dualiso_info.num_non_manifold_changes = num_changes;
}

void IVOLDUAL::laplacian_smooth_elength
(const DUALISO_SCALAR_GRID_BASE & scalar_grid,
 const IVOLDUAL_CUBE_TABLE & ivoldual_table,
 const IVOLDUAL_DATA_FLAGS & param,
 IVOL_VERTEX_ADJACENCY_LIST & vertex_adjacency_list,
 const DUAL_IVOLVERT_ARRAY & ivolv_list,
 COORD_ARRAY & vertex_coord,
 float laplacian_smooth_limit, 
 int iteration)
{
  const int d = 3;
  float dist;
  COORD_TYPE * vcoord = &(vertex_coord.front());

  for (int it = 0; it < 2*iteration+1; it++) {

    bool skipSurfaceVert = (it % 2 == 0);

    // Loop over all vertices
    for (int cur = 0; cur < vertex_adjacency_list.NumVertices(); cur++) {
      
      // Current node coordinates.
      COORD_TYPE *cur_coord = vcoord + cur*d;

      // Check if current node is on isosurface.
      const int ivolv_cur = ivolv_list[cur].patch_index;
      const TABLE_INDEX table_cur = ivolv_list[cur].table_index;
      bool curOnLower = ivoldual_table.OnLowerIsosurface(table_cur, ivolv_cur);
      bool curOnUpper = ivoldual_table.OnUpperIsosurface(table_cur, ivolv_cur);
      bool isOnSurface = curOnLower || curOnUpper;

      if (isOnSurface == skipSurfaceVert) continue;

      // Store sum of neighbor coordinates.
      COORD_TYPE neigh_sum[d]; 
      IJK::set_coord(d, 0.0, neigh_sum);
      bool flag_moving = false;
      int adj_count = 0;

      // Loop over adjacent vertices of the current vertex
      for (int  k = 0; k < vertex_adjacency_list.NumAdjacent(cur); k++) {

        // Neighbor node coordinates
        int adj = vertex_adjacency_list.AdjacentVertex(cur, k);
        COORD_TYPE *neigh_coord = vcoord + adj*d;

        // Check if neighbor node is on isosurface.
        const int ivolv_adj = ivolv_list[adj].patch_index;
        const TABLE_INDEX table_adj = ivolv_list[adj].table_index;
        bool adjOnLower = ivoldual_table.OnLowerIsosurface(table_adj, ivolv_adj);
        bool adjOnUpper = ivoldual_table.OnUpperIsosurface(table_adj, ivolv_adj);

        // Skip if a vertex and its adjacent vertex are not on the same surface.
        if ((curOnLower && !adjOnLower) || (curOnUpper && !adjOnUpper))
            continue;

        IJK::add_coord(d, neigh_sum, neigh_coord, neigh_sum);
        IJK::compute_distance(d, cur_coord, neigh_coord, dist);

        // Check if minimum distance is valid.
        if (dist < laplacian_smooth_limit) {
          flag_moving = true;
        }

        adj_count++;
      }

      // Update current node coordinate.
      if (flag_moving) {
        IJK::divide_coord(d, adj_count, neigh_sum, neigh_sum);
        IJK::copy_coord(d, neigh_sum, cur_coord);
      }
    }
  }
}

void IVOLDUAL::gradient_smooth_elength
(const std::vector<VERTEX_INDEX> & ivolpoly_vert,
 const IVOLDUAL_CUBE_TABLE & ivoldual_table,
 IVOL_VERTEX_ADJACENCY_LIST & vertex_adjacency_list,
 const DUAL_IVOLVERT_ARRAY & ivolv_list,
 COORD_ARRAY & vertex_coord, 
 float elength_limit, 
 int iteration)
{
  float dist;
  const int DIM3(3);
  COORD_TYPE * vcoord = &(vertex_coord.front());

  for (int it = 0; it < iteration; it++) {
    std::vector<int> short_edge_list;

    // Loop over all vertices
    for (int cur = 0; cur < vertex_adjacency_list.NumVertices(); cur++) {

      // Current node coordinates.
      COORD_TYPE *cur_coord = vcoord + cur * DIM3;

      // Loop over adjacent vertices of the current vertex
      for (int  k = 0; k < vertex_adjacency_list.NumAdjacent(cur); k++) {
        
        // Neighbor node coordinates
        int adj = vertex_adjacency_list.AdjacentVertex(cur, k);
        COORD_TYPE *neigh_coord = vcoord + adj * DIM3;

        IJK::compute_distance(DIM3, cur_coord, neigh_coord, dist);

        // Check if minimum distance is valid.
        if (dist < elength_limit) {
          short_edge_list.push_back(cur);
          short_edge_list.push_back(adj);
        }
      }
    }

    laplacian_smooth_jacobian
      (ivolpoly_vert, ivoldual_table, vertex_adjacency_list,
       ivolv_list, vertex_coord, short_edge_list);
  }
}

void IVOLDUAL::laplacian_smooth_jacobian
(const std::vector<VERTEX_INDEX> & ivolpoly_vert,
 const IVOLDUAL_CUBE_TABLE & ivoldual_table,
 IVOL_VERTEX_ADJACENCY_LIST & vertex_adjacency_list,
 const DUAL_IVOLVERT_ARRAY & ivolv_list,
 COORD_ARRAY & vertex_coord, 
 float jacobian_limit, 
 int iteration)
 {
  const int DIM3(3);
  const int NUM_VERT_PER_HEXAHEDRON(8); 
  COORD_TYPE * vcoord = &(vertex_coord.front());

  for (int it = 0; it < iteration; it++) {
    std::vector<int> negative_jacobian_list;
    // Find all vertices with negative Jacobian.
    for (int ihex = 0; ihex < ivolpoly_vert.size()/8; ihex++) {
      for (int i = 0; i < 8; i++) {
        // Compute Jacobian at current vertex
        COORD_TYPE jacob;        
        compute_hexahedron_normalized_Jacobian_determinant
          (ivolpoly_vert, ihex, vertex_coord, i, jacob);

        if (jacob < jacobian_limit) {
          negative_jacobian_list.push_back(ivolpoly_vert[ihex * 8 + i]);
        }
      }
    }
    laplacian_smooth_jacobian
      (ivolpoly_vert, ivoldual_table, vertex_adjacency_list,
       ivolv_list, vertex_coord, negative_jacobian_list);
  }
}


void IVOLDUAL::laplacian_smooth_jacobian
(const std::vector<VERTEX_INDEX> & ivolpoly_vert,
 const IVOLDUAL_CUBE_TABLE & ivoldual_table,
 IVOL_VERTEX_ADJACENCY_LIST & vertex_adjacency_list,
 const DUAL_IVOLVERT_ARRAY & ivolv_list,
 COORD_ARRAY & vertex_coord, 
 std::vector<int> negative_jacobian_list)
{
	const int DIM3(3);
  const int NUM_VERT_PER_HEXAHEDRON(8); 
  COORD_TYPE * vcoord = &(vertex_coord.front());

  // Polytopes dual to vertex.
  IJK::POLYMESH_DATA<VERTEX_INDEX,int, 
    IJK::HEX_TRIANGULATION_INFO<char,char>> hex_data;
  hex_data.AddPolytopes(ivolpoly_vert, NUM_VERT_PER_HEXAHEDRON);
  IJK::VERTEX_POLY_INCIDENCE<int,int> vertex_poly_incidence(hex_data);

	for (int cur : negative_jacobian_list) {

    // Current node coordinates.
    COORD_TYPE *cur_coord = vcoord + cur*DIM3;

    // Check if current node is on isosurface.
    const int ivolv_cur = ivolv_list[cur].patch_index;
    const TABLE_INDEX table_cur = ivolv_list[cur].table_index;
    const VERTEX_INDEX cube_cur = ivolv_list[cur].cube_index;
    bool curOnLower = ivoldual_table.OnLowerIsosurface(table_cur, ivolv_cur);
    bool curOnUpper = ivoldual_table.OnUpperIsosurface(table_cur, ivolv_cur);

    // Loop over adjacent vertices of the current vertex
    for (int j = 0; j < vertex_adjacency_list.NumAdjacent(cur); j++) {

      // Neighbor node coordinates
      int adj = vertex_adjacency_list.AdjacentVertex(cur, j);
      COORD_TYPE *neigh_coord = vcoord + adj*DIM3;
      
      // Check if neighbor node is on isosurface.
      const int ivolv_adj = ivolv_list[adj].patch_index;
      const TABLE_INDEX table_adj = ivolv_list[adj].table_index;
      const VERTEX_INDEX cube_adj = ivolv_list[adj].cube_index;
      bool adjOnLower = ivoldual_table.OnLowerIsosurface(table_adj, ivolv_adj);
      bool adjOnUpper = ivoldual_table.OnUpperIsosurface(table_adj, ivolv_adj);

      if (cube_cur == cube_adj) {
      	gradient_move_vertex
	      	(ivolpoly_vert, ivoldual_table, vertex_adjacency_list, vertex_poly_incidence, 
           ivolv_list, vertex_coord,neigh_coord, adj, adjOnLower, adjOnUpper);
	      gradient_move_vertex
	      	(ivolpoly_vert, ivoldual_table, vertex_adjacency_list, vertex_poly_incidence, 
           ivolv_list, vertex_coord,cur_coord, cur, curOnLower, curOnUpper);
      }
    }
  }
}

void IVOLDUAL::gradient_move_vertex
(const std::vector<VERTEX_INDEX> & ivolpoly_vert,
 const IVOLDUAL_CUBE_TABLE & ivoldual_table,
 IVOL_VERTEX_ADJACENCY_LIST & vertex_adjacency_list,
 IJK::VERTEX_POLY_INCIDENCE<int,int> vertex_poly_incidence,
 const DUAL_IVOLVERT_ARRAY & ivolv_list,
 COORD_ARRAY & vertex_coord,
 COORD_TYPE *ver_coord, int ver_index,
 bool flag_onLower,  bool flag_onUpper)
{

	const int DIM3(3);
  const float step_base(0.1);
  const int NUM_VERT_PER_HEXAHEDRON(8); 
  COORD_TYPE * vcoord = &(vertex_coord.front());

  COORD_TYPE target[DIM3];
  float pre_jacobian = -1.0;

  // Optimal position to be moved to.
  for (int d = 0; d < DIM3; d++) {
    target[d] = ver_coord[d];
  }

  // Find the direction along which Jacobian changes most.
  for (int k = 0; k < vertex_adjacency_list.NumAdjacent(ver_index); k++) {

    int adj = vertex_adjacency_list.AdjacentVertex(ver_index, k);
    COORD_TYPE *neigh_coord = vcoord + adj*DIM3;

    const int ivolv_adj = ivolv_list[adj].patch_index;
    const TABLE_INDEX table_adj = ivolv_list[adj].table_index;
    const VERTEX_INDEX cube_adj = ivolv_list[adj].cube_index;
    bool adjOnLower = ivoldual_table.OnLowerIsosurface(table_adj, ivolv_adj);
    bool adjOnUpper = ivoldual_table.OnUpperIsosurface(table_adj, ivolv_adj);

    // Skip if two vertices are not on same surface.
    if ((flag_onLower && !adjOnLower) || (flag_onUpper && !adjOnUpper))
      continue;

    // copy adj coord to a temp_adj_coord
    COORD_TYPE neigh_temp[DIM3];
    
    // Backup Coord
    for (int d = 0; d < DIM3; d++) {
      neigh_temp[d] = ver_coord[d];
    }

    for (int d = 0; d < DIM3; d++) {
      ver_coord[d] = (1.0-step_base)*ver_coord[d] + step_base*neigh_coord[d];
    }

    COORD_TYPE min_jacobian = 1.0;
    for (int ipoly = 0; ipoly < vertex_poly_incidence.NumIncidentPoly(ver_index); ipoly++) {
      const int ihex = vertex_poly_incidence.IncidentPoly(ver_index, ipoly);

      for (int i = 0; i < 8; i++) {
        // Compute Jacobian at current vertex
        COORD_TYPE jacob;        
        compute_hexahedron_normalized_Jacobian_determinant
          (ivolpoly_vert, ihex, vertex_coord, i, jacob);

        min_jacobian = std::min(jacob, min_jacobian);
      }
    }

    if (min_jacobian > pre_jacobian) {
      pre_jacobian = min_jacobian;
      for (int d = 0; d < DIM3; d++) {
        target[d] = neigh_coord[d];
      }
    }
    // Restore neighbor coordinate to backup coord
    for (int d = 0; d < DIM3; d++) {
      ver_coord[d] = neigh_temp[d];
    }
  }

  // Move the vertex along max gradient direction.
  COORD_TYPE step[DIM3], optimal[DIM3];
  pre_jacobian = -1.0;

  for (int d = 0; d < DIM3; d++) {
  	step[d] = (target[d] - ver_coord[d]) * step_base;
  }
  for (int i = 1; step_base * i < 0.5; i++) {
		for (int d = 0; d < DIM3; d++) {
			ver_coord[d] += step[d];
		}
		COORD_TYPE min_jacobian = 1.0;
    for (int ipoly = 0; ipoly < vertex_poly_incidence.NumIncidentPoly(ver_index); ipoly++) {
      const int ihex = vertex_poly_incidence.IncidentPoly(ver_index, ipoly);

      for (int i = 0; i < 8; i++) {
        // Compute Jacobian at current vertex
        COORD_TYPE jacob;        
        compute_hexahedron_normalized_Jacobian_determinant
          (ivolpoly_vert, ihex, vertex_coord, i, jacob);
        min_jacobian = std::min(jacob, min_jacobian);
      }
    }
    if (min_jacobian > pre_jacobian) {
      pre_jacobian = min_jacobian;
      for (int d = 0; d < DIM3; d++) {
        optimal[d] = ver_coord[d];
      }
    }
  }

  // Move the coordinate to the optimal position.
  for (int d = 0; d < DIM3; d++) {
    ver_coord[d] = optimal[d];
  }
}
