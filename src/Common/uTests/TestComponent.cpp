#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/range/iterator_range.hpp>
// #include <boost/range/adaptor/filtered.hpp>
#include <boost/iterator.hpp>

#include "Common/Log.hpp"
#include "Common/Component.hpp"
#include "Common/ComponentPredicates.hpp"
#include "Common/CRoot.hpp"
#include "Common/CGroup.hpp"
#include "Common/CLink.hpp"
#include "Common/DemangledTypeID.hpp"
#include "Common/XmlHelpers.hpp"

using namespace std;
using namespace boost;
using namespace CF;
using namespace CF::Common;

////////////////////////////////////////////////////////////////////////////////

struct Component_Fixture
{
  /// common setup for each test case
  Component_Fixture()
  {
    // int*    argc = &boost::unit_test::framework::master_test_suite().argc;
    // char*** argv = &boost::unit_test::framework::master_test_suite().argv;
  }

  /// common tear-down for each test case
  ~Component_Fixture()
  {
  }

  /// possibly common functions used on the tests below

  /// common values accessed by all tests goes here

};

////////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_SUITE( Component_TestSuite, Component_Fixture )

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( constructors )
{
  // constructor with passed path
  CRoot::Ptr root = CRoot::create ( "root" );

  BOOST_CHECK_EQUAL ( root->name() , "root" );
  BOOST_CHECK_EQUAL ( root->path().string() , "/" );
  BOOST_CHECK_EQUAL ( root->full_path().string() , "//root" );

  // constructor with empty path
  CGroup dir1 ( "dir1" );

  BOOST_CHECK_EQUAL ( dir1.name() , "dir1" );
  BOOST_CHECK_EQUAL ( dir1.path().string() , "" );
  BOOST_CHECK_EQUAL ( dir1.full_path().string() , "dir1" );

  // constructor with passed path
  CLink lnk ( "lnk" );

  BOOST_CHECK_EQUAL ( lnk.name() , "lnk" );
  BOOST_CHECK_EQUAL ( lnk.path().string() , "" );
  BOOST_CHECK_EQUAL ( lnk.full_path().string() , "lnk" );
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( add_component )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1 ( new CGroup ( "dir1" ) );
  Component::Ptr dir2 ( new CGroup ( "dir2" ) );

  root->add_component( dir1 );
  dir1->add_component( dir2 );

  BOOST_CHECK_EQUAL ( root->full_path().string() , "//root" );
  BOOST_CHECK_EQUAL ( dir1->full_path().string() , "//root/dir1" );
  BOOST_CHECK_EQUAL ( dir2->full_path().string() , "//root/dir1/dir2" );
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( xml_tree )
{
//  CRoot::Ptr root = CRoot::create ( "root" );
//
//  Component::Ptr dir1 ( new CGroup ( "dir1" ) );
//  Component::Ptr dir2 ( new CGroup ( "dir2" ) );
//
//  root->add_component( dir1 );
//  dir1->add_component( dir2 );
//
//  XMLNode root_node = XMLNode::createXMLTopNode("xml", TRUE);
//
//  root_node.addAttribute("version","1.0");
//  root_node.addAttribute("encoding","UTF-8");
//  root_node.addAttribute("standalone","yes");
//
//  root->xml_tree( root_node );
//
//  XMLSTR xml_str = root_node.createXMLString();
//
////  CFinfo << "xml_str\n" << xml_str << CFflush;
//
//  freeXMLString(xml_str);
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( is_link )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1 ( new CGroup ( "dir1" ) );

  root->add_component( dir1 );

  BOOST_CHECK ( ! root->is_link() );
  BOOST_CHECK ( ! dir1->is_link() );
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( get )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1 ( new CGroup ( "dir1" ) );
  Component::Ptr lnk1 ( new CLink  ( "lnk1" ) );

  // add child components to root
  root->add_component( dir1 );
  root->add_component( lnk1 );

  // point link to the dir1
  boost::shared_ptr<CLink> p_lnk1 = boost::dynamic_pointer_cast<CLink>(lnk1);
  p_lnk1->link_to(dir1);

  // check that the root returns himself
  BOOST_CHECK_EQUAL ( root->get()->name(), "root" );
  BOOST_CHECK_EQUAL ( root->get()->full_path().string(), "//root" );

  // check that the link is sane
  BOOST_CHECK_EQUAL ( lnk1->name(), "lnk1" );
  BOOST_CHECK_EQUAL ( lnk1->full_path().string(), "//root/lnk1" );

  // check that the link returns the dir1
  BOOST_CHECK_EQUAL ( lnk1->get()->name(), "dir1" );
  BOOST_CHECK_EQUAL ( lnk1->get()->full_path().string(), "//root/dir1" );

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( complete_path )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1 ( new CGroup ( "dir1" ) );
  Component::Ptr dir2 ( new CGroup ( "dir2" ) );
  Component::Ptr dir3 ( new CGroup ( "dir3" ) );

  // add child components to root
  root->add_component( dir1 );
  dir1->add_component( dir2 );
  dir2->add_component( dir3 );

  // test absolute & complete path
  CPath p0 ( "//root/dir1" );
  dir2->complete_path( p0 );
  BOOST_CHECK_EQUAL ( p0.string(), "//root/dir1" );

  // test relative
  CPath p10 ( ".." );
  dir2->complete_path( p10 );
  BOOST_CHECK_EQUAL ( p10.string(), "//root/dir1" );

  // test relative
  CPath p11 ( "./" );
  dir2->complete_path( p11 );
  BOOST_CHECK_EQUAL ( p11.string(), "//root/dir1/dir2" );

  // test relative & complete path
  CPath p12 ( "../../dir2" );
  dir3->complete_path( p12 );
  BOOST_CHECK_EQUAL ( p12.string(), "//root/dir1/dir2" );

  // test absolute & incomplete path
  CPath p2 ( "//root/dir1/dir2/../dir2" );
  dir2->complete_path( p2 );
  BOOST_CHECK_EQUAL ( p2.string(), "//root/dir1/dir2" );

  // test absolute & multiple incomplete path
  CPath p3 ( "//root/dir1/../dir2/../dir1/../dir2/dir3" );
  dir2->complete_path( p3 );
  BOOST_CHECK_EQUAL ( p3.string(), "//root/dir2/dir3" );

  // test absolute & multiple incomplete path at end
  CPath p4 ( "//root/dir1/dir2/dir3/../../" );
  dir2->complete_path( p4 );
  BOOST_CHECK_EQUAL ( p4.string(), "//root/dir1" );

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( look_component )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1  ( new CGroup ( "dir1" ) );
  Component::Ptr dir2  ( new CGroup ( "dir2" ) );
  Component::Ptr dir21 ( new CGroup ( "dir21" ) );
  Component::Ptr dir22 ( new CGroup ( "dir22" ) );

  // add child components to root
  root->add_component( dir1 );
  dir1->add_component( dir2 );
  dir2->add_component( dir21 );
  dir2->add_component( dir22 );

  // test relative & complete path
  CPath p0 ( "../dir21" );
  Component::Ptr cp0 = dir22->look_component( p0 );
  BOOST_CHECK_EQUAL ( cp0->full_path().string(), "//root/dir1/dir2/dir21" );

  // test relative & complete path
  CPath p1 ( "//root/dir1" );
  Component::Ptr cp1 = dir22->look_component( p1 );
  BOOST_CHECK_EQUAL ( cp1->full_path().string(), "//root/dir1" );
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( change_parent )
{
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr dir1  ( new CGroup ( "dir1" ) );
  Component::Ptr dir2  ( new CGroup ( "dir2" ) );

  // add child components to root
  root->add_component( dir1 );
  dir1->add_component( dir2 );

  BOOST_CHECK_EQUAL ( dir2->full_path().string(), "//root/dir1/dir2" );

  dir2->change_parent( root );

  BOOST_CHECK_EQUAL ( dir2->full_path().string(), "//root/dir2" );

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( problem )
{
  CRoot::Ptr root = CRoot::create ( "Simulator" );

  Component::Ptr proot = root->look_component("//Simulator");

  BOOST_CHECK_EQUAL ( proot->full_path().string(), "//Simulator" );

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( create_subcomponents )
{
  CRoot::Ptr root = CRoot::create ( "root" );
  Component::Ptr comp1 = root->create_component_type<Component>("comp1");
  comp1->create_component_type<Component>("comp1_1");
  comp1->create_component_type<Component>("comp1_2");

  BOOST_CHECK_EQUAL(root->get_component("comp1")->name(),"comp1");
  BOOST_CHECK_EQUAL(comp1->get_component("comp1_1")->name(),"comp1_1");
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( test_iterator )
{
  Uint counter(0);
  std::map<Uint,std::string> check_with_map;
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr comp1 = root->create_component_type<Component>("comp1");
  check_with_map[counter++]=comp1->name();
  Component::Ptr comp1_1 = comp1->create_component_type<Component>("comp1_1");
  check_with_map[counter++]=comp1_1->name();
  Component::Ptr comp1_2 = comp1->create_component_type<Component>("comp1_2");
  check_with_map[counter++]=comp1_2->name();
  Component::Ptr comp2   = root->create_component_type<Component>("comp2");
  check_with_map[counter++]=comp2->name();
  Component::Ptr comp2_1 = comp2->create_component_type<Component>("comp2_1");
  check_with_map[counter++]=comp2_1->name();
  Component::Ptr comp2_2 = comp2->create_component_type<Component>("comp2_2");
  check_with_map[counter++]=comp2_2->name();
  CGroup::Ptr group1 = root->create_component_type<CGroup>("group1");
  check_with_map[counter++]=group1->name();
  CGroup::Ptr group2 = root->create_component_type<CGroup>("group2");
  check_with_map[counter++]=group2->name();

  counter = 0;
  for (Component::iterator it = root->begin(); it!=root->end(); ++it )
  {
//    CFinfo << "component " << counter << ": " << it->name() << "\n";
    BOOST_CHECK_EQUAL(it->name(),check_with_map[counter++]);
  }

//  CFinfo << "loop ended\n";

  counter = 0;
  BOOST_FOREACH(const Component& comp, (*boost::dynamic_pointer_cast<Component>(root)))
  {
//    CFinfo << "component " << counter << ": " << comp.name() << "\n";
    BOOST_CHECK_EQUAL(comp.name(),check_with_map[counter++]);
  }

  counter = 4;
  BOOST_FOREACH(const Component& comp, (*comp2))
    BOOST_CHECK_EQUAL(comp.name(),check_with_map[counter++]);

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( test_filter_iterator )
{
  Uint counter(0);
  std::map<Uint,std::string> check_with_map;
  CRoot::Ptr root = CRoot::create ( "root" );

  Component::Ptr comp1 = root->create_component_type<Component>("comp1");
  check_with_map[counter++]=comp1->name();
  Component::Ptr comp1_1 = comp1->create_component_type<Component>("comp1_1");
  check_with_map[counter++]=comp1_1->name();
  Component::Ptr comp1_2 = comp1->create_component_type<Component>("comp1_2");
  check_with_map[counter++]=comp1_2->name();
  Component::Ptr comp2   = root->create_component_type<Component>("comp2");
  check_with_map[counter++]=comp2->name();
  Component::Ptr comp2_1 = comp2->create_component_type<Component>("comp2_1");
  check_with_map[counter++]=comp2_1->name();
  Component::Ptr comp2_2 = comp2->create_component_type<Component>("comp2_2");
  check_with_map[counter++]=comp2_2->name();
  CGroup::Ptr group1 = root->create_component_type<CGroup>("group1");
  check_with_map[counter++]=group1->name();
  CGroup::Ptr group2 = root->create_component_type<CGroup>("group2");
  check_with_map[counter++]=group2->name();

  Component::Ptr all = root;
  counter = 0;

  // Check predicates:
  IsComponentName name_is_equal("group1");
  BOOST_CHECK_EQUAL(name_is_equal(group1),true);
  BOOST_CHECK_EQUAL(name_is_equal(group2),false);

  IsComponentTag  tag_is_equal("CGroup");
  BOOST_CHECK_EQUAL(tag_is_equal(comp1),false);
  BOOST_CHECK_EQUAL(tag_is_equal(group1),true);
  BOOST_CHECK_EQUAL(tag_is_equal(group2),true);


  // Iterator tests:
  // ---------------
  // Several ways are available to iterate using a filter or predicate.
  // They are listed from most difficult to use (showing how to do it yourself -> BOILER-PLATE-CODE)
  // to most easy to use (wrappers that make users life easy)

  // 1) Example creating the filter_iterator yourself

  typedef boost::filter_iterator< IsComponentName , Component::iterator > FilterIterator;
  FilterIterator filterIterator(name_is_equal, all->begin(), all->end());
  FilterIterator last_filterIterator(name_is_equal, all->end(), all->end());

  for (; filterIterator != last_filterIterator; ++filterIterator)
    BOOST_CHECK_EQUAL(filterIterator->name(),"group1");

  // 2) Example using BOOST_FOREACH and boost::make_iterator_range
  counter = 6;
  BOOST_FOREACH(const Component& comp,
                boost::make_iterator_range(boost::filter_iterator<IsComponentTag, Component::iterator >(IsComponentTag("CGroup"),all->begin(),all->end()),
                                           boost::filter_iterator<IsComponentTag, Component::iterator >(IsComponentTag("CGroup"),all->end(),all->end())))
    BOOST_CHECK_EQUAL(comp.name(), check_with_map[counter++]);
  BOOST_CHECK_EQUAL(counter,(Uint) 8);

  // 3) Example using BOOST_FOREACH and make_component_range
  counter = 6;
  BOOST_FOREACH(const Component& comp, iterate_recursive(all->begin(), all->end(),IsComponentTag("CGroup")))
    BOOST_CHECK_EQUAL(comp.name(), check_with_map[counter++]);
  BOOST_CHECK_EQUAL(counter,(Uint) 8);

  // 4) Example using BOOST_FOREACH and make_component_range
  counter = 6;
  BOOST_FOREACH(const Component& comp, iterate_recursive(*all,IsComponentTag("CGroup")))
    BOOST_CHECK_EQUAL(comp.name(), check_with_map[counter++]);
  BOOST_CHECK_EQUAL(counter,(Uint) 8);

  // 5) Example using BOOST_FOREACH and make_component_range using default predicate IsComponentTrue
  counter=0;
  BOOST_FOREACH(const Component& comp, iterate_recursive(*all))
    BOOST_CHECK_EQUAL(comp.name(),check_with_map[counter++]);
  BOOST_CHECK_EQUAL(counter,(Uint) 8);

  // 5) Example using BOOST_FOREACH and Component::make_component_range_of_type
  //    This will use a predefined predicate
  counter = 6;
  BOOST_FOREACH(const CGroup& comp, iterate_recursive_by_type<CGroup>(*all))
    BOOST_CHECK_EQUAL(comp.name(), check_with_map[counter++]);

  BOOST_CHECK_EQUAL(counter,(Uint) 8);
  // 6) Example
    typedef boost::iterator_range<boost::filter_iterator<IsComponentTag, Component::iterator > > Component_iterator_range;
    typedef boost::iterator_range<boost::filter_iterator<IsComponentTag, CGroup::iterator    > > Group_iterator_range;

//  counter = 6;
//  BOOST_FOREACH(CGroup& group, make_component_range_of_type<CGroup>(all))
//    BOOST_CHECK_EQUAL(group.name(), check_with_map[counter++]);
//  BOOST_CHECK_EQUAL(counter,(Uint) 8);

  CGroup::Ptr groups(new CGroup("groups"));
  groups->create_component_type<CGroup>("group1");
  groups->create_component_type<CGroup>("group2");

//  BOOST_FOREACH(CGroup& group, make_component_range_of_type<CGroup>(groups))
//      std::cout<< group.name() << std::endl;

//  check_with_map[counter++]=group1->name();
//  CGroup::Ptr group2 = root->create_component_type<CGroup>("group2");
//  check_with_map[counter++]=group2->name();

  // Check that the end iterator matches that of the recursive helper
  Component_iterator<CGroup> all_it = all->end<CGroup>();
  BOOST_CHECK_EQUAL(iterate_recursive_by_type<CGroup>(*all).end() == all_it, true); // no << for iterators

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( create_component_signal )
{
  CRoot::Ptr root = CRoot::create ( "croot" );

  boost::shared_ptr<XmlDoc> doc = XmlOps::create_doc ();


  XmlNode& node = *XmlOps::goto_doc_node( *doc.get() );

  XmlParams params ( node );

  params.add_param<std::string>( "name",  "MyMesh" );
  params.add_param<std::string>( "atype", "CMeshReader" );
  params.add_param<std::string>( "ctype", "CGNS" );

//  XmlOps::print_xml_node( *doc.get() );
//  XmlOps::write_xml_node( *doc.get(),  "test.xml" );

}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////

