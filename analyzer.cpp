#include "analyzer.h"

static bool isValidNum(const std::string &str) {
	if (str.empty())
		return false;

	char *end = nullptr;
	std::strtof(str.c_str(), &end);

	return (end != str.c_str() && *end == '\0');
}

static int parseHour(const std::string &rawDate) {
	if (rawDate.empty())
		return -1;

	size_t spacePos = rawDate.find(' ');
	if (spacePos == std::string::npos)
		return -1;

	size_t timeStart = spacePos + 1;
	if (timeStart >= rawDate.length())
		return -1;

	size_t colonPos = rawDate.find(':', timeStart);
	if (colonPos == std::string::npos)
		return -1;

	std::string hourStr = rawDate.substr(timeStart, colonPos - timeStart);

	char *end = nullptr;
	long hour = std::strtol(hourStr.c_str(), &end, 10);
	if (end == hourStr.c_str())
		return -1;

	return static_cast<int>(hour);
}

bool TripAnalyzer::parseRow(const std::string &line, std::string &pickupZoneId, int &pickupHour) {
	if (line.rfind("TripID", 0) == 0)
		return false;

	std::vector<std::string> keys;
	keys.reserve(6);

	size_t start = 0;
	size_t end = line.find(',');
	while (end != std::string::npos) {
		keys.push_back(line.substr(start, end - start));
		start = end + 1;
		end = line.find(',', start);
	}
	keys.push_back(line.substr(start));
	if (keys.empty())
		return false;

	std::string rawZone;
	std::string rawDate;
	if (keys.size() >= 6) {
		rawZone = keys[1];
		rawDate = keys[3];
		if (!isValidNum(keys[4]) || !isValidNum(keys[5]))
			return false;
	} else if (keys.size() == 3) {
		rawZone = keys[1];
		rawDate = keys[2];
	} else {
		return false;
	}
	if (rawZone.empty() || rawDate.empty())
		return false;

	int hour = parseHour(rawDate);
	if (hour < 0 || hour > 23)
		return false;

	pickupZoneId = rawZone;
	pickupHour = hour;
	return true;
}

void TripAnalyzer::ingestFile(const std::string &csvPath) {
	std::ifstream file(csvPath);
	if (!file.is_open())
		return;

	std::string line;
	line.reserve(256);
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (line.empty())
			continue;

		std::string zone;
		int hour;
		if (parseRow(line, zone, hour)) {
			pickupZoneTripCounts[zone]++;

			std::vector<int> &hours = zoneHourlyTripCounts[zone];
			if (hours.empty())
				hours.resize(24, 0);
			hours[hour]++;
		}
	}
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
	std::vector<ZoneCount> results;
	results.reserve(pickupZoneTripCounts.size());

	for (const auto &pair : pickupZoneTripCounts)
		results.push_back({pair.first, (long long)pair.second});

	auto comparator = [](const ZoneCount& a, const ZoneCount& b) {
		if (a.count != b.count)
			return a.count > b.count;
		return a.zone < b.zone;
	};

	if (results.size() > static_cast<size_t>(k)) {
		std::partial_sort(results.begin(), results.begin() + k, results.end(), comparator);
		results.resize(k);
	} else {
		std::sort(results.begin(), results.end(), comparator);
	}

	return results;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
	std::vector<SlotCount> results;
	results.reserve(zoneHourlyTripCounts.size() * 12);

	for (const auto &pair : zoneHourlyTripCounts) {
		const std::vector<int> &hours = pair.second;
		const std::string &zoneName = pair.first;

		for (int h = 0; h < 24; ++h) {
			if (hours[h] > 0)
				results.push_back({zoneName, h, (long long)hours[h]});
		}
	}

	auto comparator = [](const SlotCount& a, const SlotCount& b) {
		if (a.count != b.count)
			return a.count > b.count;
		if (a.zone != b.zone)
			return a.zone < b.zone;
		return a.hour < b.hour;
	};

	if (results.size() > static_cast<size_t>(k)) {
		std::partial_sort(results.begin(), results.begin() + k, results.end(), comparator);
		results.resize(k);
	} else {
		std::sort(results.begin(), results.end(), comparator);
	}

	return results;
}
