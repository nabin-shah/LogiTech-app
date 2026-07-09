#include <gtest/gtest.h>
#include "mocks/MockDesktop.h"
#include "mocks/MockInjector.h"
#include "mocks/MockDevice.h"
#include "helpers/TestFixtures.h"

// ---------------------------------------------------------------------------

TEST(Mocks, DesktopCompiles) {
    logitune::test::ensureApp();
    logitune::test::MockDesktop desktop;
    EXPECT_TRUE(desktop.available());
}

TEST(Mocks, InjectorRecordsCalls) {
    logitune::test::MockInjector injector;
    injector.injectKeystroke(QStringLiteral("Ctrl+C"));
    EXPECT_TRUE(injector.hasCalled(QStringLiteral("injectKeystroke")));
    EXPECT_EQ(injector.lastArg(QStringLiteral("injectKeystroke")), QStringLiteral("Ctrl+C"));
}

TEST(Mocks, DeviceSetupMxControls) {
    logitune::test::MockDevice device;
    device.setupMxControls();
    EXPECT_EQ(device.controls().size(), 8);
}
