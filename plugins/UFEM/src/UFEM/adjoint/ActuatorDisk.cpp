// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "common/Core.hpp"
#include "common/FindComponents.hpp"
#include "common/Foreach.hpp"
#include "common/Log.hpp"
#include "common/OptionList.hpp"
#include "common/Signal.hpp"
#include "common/Builder.hpp"
#include "common/OptionT.hpp"
#include "common/EventHandler.hpp"

#include "math/LSS/System.hpp"

#include "mesh/Region.hpp"
#include "mesh/LagrangeP0/LibLagrangeP0.hpp"
#include "mesh/LagrangeP0/Quad.hpp"
#include "mesh/LagrangeP0/Line.hpp"

#include "ActuatorDisk.hpp"

#include "solver/actions/Proto/SurfaceIntegration.hpp"
#include "solver/actions/Proto/ProtoAction.hpp"
#include "solver/actions/Proto/Expression.hpp"


namespace cf3
{

namespace UFEM
{

namespace adjoint
{

using namespace solver::actions::Proto;

////////////////////////////////////////////////////////////////////////////////////////////

common::ComponentBuilder < ActuatorDisk, common::Action, LibUFEMAdjoint > ActuatorDisk_Builder;

////////////////////////////////////////////////////////////////////////////////////////////

using boost::proto::lit;

ActuatorDisk::ActuatorDisk(const std::string& name) :
  UnsteadyAction(name),
  rhs(options().add("lss", Handle<math::LSS::System>())
    .pretty_name("LSS")
    .description("The linear system for which the boundary condition is applied")),
  system_matrix(options().option("lss"))
{
  options().add("u_in", m_u_in)
    .pretty_name("Velocityin")
    .description("Inlet velocity")
    .link_to(&m_u_in)
    .mark_basic(); // is this is enabled, the option can be accessed directly from Python, otherwise .options is needed

  options().add("area", m_area)
    .pretty_name("Area")
    .description("Area of the disk")
    .link_to(&m_area)
    .mark_basic(); // is this is enabled, the option can be accessed directly from Python, otherwise .options is needed

  // The component that  will set the force
  create_static_component<ProtoAction>("SetForce")->options().option("regions").add_tag("norecurse");

  // Initialize the expression
  trigger_setup();
}

ActuatorDisk::~ActuatorDisk()
{
}

void ActuatorDisk::on_regions_set()
{
  auto regions = options()["regions"].value<std::vector<common::URI>>();

  // Set the regions when the option is set
  get_child("SetForce")->options().set("regions", std::vector<common::URI>({regions[0]}));
}

void ActuatorDisk::trigger_setup()
{
  Handle<ProtoAction> set_force(get_child("SetForce"));

  FieldVariable<0, VectorField> f("Force", "body_force");
  FieldVariable<1, VectorField> u("Velocity", "navier_stokes_solution");
  set_force->set_expression(nodes_expression
  (
    group
    (
      f[0] = lit(m_f),
      _cout << "disk X velocity: " << u[0] << "\n"
    )
  ));
}

void ActuatorDisk::execute()
{
  FieldVariable<0, VectorField> u("Velocity", "navier_stokes_solution");
  m_u_mean_disk = 0;
  surface_integral(m_u_mean_disk, m_loop_regions, u*normal);
  m_u_mean_disk /= m_area;

  const Real ct = (-0.0000000011324*std::pow(m_u_in, 9))+(0.00000015357*std::pow(m_u_in, 8))+(-0.000009002*std::pow(m_u_in, 7))
   + (0.00029882*std::pow(m_u_in, 6))+(-0.0061814*std::pow(m_u_in, 5))+(0.082595*std::pow(m_u_in, 4))+(-0.71366*m_u_in*m_u_in*m_u_in)+(3.8637*m_u_in*m_u_in)+(-12.101*m_u_in)+17.983;

  m_f = -0.5 * ct * m_u_in*m_u_in / (m_dt * m_u_mean_disk);
  CFinfo << "force set to " << m_f << ", CT: " << ct << "m_u_mean_disk :" << m_u_mean_disk << CFendl;

  Handle<ProtoAction> set_force(get_child("SetForce"));
  set_force->execute();
}

} // namespace adjoint

} // namespace UFEM

} // namespace cf3