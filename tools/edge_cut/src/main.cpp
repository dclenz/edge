/**
 * @file This file is part of EDGE.
 *
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
 * This is the main file of EDGEcut.
 **/
#include "io/logging.hpp"
INITIALIZE_EASYLOGGINGPP
#include "../../../submodules/pugixml/src/pugixml.hpp"
#include "io/Config.h"
#include "io/OptionParser.h"
#include "surf/meshUtils.h"
#include "surf/BdryTrimmer.h"

using namespace edge_cut::surf;

int main( int i_argc, char *i_argv[] ) {
  edge_cut::io::OptionParser l_options( i_argc, i_argv );

  EDGE_LOG_INFO << "##########################################################################";
  EDGE_LOG_INFO << "##############   ##############            ###############  ##############";
  EDGE_LOG_INFO << "##############   ###############         ################   ##############";
  EDGE_LOG_INFO << "#####            #####       #####      ######                       #####";
  EDGE_LOG_INFO << "#####            #####        #####    #####                         #####";
  EDGE_LOG_INFO << "#############    #####         #####  #####                  #############";
  EDGE_LOG_INFO << "#############    #####         #####  #####      #########   #############";
  EDGE_LOG_INFO << "#####            #####         #####  #####      #########           #####";
  EDGE_LOG_INFO << "#####            #####        #####    #####        ######           #####";
  EDGE_LOG_INFO << "#####            #####       #####      #####       #####            #####";
  EDGE_LOG_INFO << "###############  ###############         ###############   ###############";
  EDGE_LOG_INFO << "###############  ##############           #############    ###############";
  EDGE_LOG_INFO << "#######################################################################cut";
  EDGE_LOG_INFO << "";

  EDGE_LOG_INFO << "Generating runtime configuration...";
  edge_cut::io::Config< K > l_config( l_options.m_xmlPath );
  l_config.printConfig();

  EDGE_LOG_INFO << "Lets get started:";

  // Create polyhedral meshes of topography (with whatever sampling we were provided),
  // and of domain boundary
  EDGE_LOG_INFO << "Building initial polyhedral surfaces...";
  Polyhedron l_topoPoly, l_bdryPoly;

  edge_cut::surf::topoPolyMeshFromXYZ( l_topoPoly, l_config.m_topoIn );
  EDGE_LOG_INFO << "  Initial topography mesh:";
  EDGE_LOG_INFO << "    #vertices: " << l_topoPoly.size_of_vertices();
  EDGE_LOG_INFO << "    #faces:    " << l_topoPoly.size_of_facets();

  edge_cut::surf::makeBdry( l_bdryPoly, l_config.m_bBox );
  EDGE_LOG_INFO << "  Initial boundary mesh:";
  EDGE_LOG_INFO << "    #vertices: " << l_bdryPoly.size_of_vertices();
  EDGE_LOG_INFO << "    #faces:    " << l_bdryPoly.size_of_facets();


  // Create polyhedral domains for re-meshing
  std::vector< Polyhedron* > l_topoVector( 1, &l_topoPoly );
  std::vector< Polyhedron* > l_bdryVector( 1, &l_bdryPoly );
  Mesh_domain l_topoDomain( l_topoVector.begin(), l_topoVector.end() );
  Mesh_domain l_bdryDomain( l_bdryVector.begin(), l_bdryVector.end() );

  // Preserve the intersection between topography and boundary meshes.
  // This ensures that the two meshes coincide on their boundaries.
  EDGE_LOG_INFO << "Computing topography-boundary intersection...";
  std::list< Polyline_type > l_intersectFeatures = edge_cut::surf::getIntersectionFeatures( l_topoPoly, l_config.m_bBox );
  l_bdryDomain.add_features( l_intersectFeatures.begin(), l_intersectFeatures.end() );
  l_topoDomain.add_features( l_intersectFeatures.begin(), l_intersectFeatures.end() );

  // Free memory, a copy is stored in the Mesh_domain object (no move semantics in CGAL yet)
  l_topoPoly.clear();
  l_bdryPoly.clear();


  // NOTE meshes must share common edge refinement criteria in order for borders
  //      to coincide. (recall edge criteria only affects specified 1D features)
  SizingField l_edgeCrit( l_config.m_edgeBase, l_config.m_scale, l_config.m_center, l_config.m_innerRad, l_config.m_outerRad );

  SizingField l_topoFacetCrit(  l_config.m_facetSizeBase, l_config.m_scale, l_config.m_center, l_config.m_innerRad, l_config.m_outerRad );
  SizingField l_bdryFacetCrit(  l_config.m_facetSizeBase, l_config.m_scale, l_config.m_center, 0, 0 );
  SizingField l_topoApproxCrit( l_config.m_facetApproxBase, l_config.m_scale, l_config.m_center, l_config.m_innerRad, l_config.m_outerRad );
  SizingField l_bdryApproxCrit( l_config.m_facetApproxBase, l_config.m_scale, l_config.m_center, 0, 0 );

  Mesh_criteria   l_topoCriteria( CGAL::parameters::edge_size = l_edgeCrit,
                                  CGAL::parameters::facet_size = l_topoFacetCrit,
                                  CGAL::parameters::facet_distance = l_topoApproxCrit,
                                  CGAL::parameters::facet_angle = l_config.m_angleBound );
  Mesh_criteria   l_bdryCriteria( CGAL::parameters::edge_size = l_edgeCrit,
                                  CGAL::parameters::facet_size = l_bdryFacetCrit,
                                  CGAL::parameters::facet_distance = l_bdryApproxCrit,
                                  CGAL::parameters::facet_angle = l_config.m_angleBound );


  // Mesh generation
  EDGE_LOG_INFO << "Re-meshing polyhedral surfaces according to provided criteria...";
  C3t3 l_topoComplex = CGAL::make_mesh_3<C3t3>( l_topoDomain,
                                                l_topoCriteria,
                                                l_config.m_lloydOpts,
                                                l_config.m_odtOpts,
                                                l_config.m_perturbOpts,
                                                l_config.m_exudeOpts    );
  C3t3 l_bdryComplex = CGAL::make_mesh_3<C3t3>( l_bdryDomain,
                                                l_bdryCriteria,
                                                l_config.m_lloydOpts,
                                                l_config.m_odtOpts,
                                                l_config.m_perturbOpts,
                                                l_config.m_exudeOpts    );
  

  // Trim bits of boundary mesh which extend above topography
  EDGE_LOG_INFO << "Trimming boundary mesh...";
  Polyhedron l_topoPolyMeshed, l_bdryPolyMeshed;
  edge_cut::surf::c3t3ToPolyhedron( l_topoComplex, l_topoPolyMeshed );
  edge_cut::surf::c3t3ToPolyhedron( l_bdryComplex, l_bdryPolyMeshed );
  edge_cut::surf::BdryTrimmer< Polyhedron > l_trimmer( l_bdryPolyMeshed, l_topoPolyMeshed );

  l_trimmer.trim();

  // Output
  EDGE_LOG_INFO << "Writing meshes...";
  std::ofstream l_topoFile( l_config.m_topoOut );
  std::ofstream l_bdryFile( l_config.m_bdryOut );

  l_bdryFile << l_bdryPolyMeshed;
  l_topoFile << l_topoPolyMeshed;
  EDGE_LOG_INFO << "Done.";
  EDGE_LOG_INFO << "Thank you for using EDGEcut!";
}
