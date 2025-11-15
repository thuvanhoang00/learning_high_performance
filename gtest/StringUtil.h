#pragma once
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

/**
 * @brief StringUtil class containing utility methods for string manipulation.
 * * In a real project, the class definition (header) and implementation (source) 
 * would typically be in separate files.
 */
class StringUtil {
public:
    /**
     * @brief Reverses a given string.
     * @param s The input string.
     * @return The reversed string.
     */
    static std::string Reverse(const std::string& s) {
        std::string reversed_s = s;
        std::reverse(reversed_s.begin(), reversed_s.end());
        return reversed_s;
    }

    /**
     * @brief Checks if a string is a palindrome (ignoring case and non-alphanumeric characters).
     * @param s The input string.
     * @return True if the string is a palindrome, false otherwise.
     */
    static bool IsPalindrome(const std::string& s) {
        std::string processed_s;
        // 1. Filter out non-alphanumeric characters and convert to lower case
        for (char c : s) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                processed_s += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        // 2. Reverse the processed string
        std::string reversed_processed_s = processed_s;
        std::reverse(reversed_processed_s.begin(), reversed_processed_s.end());

        // 3. Compare
        return processed_s == reversed_processed_s;
    }
};
