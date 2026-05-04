#include <gtest/gtest.h>
#include <fstream>
#include "Config.h"

class ConfigLoadTest : public ::testing::Test {
protected:
    void TearDown() override {
        system("rm -f test_config.ini");
    }
};

TEST_F(ConfigLoadTest, ValidIniFile_ReadsCorrectly) {
    std::ofstream file("test_config.ini");
    file << "server_url = http://example.com\npoll_interval = 30\nmax_backoff = 60\nuid = test123";
    file.close();
    
    Config config;
    EXPECT_TRUE(config.load("test_config.ini"));
}

TEST_F(ConfigLoadTest, MissingRequiredFields_ReturnsFalse) {
    std::ofstream file("test_config.ini");
    file << "poll_interval = 30";
    file.close();
    
    Config config;
    EXPECT_FALSE(config.load("test_config.ini"));
}

TEST_F(ConfigLoadTest, EmptyFile_ReturnsFalse) {
    std::ofstream file("test_config.ini");
    file << "";
    file.close();
    
    Config config;
    EXPECT_FALSE(config.load("test_config.ini"));
}

TEST_F(ConfigLoadTest, NonExistentFile_ReturnsFalse) {
    Config config;
    EXPECT_FALSE(config.load("nonexistent.ini"));
}
