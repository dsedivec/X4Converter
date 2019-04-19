#include <boost/test/unit_test.hpp>


#include <X4ConverterTools/model/Part.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <assimp/scene.h>
#include <pugixml.hpp>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <X4ConverterTools/model/Connection.h>
#include "../testUtil.h"

namespace fs = boost::filesystem;
using namespace boost;
using namespace Assimp;
using namespace model;
BOOST_AUTO_TEST_SUITE(UnitTests) // NOLINT(cert-err58-cpp)

    BOOST_AUTO_TEST_SUITE(ConnectionUnitTests) // NOLINT(cert-err58-cpp)

        BOOST_AUTO_TEST_CASE(read_conn_offset) { // NOLINT(cert-err58-cpp)
            const std::string xmlFile =
                    test::TestUtil::GetBasePath() + "/assets/units/size_s/ship_arg_s_fighter_01.xml";
            pugi::xml_document expected;
            pugi::xml_parse_result expectedResult = expected.load_file(xmlFile.c_str());
            BOOST_TEST_REQUIRE(expectedResult.status == pugi::status_ok);
            auto node = expected.select_node(
                    "/components/component/connections/connection[@name='Connection02']").node();
            BOOST_TEST_REQUIRE(!node.empty());

            auto conn = new Connection(node);
            auto result = conn->ConvertToAiNode();
            aiMatrix4x4 expectedMatrix(aiVector3D(1, 1, 1), aiQuaternion(0.976296, -0, -0, -0.2164396),
                                       aiVector3D(9.411734, -2.738604, -2.866085));
            BOOST_TEST(result->mTransformation.Equal(expectedMatrix));
        }

        BOOST_AUTO_TEST_CASE(no_parent) { // NOLINT(cert-err58-cpp)
            const std::string xmlFile =
                    test::TestUtil::GetBasePath() + "/assets/units/size_s/ship_arg_s_fighter_01.xml";
            pugi::xml_document expected;
            pugi::xml_parse_result expectedResult = expected.load_file(xmlFile.c_str());
            BOOST_TEST_REQUIRE(expectedResult.status == pugi::status_ok);
            auto node = expected.select_node(
                    "/components/component/connections/connection[@name='Connection01']").node();
            BOOST_TEST_REQUIRE(!node.empty());

            auto conn = new Connection(node);
            BOOST_TEST(conn->getParentName() == "ROOT");
        }

        BOOST_AUTO_TEST_CASE(has_parent) { // NOLINT(cert-err58-cpp)
            const std::string xmlFile =
                    test::TestUtil::GetBasePath() + "/assets/units/size_s/ship_arg_s_fighter_01.xml";
            pugi::xml_document expected;
            pugi::xml_parse_result expectedResult = expected.load_file(xmlFile.c_str());
            BOOST_TEST_REQUIRE(expectedResult.status == pugi::status_ok);
            auto node = expected.select_node(
                    "/components/component/connections/connection[@name='Connection02']").node();
            BOOST_TEST_REQUIRE(!node.empty());

            auto conn = new Connection(node);
            BOOST_TEST(conn->getParentName() == "anim_main");
        }

        BOOST_AUTO_TEST_CASE(read_conn_attrs_tags) { // NOLINT(cert-err58-cpp)
            const std::string xmlFile =
                    test::TestUtil::GetBasePath() + "/assets/units/size_s/ship_arg_s_fighter_01.xml";
            pugi::xml_document expected;
            pugi::xml_parse_result expectedResult = expected.load_file(xmlFile.c_str());
            BOOST_TEST_REQUIRE(expectedResult.status == pugi::status_ok);
            auto node = expected.select_node(
                    "/components/component/connections/connection[@name='Connection02']").node();
            BOOST_TEST_REQUIRE(!node.empty());

            auto conn = new Connection(node);
            auto result = conn->ConvertToAiNode();
            BOOST_TEST_REQUIRE(result->mNumChildren == 2);
//        BOOST_TEST(result->mChildren[0]->mName); TODO someday
            test::TestUtil::checkAiNodeName(result->mChildren[1],
                                            "Connection02^Tags^part animation iklink nocollision forceoutline detail_xl  ");

        }

    BOOST_AUTO_TEST_SUITE_END() // NOLINT(cert-err58-cpp)
BOOST_AUTO_TEST_SUITE_END() // NOLINT(cert-err58-cpp)
// TODO animations, tags, etc