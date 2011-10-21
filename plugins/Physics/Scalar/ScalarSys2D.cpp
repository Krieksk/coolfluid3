// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/algorithm/string/predicate.hpp>

#include "common/Builder.hpp"

#include "Physics/Variables.hpp"

#include "ScalarSys2D.hpp"

namespace cf3 {
namespace Physics {
namespace Scalar {

using namespace common;

////////////////////////////////////////////////////////////////////////////////

common::ComponentBuilder < Scalar::ScalarSys2D,
                           Physics::PhysModel,
                           LibScalar >
                           Builder_ScalarSys2D;

ScalarSys2D::ScalarSys2D( const std::string& name ) : Physics::PhysModel(name)
{
}

ScalarSys2D::~ScalarSys2D() {}

boost::shared_ptr< Physics::Variables > ScalarSys2D::create_variables( const std::string type, const std::string name )
{
  Physics::Variables::Ptr vars = boost::algorithm::contains( type, "." ) ?
        build_component_abstract_type< Physics::Variables >( type, name ) :
        build_component_abstract_type< Physics::Variables >( LibScalar::library_namespace() + "." + type, name );

  add_component( vars );

  return vars;
}

////////////////////////////////////////////////////////////////////////////////

} // Scalar
} // Physics
} // cf3
