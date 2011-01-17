// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_Solver_Actions_CSetFieldValues2_hpp
#define CF_Solver_Actions_CSetFieldValues2_hpp

#include "Mesh/CFieldElements.hpp"
#include "Mesh/CNodes.hpp"

#include "Solver/Actions/CLoopOperation.hpp"

/////////////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Mesh {
  template <typename T> class CTable;
  class CElements;
}
namespace Solver {
namespace Actions {

///////////////////////////////////////////////////////////////////////////////////////

class Solver_Actions_API CSetFieldValues2 : public CLoopOperation
{
public: // typedefs

  /// pointers
  typedef boost::shared_ptr<CSetFieldValues2> Ptr;
  typedef boost::shared_ptr<CSetFieldValues2 const> ConstPtr;

public: // functions
  /// Contructor
  /// @param name of the component
  CSetFieldValues2 ( const std::string& name );

  /// Virtual destructor
  virtual ~CSetFieldValues2() {};

  /// Get the class name
  static std::string type_name () { return "CSetFieldValues2"; }

  /// Set the loop_helper
  void create_loop_helper (Mesh::CElements& geometry_elements );

  /// execute the action
  virtual void execute ();

private: // data
	
  struct LoopHelper
  {
    LoopHelper(Mesh::CElements& geometry_elements, CLoopOperation& op) :
    field_data(geometry_elements.get_field_elements(op.properties()["Field"].value<std::string>()).data()),
    coordinates(geometry_elements.get_field_elements(op.properties()["Field"].value<std::string>()).nodes().coordinates())
    { }
    Mesh::CTable<Real>& field_data;
    Mesh::CTable<Real>& coordinates;
  };
	
  boost::shared_ptr<LoopHelper> m_loop_helper;

};

/////////////////////////////////////////////////////////////////////////////////////

} // Actions
} // Solver
} // CF

/////////////////////////////////////////////////////////////////////////////////////

#endif // CF_Solver_Actions_CSetFieldValues2_hpp