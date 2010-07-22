#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Common/Log.hpp"
#include "Common/CRoot.hpp"
#include "Common/ComponentPredicates.hpp"

#include "Mesh/CMesh.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/CElements.hpp"
#include "Mesh/CArray.hpp"
#include "Mesh/ElementNodes.hpp"
#include "Mesh/ElementType.hpp"

using namespace std;
using namespace boost;
using namespace CF;
using namespace CF::Mesh;
using namespace CF::Common;

////////////////////////////////////////////////////////////////////////////////

struct MeshConstruction_Fixture
{
  /// common setup for each test case
  MeshConstruction_Fixture()
  {
     // uncomment if you want to use arguments to the test executable
     //int*    argc = &boost::unit_test::framework::master_test_suite().argc;
     //char*** argv = &boost::unit_test::framework::master_test_suite().argv;
  }

  /// common tear-down for each test case
  ~MeshConstruction_Fixture()
  {
  }

  /// possibly common functions used on the tests below
  
  /// These are handy functions that should maybe be implemented somewhere easily accessible.
  
  /// create a Real vector with 2 coordinates
  RealVector create_coord(const Real& x, const Real& y) {
    RealVector coordVec(2);
    coordVec[XX]=x;
    coordVec[YY]=y;
    return coordVec;
  }
  
  /// create a Uint vector with 4 node ID's
  std::vector<Uint> create_quad(const Uint& A, const Uint& B, const Uint& C, const Uint& D) {
    Uint quad[] = {A,B,C,D};
    std::vector<Uint> quadVec;
    quadVec.assign(quad,quad+4);
    return quadVec;
  }
  
  /// create a Uint vector with 3 node ID's
  std::vector<Uint> create_triag(const Uint& A, const Uint& B, const Uint& C) {
    Uint triag[] = {A,B,C};
    std::vector<Uint> triagVec;
    triagVec.assign(triag,triag+3);
    return triagVec;
  }
  
  /// common values accessed by all tests goes here

};

////////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_SUITE( MeshConstruction_TestSuite, MeshConstruction_Fixture )

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( MeshConstruction )
{
  
  
  // Create root and mesh component
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr mesh ( new CMesh  ( "mesh" ) );

  root->add_component( mesh );
  
  // create a mesh pointer
  CMesh::Ptr p_mesh = boost::dynamic_pointer_cast<CMesh>(mesh);

  // create regions
  CRegion& superRegion = *p_mesh->create_region("superRegion");
  CElements& quadRegion = superRegion.create_elements("Quad2DLagrangeP1");
  CElements& triagRegion = superRegion.create_elements("Triag2DLagrangeP1");

  // create a coordinates array in the mesh component
  CArray& coordinates = *p_mesh->create_array("coordinates");

  // initialize the coordinates array and connectivity tables
  const Uint dim=2;
  coordinates.initialize(dim);
  CTable::Buffer qTableBuffer = quadRegion.connectivity_table().create_buffer();
  CTable::Buffer tTableBuffer = triagRegion.connectivity_table().create_buffer();
  CArray::Buffer coordinatesBuffer = coordinates.create_buffer();
  
  //  Mesh of quads and triangles with node and element numbering:
  //
  //    5----4----6    
  //    |    |\ 3 |
  //    |    | \  |
  //    | 1  |  \ |
  //    |    | 2 \|
  //    3----2----7
  //    |    |\ 1 |
  //    |    | \  |
  //    | 0  |  \ |
  //    |    | 0 \|
  //    0----1----8   
  
  // fill coordinates in the buffer
  coordinatesBuffer.add_row(create_coord( 0.0 , 0.0 ));  // 0
  coordinatesBuffer.add_row(create_coord( 1.0 , 0.0 ));  // 1
  coordinatesBuffer.add_row(create_coord( 1.0 , 1.0 ));  // 2
  coordinatesBuffer.add_row(create_coord( 0.0 , 1.0 ));  // 3
  coordinatesBuffer.add_row(create_coord( 1.0 , 2.0 ));  // 4
  coordinatesBuffer.add_row(create_coord( 0.0 , 2.0 ));  // 5
  coordinatesBuffer.add_row(create_coord( 2.0 , 2.0 ));  // 6
  coordinatesBuffer.add_row(create_coord( 2.0 , 1.0 ));  // 7
  coordinatesBuffer.add_row(create_coord( 2.0 , 0.0 ));  // 8

  
  // fill connectivity in the buffer
  qTableBuffer.add_row(create_quad( 0 , 1 , 2 , 3 ));
  qTableBuffer.add_row(create_quad( 3 , 2 , 4 , 5 ));

  tTableBuffer.add_row(create_triag( 1 , 8 , 2 ));
  tTableBuffer.add_row(create_triag( 8 , 7 , 2 ));
  tTableBuffer.add_row(create_triag( 2 , 7 , 4 ));
  tTableBuffer.add_row(create_triag( 7 , 6 , 4 ));

  // flush buffers into the table. 
  // This causes the table and array to be resized and filled.
  coordinatesBuffer.flush();
  qTableBuffer.flush();
  tTableBuffer.flush();
  
  // check if coordinates match
  Uint elem=1;
  Uint node=2;
  
  CTable::ConstRow nodesRef = triagRegion.connectivity_table().table()[elem];
  CArray::Row coordRef = coordinates[nodesRef[node]];
  BOOST_CHECK_EQUAL(coordRef[0],1.0);
  BOOST_CHECK_EQUAL(coordRef[1],1.0);

 // calculate all volumes of a region
  BOOST_FOREACH( CElements& region, recursive_range_typed<CElements>(superRegion))
  {
   const ElementType& elementType = region.element_type();
   const CTable::ConnectivityTable& connTable = region.connectivity_table().table();
   //CFinfo << "type = " << elementType->getShapeName() << "\n" << CFflush;
   const Uint nbRows = connTable.size();
   std::vector<Real> volumes(nbRows);
   
   // the loop
   for (Uint iElem=0; iElem<nbRows; ++iElem)
   {
     std::vector<RealVector> elementCoordinates;
     fill_node_list(std::inserter(elementCoordinates, elementCoordinates.begin()), coordinates.array(), connTable, iElem);

     volumes[iElem]=elementType.computeVolume(elementCoordinates);
     //CFinfo << "\t volume["<<iElem<<"] =" << volumes[iElem] << "\n" << CFflush;

     // check
     if(elementType.shape() == GeoShape::QUAD)
       BOOST_CHECK_EQUAL(volumes[iElem],1.0);
     if(elementType.shape() == GeoShape::TRIAG)
       BOOST_CHECK_EQUAL(volumes[iElem],0.5);
   }
 }
    

//  BOOST_FOREACH(CArray::Row node , elem_coord)
//  {
//    CFinfo << "node = ";
//    for (Uint j=0; j<node.size(); j++) {
//      CFinfo << node[j] << " ";
//    }
//    CFinfo << "\n" << CFflush;
//  }
      
}


////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////

