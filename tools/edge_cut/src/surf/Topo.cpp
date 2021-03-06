/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (anbreuer AT ucsd.edu)
 * @author David Lenz (dlenz AT ucsd.edu)
 *
 * @section LICENSE
 * Copyright (c) 2018, Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 * Meshing of topographic data.
 **/

#include "Topo.h"
#include "io/logging.hpp"

#include <fstream>


edge_cut::surf::Topo::Topo( std::string const & i_topoFile )
{
  std::ifstream l_ptFile( i_topoFile, std::ios::in );
  std::istream_iterator< TopoPoint > l_fileBegin( l_ptFile );
  std::istream_iterator< TopoPoint > l_fileEnd;

  m_delTria = new Triangulation( l_fileBegin, l_fileEnd );
}


edge_cut::surf::Topo::~Topo()
{
  delete m_delTria;
}


double edge_cut::surf::Topo::topoDisp( TopoPoint const & i_pt ) const
{
  double l_zTopoDisp = 1;

  Triangulation::Face_handle l_faceHa = m_delTria->locate( i_pt );

  // return if no face qualifies
  if( l_faceHa == NULL ) {
    EDGE_LOG_FATAL << "Error: Got NULL value when locating point on topography";
  }
  // do the 3D intersection otherwise and check the side w.r.t. to the face
  else {
    // set up the face
    CGAL::Triangle_3< K > l_face(
      l_faceHa->vertex(0)->point(),
      l_faceHa->vertex(1)->point(),
      l_faceHa->vertex(2)->point()
    );

    // set up the vertical ray
    CGAL::Direction_3< K > l_dirPos(0,0,1);
    CGAL::Direction_3< K > l_dirNeg(0,0,-1);
    CGAL::Ray_3< K > l_rayPos( i_pt, l_dirPos );
    CGAL::Ray_3< K > l_rayNeg( i_pt, l_dirNeg );

    // compute the intersection (if available)
    auto l_interPos = CGAL::intersection( l_face, l_rayPos );
    auto l_interNeg = CGAL::intersection( l_face, l_rayNeg );
    if ( l_interPos )
      l_zTopoDisp = i_pt.z() - boost::get<CGAL::Point_3< K > >(&*l_interPos)->z();
    else if ( l_interNeg )
      l_zTopoDisp = i_pt.z() - boost::get<CGAL::Point_3< K > >(&*l_interNeg)->z();
  }

  return l_zTopoDisp;
}


edge_cut::surf::TopoPoint edge_cut::surf::Topo::interpolatePt(  double i_x,
                                                                double i_y ) const
{
  TopoPoint l_basePt( i_x, i_y, 0 );
  double l_zDist = topoDisp( l_basePt );

  return TopoPoint( i_x, i_y, -1*l_zDist );
}


bool
edge_cut::surf::Topo::interRay( TopoPoint const & i_pt,
                                     bool         i_positive   ) const
{
  Triangulation::Face_handle l_faceHa;

  // get the possible 2D face
  l_faceHa = m_delTria->locate( i_pt );

  // return if no face qualifies
  if( l_faceHa == NULL ) return false;
  // do the 3D intersection otherwise and check the side w.r.t. to the face
  else {
      // set up the face
      CGAL::Triangle_3< K > l_face(
        l_faceHa->vertex(0)->point(),
        l_faceHa->vertex(1)->point(),
        l_faceHa->vertex(2)->point()
      );

    // set up the vertical ray
    CGAL::Direction_3< K > l_dir(0,0,1);
    if( i_positive == false ) l_dir = -l_dir;
    CGAL::Ray_3< K > l_ray( i_pt, l_dir );

    // compute the intersection (if available)
    auto l_inter = CGAL::intersection( l_face, l_ray );
    if( l_inter ) return true;
  }

  return false;
}


unsigned short
edge_cut::surf::Topo::interSeg( TopoPoint const & i_segPt1,
                                TopoPoint const & i_segPt2,
                                TopoPoint         o_inters [C_MAX_SURF_INTER] ) const
{
  // set up segment
  CGAL::Segment_3< K > l_seg( i_segPt1, i_segPt2 );

  // get intersections in 2D
  Triangulation::Line_face_circulator l_lineWalk = m_delTria->line_walk( i_segPt1, i_segPt2 ), l_lineWalkDone(l_lineWalk);

  unsigned int l_interCount = 0;

  if( l_lineWalk != 0) {
    do {
      // set up the face
      CGAL::Triangle_3< K > l_face(
        (*l_lineWalk).vertex(0)->point(),
        (*l_lineWalk).vertex(1)->point(),
        (*l_lineWalk).vertex(2)->point()
      );

      // compute the intersection (if available)
      auto l_inter = CGAL::intersection( l_face, l_seg );

      // only continue for non-empty intersections
      if( l_inter ) {
        // save intersection (dependent on return type of the intersection)
        if( auto l_pt = boost::get< CGAL::Point_3< K > >( &*l_inter ) ) {
          o_inters[l_interCount] = *l_pt;
        }
        else EDGE_LOG_FATAL << "intersection return type not supported";

        // increase counter
        l_interCount++;
      }
    }
    while(++l_lineWalk != l_lineWalkDone);
  }

  // return number of intersections
  return l_interCount;
}


std::ostream &
edge_cut::surf::Topo::writeTriaToOff( std::ostream & io_os ) const
{
  typedef typename Triangulation::Vertex_handle                Vertex_handle;
  typedef typename Triangulation::Finite_vertices_iterator     Vertex_iterator;
  typedef typename Triangulation::Finite_faces_iterator        Face_iterator;

  io_os << "OFF" << std::endl;

  // outputs the number of vertices and faces
  std::size_t const l_nVerts = m_delTria->number_of_vertices();
  std::size_t const l_nFaces = m_delTria->number_of_faces();
  std::size_t const l_nEdges = 0;                          //Assumption

  io_os << l_nVerts << " " << l_nFaces << " " << l_nEdges << std::endl;

  // write the vertices
  std::map<Vertex_handle, std::size_t> l_idVertMap;

  std::size_t l_vertID = 0;
  for( Vertex_iterator l_vit = m_delTria->finite_vertices_begin();
       l_vit != m_delTria->finite_vertices_end();
       ++l_vit, ++l_vertID )
  {
      io_os << *l_vit << std::endl;
      l_idVertMap[ l_vit ] = l_vertID;
  }
  EDGE_CHECK_EQ( l_vertID, l_nVerts );

  // write the vertex indices of each full_cell
  std::size_t l_faceID = 0;
  for( Face_iterator l_fit = m_delTria->finite_faces_begin();
       l_fit != m_delTria->finite_faces_end();
       ++l_fit, ++l_faceID )
  {
      io_os << 3;
      for( int j = 0; j < 3; ++j )
      {
        io_os << ' ' << l_idVertMap[ l_fit->vertex(j) ];
      }
      io_os << std::endl;
  }
  EDGE_CHECK_EQ( l_faceID, l_nFaces );

  return io_os;
}
