/**
 * @file minimal_test.cpp
 * @brief Minimal test of UUID functionality for prefab system
 *
 * This is a simplified test that verifies UUID parsing works correctly
 * without requiring ANY engine dependencies (no ECS, no components, no reflection).
 *
 * Tests:
 * 1. UUID generation
 * 2. UUID ToString()
 * 3. UUID FromString() (Phase 2 implementation)
 * 4. UUID round-trip (generate -> toString -> fromString -> compare)
 */

#include "uuid/uuid.hpp"
#include <iostream>
#include <cassert>
#include <string>

void TestUUIDGeneration()
{
    std::cout << "=== Testing UUID Generation ===\n";

    // Generate two UUIDs
    UUID<128> uuid1 = UUID<128>::Generate();
    UUID<128> uuid2 = UUID<128>::Generate();

    std::cout << "UUID 1: " << uuid1.ToString() << "\n";
    std::cout << "UUID 2: " << uuid2.ToString() << "\n";

    // They should be different
    assert(uuid1 != uuid2);

    std::cout << "[PASS] UUIDs are unique!\n\n";
}

void TestUUIDToString()
{
    std::cout << "=== Testing UUID ToString() ===\n";

    UUID<128> uuid = UUID<128>::Generate();
    std::string uuidStr = uuid.ToString();

    std::cout << "Generated UUID: " << uuidStr << "\n";

    // Check format: 8-4-4-4-12 (32 hex chars + 4 dashes = 36 chars)
    assert(uuidStr.length() == 36);

    // Check dashes are in the right places
    assert(uuidStr[8] == '-');
    assert(uuidStr[13] == '-');
    assert(uuidStr[18] == '-');
    assert(uuidStr[23] == '-');

    std::cout << "[PASS] UUID format is correct (8-4-4-4-12)!\n\n";
}

void TestUUIDFromString()
{
    std::cout << "=== Testing UUID FromString() ===\n";

    // Test standard UUID format
    std::string testUUID = "550e8400-e29b-41d4-a716-446655440000";
    std::cout << "Parsing: " << testUUID << "\n";

    UUID<128> parsed = UUID<128>::FromString(testUUID);
    std::string result = parsed.ToString();

    std::cout << "Result:  " << result << "\n";

    // They should match
    assert(testUUID == result);

    std::cout << "[PASS] UUID parsing successful!\n\n";
}

void TestUUIDRoundTrip()
{
    std::cout << "=== Testing UUID Round-Trip ===\n";

    // Generate a UUID
    UUID<128> original = UUID<128>::Generate();
    std::string uuidStr = original.ToString();

    std::cout << "Generated UUID: " << uuidStr << "\n";

    // Parse it back
    UUID<128> parsed = UUID<128>::FromString(uuidStr);
    std::string parsedStr = parsed.ToString();

    std::cout << "Parsed UUID:    " << parsedStr << "\n";

    // Verify they match
    assert(original == parsed);
    assert(uuidStr == parsedStr);

    std::cout << "[PASS] UUID round-trip successful!\n\n";
}

void TestUUIDEquality()
{
    std::cout << "=== Testing UUID Equality ===\n";

    UUID<128> uuid1 = UUID<128>::Generate();
    UUID<128> uuid2 = uuid1;  // Copy
    UUID<128> uuid3 = UUID<128>::Generate();

    std::cout << "UUID 1: " << uuid1.ToString() << "\n";
    std::cout << "UUID 2: " << uuid2.ToString() << " (copy of UUID 1)\n";
    std::cout << "UUID 3: " << uuid3.ToString() << " (different)\n";

    // uuid1 should equal uuid2
    assert(uuid1 == uuid2);

    // uuid1 should not equal uuid3
    assert(uuid1 != uuid3);

    std::cout << "[PASS] UUID equality works correctly!\n\n";
}

void TestUUIDParsing_Uppercase()
{
    std::cout << "=== Testing UUID Parsing (Uppercase) ===\n";

    std::string uppercaseUUID = "550E8400-E29B-41D4-A716-446655440000";
    std::cout << "Parsing: " << uppercaseUUID << "\n";

    UUID<128> parsed = UUID<128>::FromString(uppercaseUUID);
    std::string result = parsed.ToString();

    std::cout << "Result:  " << result << "\n";

    // Result is lowercase, but should represent same UUID
    std::string lowercaseExpected = "550e8400-e29b-41d4-a716-446655440000";
    assert(result == lowercaseExpected);

    std::cout << "[PASS] Uppercase UUID parsing works!\n\n";
}

void TestUUIDParsing_NoDashes()
{
    std::cout << "=== Testing UUID Parsing (No Dashes) ===\n";

    std::string noDashesUUID = "550e8400e29b41d4a716446655440000";
    std::cout << "Parsing: " << noDashesUUID << "\n";

    UUID<128> parsed = UUID<128>::FromString(noDashesUUID);
    std::string result = parsed.ToString();

    std::cout << "Result:  " << result << "\n";

    // Result should have dashes
    std::string expectedWithDashes = "550e8400-e29b-41d4-a716-446655440000";
    assert(result == expectedWithDashes);

    std::cout << "[PASS] No-dash UUID parsing works!\n\n";
}

int main()
{
    std::cout << "\n==========================================\n";
    std::cout << "   UUID System - Minimal Test Suite\n";
    std::cout << "   (Phase 2: UUID Parsing)\n";
    std::cout << "==========================================\n\n";

    std::cout << "This test verifies that UUID::FromString() was\n";
    std::cout << "implemented correctly in Phase 2.\n\n";

    try
    {
        TestUUIDGeneration();
        TestUUIDToString();
        TestUUIDFromString();
        TestUUIDRoundTrip();
        TestUUIDEquality();
        TestUUIDParsing_Uppercase();
        TestUUIDParsing_NoDashes();

        std::cout << "==========================================\n";
        std::cout << "  [PASS] All Tests Passed!\n";
        std::cout << "==========================================\n\n";

        std::cout << "What this proves:\n";
        std::cout << "[+] UUID generation works\n";
        std::cout << "[+] UUID ToString() produces correct format\n";
        std::cout << "[+] UUID FromString() parses correctly (Phase 2)\n";
        std::cout << "[+] Round-trip (generate -> string -> parse) preserves data\n";
        std::cout << "[+] UUID equality comparison works\n";
        std::cout << "[+] Both uppercase and lowercase parsing work\n";
        std::cout << "[+] Parsing works with or without dashes\n\n";

        std::cout << "This is the foundation for prefab UUID persistence.\n";
        std::cout << "Prefabs can now be saved with UUIDs and loaded back!\n\n";

        std::cout << "Next steps:\n";
        std::cout << "1. Compile the full engine to test PrefabSystem\n";
        std::cout << "2. Run integration tests with actual prefab files\n";
        std::cout << "3. Test prefab instantiation and syncing\n\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[FAIL] Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
