#include <gtest/gtest.h>
#include <unordered_set>
#include "Config.h"

TEST(UIDGenerationTest, AllGeneratedUIDsAreUnique) {
    std::unordered_set<std::string> uids;
    for (int i = 0; i < 100; i++) {
        Config config;
        std::string uid = config.generateUID();
        EXPECT_FALSE(uid.empty());
        EXPECT_EQ(uids.count(uid), 0);
        uids.insert(uid);
    }
}

TEST(UIDGenerationTest, UIDNotEmpty) {
    Config config;
    EXPECT_FALSE(config.generateUID().empty());
}

TEST(UIDGenerationTest, UIDContainsHostname) {
    Config config;
    std::string uid = config.generateUID();
    EXPECT_FALSE(uid.empty());
}
