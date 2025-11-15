#include <gtest/gtest.h>
#include "StringUtil.h"
// 1a. Define the Fixture Class, inheriting from ::testing::Test
class StringUtilTestFixture : public ::testing::Test {
protected:
    // SetUp() runs immediately before each TEST_F method in this fixture.
    void SetUp() override {
        std::cout << "\n[SetUp] Initializing shared test data..." << std::endl;
        test_data_ = "GTest is Awesome!";
        expected_reversed_ = "!emosewA si tseTG";
    }

    // TearDown() runs immediately after each TEST_F method in this fixture.
    void TearDown() override {
        // Clean up or print final state if necessary
        std::cout << "[TearDown] Cleaning up." << std::endl;
    }

    // Members accessible to all TEST_F's
    std::string test_data_;
    std::string expected_reversed_;
};

// 1b. Use TEST_F(FixtureName, TestName)
TEST_F(StringUtilTestFixture, ReverseBasicString) {
    // Accessing shared state from the fixture
    std::string result = StringUtil::Reverse(test_data_);
    
    // ASSERT_EQ is a critical GTest assertion
    ASSERT_EQ(expected_reversed_, result) 
        << "The reversed string should match the expected value.";
}

TEST_F(StringUtilTestFixture, ReverseEmptyString) {
    std::string empty_string = "";
    ASSERT_EQ("", StringUtil::Reverse(empty_string));
}


// ====================================================================
// 2. TEST_P: Using Parameterized Tests (PalindromeTestP)
//    - Runs the same test logic with different input data.
// ====================================================================

// 2a. Define the parameter structure (input and expected output)
struct PalindromeTestParams {
    std::string input;
    bool expected_is_palindrome;
};

// 2b. Define the Parameterized Fixture Class, inheriting from ::testing::TestWithParam<T>
class PalindromeTestP : public ::testing::TestWithParam<PalindromeTestParams> {
    // No SetUp/TearDown needed, as the parameters handle the state.
};

// 2c. Use TEST_P(FixtureName, TestName)
TEST_P(PalindromeTestP, PalindromeCheck) {
    // Get the parameters for the current test run
    PalindromeTestParams params = GetParam();

    // Call the function under test
    bool result = StringUtil::IsPalindrome(params.input);

    // EXPECT_EQ allows the test to continue if this assertion fails
    EXPECT_EQ(params.expected_is_palindrome, result) 
        << "Test failed for input: \"" << params.input << "\".";
}

// 2d. Instantiate the parameterized test suite (defines the actual test data)
INSTANTIATE_TEST_SUITE_P(
    StringUtilIsPalindrome, // Test Suite Name (Used in test output)
    PalindromeTestP,        // Fixture Class Name
    ::testing::Values(      // Generator function for the test cases
        // Test Cases for Palindromes
        PalindromeTestParams{"racecar", true},
        PalindromeTestParams{"level", true},
        PalindromeTestParams{"A man, a plan, a canal: Panama!", true}, // Complex with spaces/punc.
        PalindromeTestParams{"", true}, 

        // Test Cases for Non-Palindromes
        PalindromeTestParams{"hello", false},
        PalindromeTestParams{"gtest", false},
        PalindromeTestParams{"palindrome test", false}
    )
);


// ====================================================================
// Main function to run all tests
// ====================================================================

// NOTE: This setup assumes the GTest library is linked correctly during compilation.
int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all registered tests
    return RUN_ALL_TESTS();
}