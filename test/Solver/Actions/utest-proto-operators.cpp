// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test module for proto operators"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "Solver/Actions/Proto/ElementLooper.hpp"
#include "Solver/Actions/Proto/Functions.hpp"
#include "Solver/Actions/Proto/NodeLooper.hpp"
#include "Solver/Actions/Proto/Terminals.hpp"

#include "Common/Core.hpp"
#include "Common/CRoot.hpp"
#include "Common/Log.hpp"

#include "Math/MatrixTypes.hpp"

#include "Mesh/CMesh.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/CElements.hpp"
#include "Mesh/CMeshReader.hpp"
#include "Mesh/ElementData.hpp"
#include "Mesh/CNodes.hpp"

#include "Mesh/Integrators/Gauss.hpp"
#include "Mesh/SF/Types.hpp"

#include "Tools/MeshGeneration/MeshGeneration.hpp"
#include "Tools/Testing/TimedTestFixture.hpp"
#include "Tools/Testing/ProfiledTestFixture.hpp"

using namespace CF;
using namespace CF::Solver;
using namespace CF::Solver::Actions;
using namespace CF::Solver::Actions::Proto;
using namespace CF::Mesh;
using namespace CF::Common;

using namespace CF::Math::MathConsts;

using namespace boost;

/// Check close, for testing purposes
inline void check_close(const RealMatrix2& a, const RealMatrix2& b, const Real threshold)
{
  for(Uint i = 0; i != a.rows(); ++i)
    for(Uint j = 0; j != a.cols(); ++j)
      BOOST_CHECK_CLOSE(a(i,j), b(i,j), threshold);
}

static boost::proto::terminal< void(*)(const RealMatrix2&, const RealMatrix2&, Real) >::type const _check_close = {&check_close};

////////////////////////////////////////////////////

/// List of all supported shapefunctions that allow high order integration
typedef boost::mpl::vector3< SF::Line1DLagrangeP1,
                            SF::Quad2DLagrangeP1,
                            SF::Hexa3DLagrangeP1
> HigherIntegrationElements;

typedef boost::mpl::vector5< SF::Line1DLagrangeP1,
                            SF::Triag2DLagrangeP1,
                            SF::Quad2DLagrangeP1,
                            SF::Hexa3DLagrangeP1,
                            SF::Tetra3DLagrangeP1
> VolumeTypes;

BOOST_AUTO_TEST_SUITE( ProtoOperatorsSuite )

//////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( ProtoBasics )
{
  CMesh::Ptr mesh( allocate_component<CMesh>("rect") );
  Tools::MeshGeneration::create_rectangle(*mesh, 5, 5, 5, 5);
  
  RealVector center_coords(2);
  center_coords.setZero();
  
  // Use the volume function
  for_each_element<VolumeTypes>(mesh->topology(), _cout << "Volume = " << volume << ", centroid = " << transpose(coordinates(center_coords)) << "\n");
  std::cout << std::endl; // Can't be in expression
  
  // volume calculation
  Real vol1 = 0.;
  for_each_element<VolumeTypes>(mesh->topology(), vol1 += volume);
  
  CFinfo << "Mesh volume: " << vol1 << CFendl;
  
  // For an all-quad mesh, this is the same... cool or what? TODO: restore this
//   Real vol2 = 0.;
//   for_each_element<VolumeTypes>
//   (
//     mesh->topology(),
//     vol2 += 0.5*((coordinates(2, 0) - coordinates(0, 0)) * (coordinates(3, 1) - coordinates(1, 1))
//               -  (coordinates(2, 1) - coordinates(0, 1)) * (coordinates(3, 0) - coordinates(1, 0)))
//   );
//   BOOST_CHECK_CLOSE(vol1, vol2, 1e-5);
}

// Deactivated, until for_each_element_node is ported from the old proto code
// BOOST_AUTO_TEST_CASE( VertexValence )
// {
//   // Create a 3x3 rectangle
//   CMesh::Ptr mesh( allocate_component<CMesh>("rect") );
//   Tools::MeshGeneration::create_rectangle(*mesh, 5., 5., 2, 2);
//   
//   // Set up a node-based field to store the number of cells that are adjacent to each node
//   const std::vector<std::string> vars(1, "Valence[1]");
//   mesh->create_field("Valences", vars, CField::NODE_BASED);
//   
//   // Set up proto variables
//   MeshTerm<0, ConstNodes> nodes( "Region", find_component_ptr_recursively_with_name<CRegion>(*mesh, "region") ); // Mesh geometry nodes
//   MeshTerm<1, ScalarField > valence("Valences", "Valence"); // Valence field
//   
//   // Count the elements!
//   for_each_element<SF::VolumeTypes>(find_component_recursively_with_name<CRegion>(*mesh, "region")
//                                   , for_each_element_node(nodes, valence[_elem_node]++));
//   
//   // output the result
//   for_each_node(find_component_recursively_with_name<CRegion>(*mesh, "region")
//               , _cout << valence << " ");
//   std::cout << std::endl;
// }

BOOST_AUTO_TEST_CASE( MatrixProducts )
{
  CMesh::Ptr mesh = Core::instance().root()->create_component<CMesh>("line");
  Tools::MeshGeneration::create_line(*mesh, 1., 1);
  
  mesh->create_scalar_field("Temperature", "T", CField2::Basis::POINT_BASED);
  
  MeshTerm<0, ScalarField > temperature("Temperature", "T");
  
  RealVector1 mapped_coords;
  mapped_coords.setZero();
  
  RealMatrix2 exact; exact << 1., -1., -1., 1;
  RealMatrix2 result;
  
  for_each_element< boost::mpl::vector1<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    boost::proto::lit(result) = 0.5 * integral<1>(laplacian_elm(temperature)) * integral<1>(laplacian_elm(temperature)) * laplacian_elm(temperature, mapped_coords)
  );
  
  check_close(result, 8*exact, 1e-10);
  
  for_each_element< boost::mpl::vector1<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    boost::proto::lit(result) = integral<1>(laplacian_elm(temperature)) * 0.5
  );
  
 check_close(result, exact, 1e-10);
  
  for_each_element< boost::mpl::vector1<SF::Line1DLagrangeP1> >
  (
    mesh->topology(),
    boost::proto::lit(result) = laplacian_elm(temperature, mapped_coords) * laplacian_elm(temperature, mapped_coords) * integral<1>(laplacian_elm(temperature))
  );
  
   check_close(result, 8.*exact, 1e-10);
}

BOOST_AUTO_TEST_CASE( RotatingCylinder )
{
  const Real radius = 1.;
  const Uint segments = 1000;
  const Real u = 300.;
  const Real circulation = 975.;
  const Real rho = 1.225;
  
  CMesh::Ptr mesh(allocate_component<CMesh>("circle"));
  Tools::MeshGeneration::create_circle_2d(*mesh, radius, segments);
  
  typedef boost::mpl::vector1< SF::Line2DLagrangeP1> SurfaceTypes;
  
  RealVector2 force;
  force.setZero();
  
  for_each_element<SurfaceTypes>
  (
    mesh->topology(),
    force += integral<1>
    (
      pow<2>
      (
        2. * u * _sin( _atan2(coordinates[1], coordinates[0]) ) + circulation / (2. * pi() * radius)
      )  * 0.5 * rho * normal 
    )
  );

  BOOST_CHECK_CLOSE(force[YY], rho*u*circulation, 0.001); // lift according to theory
  BOOST_CHECK_SMALL(force[XX], 1e-8); // Drag should be zero
}

/// First create a field with the pressure distribution, then integrate it
BOOST_AUTO_TEST_CASE( RotatingCylinderField )
{
  const Real radius = 1.;
  const Uint segments = 1000;
  const Real u = 300.;
  const Real circulation = 975.;
  const Real rho = 1.225;
  
  CMesh::Ptr mesh = Core::instance().root()->create_component<CMesh>("circle");
  Tools::MeshGeneration::create_circle_2d(*mesh, radius, segments);
  
  mesh->create_scalar_field("Pressure", "p", CF::Mesh::CField2::Basis::POINT_BASED);
  
  CRegion::Ptr region = find_component_ptr_recursively_with_name<CRegion>(*mesh, "region");
  
  MeshTerm<1, ScalarField > p("Pressure", "p"); // Pressure field

  typedef boost::mpl::vector1< SF::Line2DLagrangeP1> SurfaceTypes;

  // Set a field with the pressures
  for_each_node
  (
    mesh->topology(),
    p += pow<2>
    (
      2. * u * _sin( _atan2(coordinates[1], coordinates[0]) ) + circulation / (2. * pi() * radius)
    )  * 0.5 * rho
  );
  
  RealVector2 force;
  force.setZero();
  
  RealVector1 mc;
  mc.setZero();
  
  for_each_element<SurfaceTypes>
  (
    mesh->topology(),
    force += integral<1>(p * normal)
  );
    
  BOOST_CHECK_CLOSE(force[YY], rho*u*circulation, 0.001); // lift according to theory
  BOOST_CHECK_SMALL(force[XX], 1e-8); // Drag should be zero
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////
