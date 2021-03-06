#include "common/network/utility.h"

#include "extensions/filters/common/rbac/engine_impl.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/ssl/mocks.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Const;
using testing::Return;
using testing::ReturnRef;
using testing::_;

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace RBAC {
namespace {

void checkEngine(const RBAC::RoleBasedAccessControlEngineImpl& engine, bool expected,
                 const Envoy::Network::Connection& connection = Envoy::Network::MockConnection(),
                 const Envoy::Http::HeaderMap& headers = Envoy::Http::HeaderMapImpl()) {
  EXPECT_EQ(expected, engine.allowed(connection, headers));
}

TEST(RoleBasedAccessControlEngineImpl, Disabled) {
  envoy::config::filter::http::rbac::v2::RBACPerRoute config;
  config.set_disabled(true);
  checkEngine(RBAC::RoleBasedAccessControlEngineImpl(config), true);
  checkEngine(
      RBAC::RoleBasedAccessControlEngineImpl(envoy::config::filter::http::rbac::v2::RBAC(), true),
      true);
}

TEST(RoleBasedAccessControlEngineImpl, AllowedWhitelist) {
  envoy::config::rbac::v2alpha::Policy policy;
  policy.add_permissions()->set_destination_port(123);
  policy.add_principals()->set_any(true);

  envoy::config::filter::http::rbac::v2::RBACPerRoute config;
  envoy::config::rbac::v2alpha::RBAC* rbac = config.mutable_rbac()->mutable_rules();
  rbac->set_action(envoy::config::rbac::v2alpha::RBAC_Action::RBAC_Action_ALLOW);
  (*rbac->mutable_policies())["foo"] = policy;

  RBAC::RoleBasedAccessControlEngineImpl engine(config);

  Envoy::Network::MockConnection conn;
  Envoy::Network::Address::InstanceConstSharedPtr addr =
      Envoy::Network::Utility::parseInternetAddress("1.2.3.4", 123, false);
  EXPECT_CALL(conn, localAddress()).WillOnce(ReturnRef(addr));
  checkEngine(engine, true, conn);

  addr = Envoy::Network::Utility::parseInternetAddress("1.2.3.4", 456, false);
  EXPECT_CALL(conn, localAddress()).WillOnce(ReturnRef(addr));
  checkEngine(engine, false, conn);
}

TEST(RoleBasedAccessControlEngineImpl, DeniedBlacklist) {
  envoy::config::rbac::v2alpha::Policy policy;
  policy.add_permissions()->set_destination_port(123);
  policy.add_principals()->set_any(true);

  envoy::config::filter::http::rbac::v2::RBACPerRoute config;
  envoy::config::rbac::v2alpha::RBAC* rbac = config.mutable_rbac()->mutable_rules();
  rbac->set_action(envoy::config::rbac::v2alpha::RBAC_Action::RBAC_Action_DENY);
  (*rbac->mutable_policies())["foo"] = policy;

  RBAC::RoleBasedAccessControlEngineImpl engine(config);

  Envoy::Network::MockConnection conn;
  Envoy::Network::Address::InstanceConstSharedPtr addr =
      Envoy::Network::Utility::parseInternetAddress("1.2.3.4", 123, false);
  EXPECT_CALL(conn, localAddress()).WillOnce(ReturnRef(addr));
  checkEngine(engine, false, conn);

  addr = Envoy::Network::Utility::parseInternetAddress("1.2.3.4", 456, false);
  EXPECT_CALL(conn, localAddress()).WillOnce(ReturnRef(addr));
  checkEngine(engine, true, conn);
}

} // namespace
} // namespace RBAC
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
