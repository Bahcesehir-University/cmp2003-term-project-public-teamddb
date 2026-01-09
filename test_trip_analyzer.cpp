#include "analyzer.h"
#include "catch_amalgamated.hpp"

#include <fstream>
#include <string>
#include <vector>
#include <cstdio>   // std::remove

// ------------------- helpers -------------------
static void writeFile(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream out(path);
    REQUIRE(out.is_open());
    for (const auto& ln : lines) out << ln << "\n";
}

static bool hasZone(const std::vector<ZoneCount>& v, const std::string& zone, long long count) {
    for (const auto& z : v) if (z.zone == zone && z.count == count) return true;
    return false;
}

static bool hasSlot(const std::vector<SlotCount>& v, const std::string& zone, int hour, long long count) {
    for (const auto& s : v) if (s.zone == zone && s.hour == hour && s.count == count) return true;
    return false;
}

static const char* HDR = "TripID,PickupZoneID,DropoffZoneID,PickupDateTime,DistanceKm,FareAmount";

// ------------------- A: ingestion robustness -------------------

TEST_CASE("A1", "[A1]") {
    TripAnalyzer ta;
    ta.ingestFile("missing_file_hopefully_123.csv");

    REQUIRE(ta.topZones(10).empty());
    REQUIRE(ta.topBusySlots(10).empty());
}

TEST_CASE("A2", "[A2]") {
    const std::string path = "a2.csv";

    // Mix of valid + malformed
    writeFile(path, {
        HDR,
        // valid
        "1,ZONE_A,ZONE_X,2024-01-01 09:15,1.2,10.0",
        // malformed: missing PickupZoneID
        "2,,ZONE_X,2024-01-01 09:15,1.2,10.0",
        // malformed: missing PickupDateTime
        "3,ZONE_A,ZONE_X,,1.2,10.0",
        // malformed: too few columns
        "4,ZONE_A,ZONE_X,2024-01-01 10:00",
        // malformed: bad date string (hour can't be parsed)
        "5,ZONE_B,ZONE_Y,NOT_A_DATE,2.0,12.5",
        // valid
        "6,ZONE_B,ZONE_Y,2024-01-01 23:59,2.0,12.5"
    });

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(10);
    auto topS = ta.topBusySlots(10);

    // Only rows 1 and 6 should count:
    REQUIRE(hasZone(topZ, "ZONE_A", 1));
    REQUIRE(hasZone(topZ, "ZONE_B", 1));

    REQUIRE(hasSlot(topS, "ZONE_A", 9, 1));
    REQUIRE(hasSlot(topS, "ZONE_B", 23, 1));

    std::remove(path.c_str());
}

TEST_CASE("A3", "[A3]") {
    const std::string path = "a3.csv";

    writeFile(path, {
        HDR,
        "1,ZONE_A,ZX,2024-01-01 00:00,1,1",
        "2,ZONE_A,ZX,2024-01-01 23:59,1,1",
        "3,ZONE_A,ZX,2024-01-01 23:00,1,1"
    });

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topS = ta.topBusySlots(10);
    REQUIRE(hasSlot(topS, "ZONE_A", 0, 1));
    REQUIRE(hasSlot(topS, "ZONE_A", 23, 2));

    std::remove(path.c_str());
}

// ------------------- B: correctness + sorting -------------------

TEST_CASE("B1", "[B1]") {
    const std::string path = "b1.csv";

    writeFile(path, {
        HDR,
        "1,ZONE_A,ZX,2024-01-01 10:00,1,1",
        "2,ZONE_A,ZY,2024-01-01 11:00,1,1",
        "3,ZONE_B,ZX,2024-01-01 10:30,1,1",
        "4,ZONE_A,ZZ,2024-01-01 12:00,1,1",
        "5,ZONE_C,ZX,2024-01-01 10:00,1,1"
    });

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(10);
    REQUIRE(hasZone(topZ, "ZONE_A", 3));
    REQUIRE(hasZone(topZ, "ZONE_B", 1));
    REQUIRE(hasZone(topZ, "ZONE_C", 1));

    std::remove(path.c_str());
}

TEST_CASE("B2", "[B2]") {
    const std::string path = "b2.csv";

    // Tie: ZONE_A=2, ZONE_B=2, ensure zone asc for ties.
    writeFile(path, {
        HDR,
        "1,ZONE_B,ZX,2024-01-01 10:00,1,1",
        "2,ZONE_A,ZX,2024-01-01 10:00,1,1",
        "3,ZONE_B,ZX,2024-01-01 11:00,1,1",
        "4,ZONE_A,ZX,2024-01-01 11:00,1,1",
        "5,ZONE_C,ZX,2024-01-01 10:00,1,1"
    });

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(10);
    REQUIRE(topZ.size() >= 3);

    // top two must be (ZONE_A,2) then (ZONE_B,2)
    REQUIRE(topZ[0].count == 2);
    REQUIRE(topZ[1].count == 2);
    REQUIRE(topZ[0].zone == "ZONE_A");
    REQUIRE(topZ[1].zone == "ZONE_B");

    std::remove(path.c_str());
}

TEST_CASE("B3", "[B3]") {
    const std::string path = "b3.csv";

    // Case sensitivity: ZONE01 != zone01
    writeFile(path, {
        HDR,
        "1,ZONE01,ZX,2024-01-01 10:00,1,1",
        "2,zone01,ZX,2024-01-01 10:00,1,1",
        "3,ZONE01,ZX,2024-01-01 10:00,1,1"
    });

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(10);
    REQUIRE(hasZone(topZ, "ZONE01", 2));
    REQUIRE(hasZone(topZ, "zone01", 1));

    std::remove(path.c_str());
}

// ------------------- C: scale / efficiency style tests -------------------
// NOTE: avoid strict timing assertions (unstable across machines).
// These tests validate correctness on large inputs.

TEST_CASE("C1", "[C1]") {
    const std::string path = "c1.csv";

    std::ofstream out(path);
    REQUIRE(out.is_open());
    out << HDR << "\n";

    long long id = 1;
    // 60k ZONE_BIG @ hour 12
    for (int i = 0; i < 60000; ++i, ++id)
        out << id << ",ZONE_BIG,ZX,2024-01-01 12:00,1.0,5.0\n";
    // 30k ZONE_MED @ hour 12
    for (int i = 0; i < 30000; ++i, ++id)
        out << id << ",ZONE_MED,ZX,2024-01-01 12:00,1.0,5.0\n";
    // 10k ZONE_SMALL @ hour 12
    for (int i = 0; i < 10000; ++i, ++id)
        out << id << ",ZONE_SMALL,ZX,2024-01-01 12:00,1.0,5.0\n";
    out.close();

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(3);
    REQUIRE(topZ.size() == 3);
    REQUIRE(topZ[0].zone == "ZONE_BIG");
    REQUIRE(topZ[0].count == 60000);
    REQUIRE(topZ[1].zone == "ZONE_MED");
    REQUIRE(topZ[1].count == 30000);
    REQUIRE(topZ[2].zone == "ZONE_SMALL");
    REQUIRE(topZ[2].count == 10000);

    auto topS = ta.topBusySlots(1);
    REQUIRE(topS.size() == 1);
    REQUIRE(topS[0].zone == "ZONE_BIG");
    REQUIRE(topS[0].hour == 12);
    REQUIRE(topS[0].count == 60000);

    std::remove(path.c_str());
}

TEST_CASE("C2", "[C2]") {
    const std::string path = "c2.csv";

    // Many unique zones, same hour -> tests map growth / hashing behavior
    std::ofstream out(path);
    REQUIRE(out.is_open());
    out << HDR << "\n";

    long long id = 1;
    // 50k unique-ish zones each 1 trip @ 08
    for (int i = 0; i < 50000; ++i, ++id) {
        out << id << ",ZONE_" << i << ",ZX,2024-01-01 08:00,1.0,5.0\n";
    }
    // Add some repeats to create a clear top
    for (int i = 0; i < 20000; ++i, ++id) {
        out << id << ",ZONE_TOP,ZX,2024-01-01 08:30,1.0,5.0\n";
    }
    out.close();

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topZ = ta.topZones(1);
    REQUIRE(topZ.size() == 1);
    REQUIRE(topZ[0].zone == "ZONE_TOP");
    REQUIRE(topZ[0].count == 20000);

    auto topS = ta.topBusySlots(1);
    REQUIRE(topS.size() == 1);
    REQUIRE(topS[0].zone == "ZONE_TOP");
    REQUIRE(topS[0].hour == 8);
    REQUIRE(topS[0].count == 20000);

    std::remove(path.c_str());
}

TEST_CASE("C3", "[C3]") {
    const std::string path = "c3.csv";

    // Stress busy slots across all 24 hours for one zone, verify tie-breaking by hour
    std::ofstream out(path);
    REQUIRE(out.is_open());
    out << HDR << "\n";

    long long id = 1;
    // For ZONE_TIE, each hour gets exactly 1000 trips.
    // Then topBusySlots(5) should return hours 0,1,2,3,4 (hour asc tie-break).
    for (int h = 0; h < 24; ++h) {
        for (int i = 0; i < 1000; ++i, ++id) {
            // keep HH:MM valid
            char buf[32];
            std::snprintf(buf, sizeof(buf), "2024-01-01 %02d:%02d", h, (i % 60));
            out << id << ",ZONE_TIE,ZX," << buf << ",1.0,5.0\n";
        }
    }
    out.close();

    TripAnalyzer ta;
    ta.ingestFile(path);

    auto topS = ta.topBusySlots(5);
    REQUIRE(topS.size() == 5);

    // All counts equal (1000), same zone => hour asc
    for (int i = 0; i < 5; ++i) {
        REQUIRE(topS[i].zone == "ZONE_TIE");
        REQUIRE(topS[i].count == 1000);
        REQUIRE(topS[i].hour == i);
    }

    std::remove(path.c_str());
}
