// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test module for benchmarking proto operators"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

#include "common/Core.hpp"
#include "common/Root.hpp"
#include "common/Log.hpp"

#include "math/MatrixTypes.hpp"

#include "mesh/Domain.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Region.hpp"
#include "mesh/Elements.hpp"
#include "mesh/MeshWriter.hpp"
#include "mesh/ElementData.hpp"
#include "mesh/FieldManager.hpp"
#include "mesh/Geometry.hpp"

#include "mesh/Integrators/Gauss.hpp"
#include "mesh/LagrangeP0/Hexa.hpp"

#include "mesh/BlockMesh/BlockData.hpp"

#include "Physics/PhysModel.hpp"

#include "Solver/CModel.hpp"
#include "Solver/CSolver.hpp"
#include "Solver/Tags.hpp"

#include "Solver/Actions/CForAllElements.hpp"
#include "Solver/Actions/CComputeVolume.hpp"

#include "Solver/Actions/Proto/CProtoAction.hpp"
#include "Solver/Actions/Proto/ElementLooper.hpp"
#include "Solver/Actions/Proto/Expression.hpp"
#include "Solver/Actions/Proto/Functions.hpp"
#include "Solver/Actions/Proto/NodeLooper.hpp"
#include "Solver/Actions/Proto/Terminals.hpp"

#include "Tools/MeshGeneration/MeshGeneration.hpp"
#include "Tools/Testing/ProfiledTestFixture.hpp"
#include "Tools/Testing/TimedTestFixture.hpp"

using namespace cf3;
using namespace cf3::Solver;
using namespace cf3::Solver::Actions;
using namespace cf3::Solver::Actions::Proto;
using namespace cf3::mesh;
using namespace cf3::common;

////////////////////////////////////////////////////

struct ProtoBenchmarkFixture :
  public Tools::Testing::ProfiledTestFixture,
  public Tools::Testing::TimedTestFixture
{
  ProtoBenchmarkFixture() :
    root(Core::instance().root()),
    length(12.),
    half_height(0.5),
    width(6.)
  {
  }

  // Setup a model under root
  CModel& setup(const std::string& model_name)
  {
    int argc = boost::unit_test::framework::master_test_suite().argc;
    char** argv = boost::unit_test::framework::master_test_suite().argv;

    cf3_assert(argc == 4);
    const Uint x_segs = boost::lexical_cast<Uint>(argv[1]);
    const Uint y_segs = boost::lexical_cast<Uint>(argv[2]);
    const Uint z_segs = boost::lexical_cast<Uint>(argv[3]);

    CModel& model = Core::instance().root().create_component<CModel>(model_name);
    Physics::PhysModel& phys_model = model.create_physics("CF.Physics.DynamicModel");
    Domain& dom = model.create_domain("Domain");
    CSolver& solver = model.create_solver("CF.Solver.CSimpleSolver");

    Mesh& mesh = dom.create_component<Mesh>("mesh");

    const Real ratio = 0.1;

    BlockMesh::BlockData& blocks = dom.create_component<BlockMesh::BlockData>("blocks");
    Tools::MeshGeneration::create_channel_3d(blocks, length, half_height, width, x_segs, y_segs/2, z_segs, ratio);
    BlockMesh::build_mesh(blocks, mesh);

    mesh.check_sanity();

    // Set up variables
    phys_model.variable_manager().create_descriptor("volume", "CellVolume");

    // Create field
    boost_foreach(Entities& elements, mesh.topology().elements_range())
    {
      elements.create_space("elems_P0","CF.Mesh.LagrangeP0."+elements.element_type().shape_name());
    }
    FieldGroup& elems_P0 = mesh.create_field_group("elems_P0",FieldGroup::Basis::ELEMENT_BASED);
    solver.field_manager().create_field("volume", elems_P0);

    return model;
  }

  struct DirectArrays
  {
    DirectArrays(const Table<Real>& p_coords, const Table<Uint>::ArrayT& p_conn, Field& p_vol_field, const Uint p_offset) :
      coords(p_coords),
      conn(p_conn),
      vol_field(p_vol_field),
      offset(p_offset)
    {
    }

    const Table<Real>& coords;
    const Table<Uint>::ArrayT& conn;
    Field& vol_field;
    const Uint offset;
  };

  Root& root;
  const Real length;
  const Real half_height;
  const Real width;
  typedef boost::mpl::vector2<LagrangeP1::Hexa3D, LagrangeP0::Hexa> ElementsT;

  /// Arrays used by the direct method
  static boost::shared_ptr<DirectArrays> direct_arrays;
};

boost::shared_ptr<ProtoBenchmarkFixture::DirectArrays> ProtoBenchmarkFixture::direct_arrays;


BOOST_FIXTURE_TEST_SUITE( ProtoBenchmarkSuite, ProtoBenchmarkFixture )

//////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SetupProto )
{
  CModel& model = setup("Proto");

  MeshTerm<0, ScalarField> V("CellVolume", "volume");

  model.solver() << create_proto_action("ComputeVolume", elements_expression(ElementsT(), V = volume));

  std::vector<URI> root_regions;
  root_regions.push_back(model.domain().get_child("mesh").as_type<Mesh>().topology().uri());
  model.solver().configure_option_recursively(Solver::Tags::regions(), root_regions);
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SetupDirect )
{
  CModel& model = setup("Direct");
  Mesh& mesh = model.domain().get_child("mesh").as_type<Mesh>();
  Elements& elements = find_component_recursively_with_filter<Elements>(mesh.topology(), IsElementsVolume());
  Field& vol_field = find_component_recursively_with_tag<Field>(mesh, "volume");

  direct_arrays.reset(new DirectArrays
  (
    mesh.geometry().coordinates(),
    elements.node_connectivity().array(),
    vol_field,
    vol_field.field_group().space(elements).elements_begin()
  ));
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SetupVolumeComputer )
{
  CModel& model = setup("VolumeComputer");

  CLoop& elem_loop = model.solver().create_component< CForAllElements >("elem_loop");
  model.solver() << elem_loop;

  std::vector<URI> root_regions;
  root_regions.push_back(model.domain().get_child("mesh").as_type<Mesh>().topology().uri());
  model.solver().configure_option_recursively(Solver::Tags::regions(), root_regions);

  Mesh& mesh = model.domain().get_child("mesh").as_type<Mesh>();
  Field& vol_field = find_component_recursively_with_tag<Field>(mesh, "volume");
  Elements& elements = find_component_recursively_with_filter<Elements>(mesh.topology(), IsElementsVolume());

  CLoopOperation& volume_computer = elem_loop.create_loop_operation("CF.Solver.Actions.CComputeVolume");
  volume_computer.configure_option("volume",vol_field.uri());
  volume_computer.configure_option("elements",elements.uri());
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SimulateProto )
{
  root.get_child("Proto").as_type<CModel>().simulate();
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SimulateDirect )
{
  const Table<Uint>::ArrayT& conn = direct_arrays->conn;
  const Table<Real>& coords = direct_arrays->coords;
  Field& vol_field = direct_arrays->vol_field;

  LagrangeP1::Hexa3D::NodesT nodes;
  const Uint offset = direct_arrays->offset;
  const Uint elems_begin = 0;
  const Uint elems_end = conn.size();
  for(Uint elem = elems_begin; elem != elems_end; ++elem)
  {
    fill(nodes, coords,  conn[elem]);
    vol_field[elem+offset][0] = LagrangeP1::Hexa3D::volume(nodes);
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( SimulateVolumeComputer )
{
  root.get_child("VolumeComputer").as_type<CModel>().simulate();
}

////////////////////////////////////////////////////////////////////////////////

// Check the volume results (uses proto)
BOOST_AUTO_TEST_CASE( CheckResult )
{
  MeshTerm<0, ScalarField> V("CellVolume", "volume");

  const Real wanted_volume = width*length*half_height*2.;

  BOOST_FOREACH(Mesh& mesh, find_components_recursively_with_name<Mesh>(root, "mesh"))
  {
    std::cout << "Checking volume for mesh " << mesh.uri().path() << std::endl;
    Real vol_check = 0;
    for_each_element< ElementsT >(mesh.topology(), vol_check += V);
    BOOST_CHECK_CLOSE(vol_check, wanted_volume, 1e-6);
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////
