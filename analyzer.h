#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>

struct ZoneCount {
	std::string zone;
	long long count;
};

struct SlotCount {
	std::string zone;
	int hour;
	long long count;
};

class TripAnalyzer {
private:
	std::unordered_map<std::string, int> pickupZoneTripCounts;
	std::unordered_map<std::string, std::vector<int>> zoneHourlyTripCounts;
	bool parseRow(const std::string& line, std::string& outZone, int& outHour);
public:
	void ingestFile(const std::string& csvPath);
	std::vector<ZoneCount> topZones(int k = 10) const;
	std::vector<SlotCount> topBusySlots(int k = 10) const;
};
