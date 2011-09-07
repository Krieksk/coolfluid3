// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/assign/list_of.hpp>

#include "Common/CBuilder.hpp"

#include "Math/Consts.hpp"

#include "Mesh/ElementTypeT.hpp"
#include "Mesh/ShapeFunctionT.hpp"

#include "Mesh/LagrangeP2/LibLagrangeP2.hpp"
#include "Mesh/LagrangeP2/Triag2D.hpp"
#include "Mesh/LagrangeP2/Line2D.hpp"

namespace CF {
namespace Mesh {
namespace LagrangeP2 {

////////////////////////////////////////////////////////////////////////////////

Common::ComponentBuilder < ElementTypeT<Triag2D>, ElementType , LibLagrangeP2 >
   Triag2D_Builder(LibLagrangeP2::library_namespace()+"."+Triag2D::type_name());

////////////////////////////////////////////////////////////////////////////////

const ShapeFunctionT<Triag2D::SF>& Triag2D::shape_function()
{
  const static ShapeFunctionT<SF> shape_function_obj;
  return shape_function_obj;
}

////////////////////////////////////////////////////////////////////////////////

const CF::Mesh::ElementType::FaceConnectivity& Triag2D::faces()
{
  static ElementType::FaceConnectivity connectivity;
  if(connectivity.displs.empty())
  {
    connectivity.displs = boost::assign::list_of(0)(3)(6);
    connectivity.stride.assign(nb_faces, 3);
    connectivity.nodes = boost::assign::list_of(0)(1)(3)
                                               (1)(2)(4)
                                               (2)(0)(5);
  }
  return connectivity;
}

////////////////////////////////////////////////////////////////////////////////

const CF::Mesh::ElementType& Triag2D::face_type(const CF::Uint face)
{
  static const ElementTypeT<LagrangeP2::Line2D> facetype;
  return facetype;
}

////////////////////////////////////////////////////////////////////////////////

Real Triag2D::area(const NodesT& nodes)
{
  return 0.;
}

////////////////////////////////////////////////////////////////////////////////

} // LagrangeP2
} // Mesh
} // CF