#ifndef CF_ComputeVolumes_hpp
#define CF_ComputeVolumes_hpp

#include "Mesh/CRegion.hpp"
#include "Mesh/CArray.hpp"

/////////////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Mesh {

struct ComputeVolumes
{
  CElements& region;
  CArray::Ptr ptr_volumes;
  CArray::Array* volumes;

  ComputeVolumes ( CElements& aregion ) : region(aregion)
  {
    // create an array to store the volumes
    CArray::Ptr ptr_volumes = region.get_child("volumes") ?
                              boost::dynamic_pointer_cast<CArray>(region.get_child("volumes")) :
                              region.create_component_type<CArray>("volumes");

    volumes = &ptr_volumes->array();
    volumes->resize( boost::extents[region.connectivity_table().table().size()][1]);
  }

  template < typename EType >
  void execute (  Uint elem, std::vector<RealVector>& nodes )
  {
    (*volumes)[elem][0] = EType::volume( nodes );
  }

};

} // Mesh
} // CF

/////////////////////////////////////////////////////////////////////////////////////

#endif // CF_ComputeVolumes_hpp
