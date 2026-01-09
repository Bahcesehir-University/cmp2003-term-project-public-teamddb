[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/kNr72eSq)

# CMP2003 – Trip Analyzer Term Project

## Overview
This project is part of **CMP2003 – Data Structures** and focuses on building a **robust, efficient, and deterministic data analysis tool** for processing trip records stored in CSV format. Students are required to implement a `TripAnalyzer` class that ingests trip data, aggregates statistics, and returns ranked analytical results under strict correctness and performance constraints.

The project is **data-structure agnostic by design**: you are not told which data structures to use. Any correct STL-based solution is acceptable, but inefficient approaches will fail performance-gated tests.

---

## Learning Objectives
By completing this project, students will demonstrate the ability to:

- Parse large CSV files safely without crashing
- Handle malformed (dirty) data robustly
- Design efficient aggregation logic using STL containers
- Implement deterministic multi-key sorting
- Respect performance constraints and asymptotic complexity
- Write code that scales from small test files to millions of rows

---

## Files in This Repository

### 1. `analyzer.h`
Declares the public interface students must implement.

Key structures:
- `ZoneCount`: holds a zone ID and total trip count
- `SlotCount`: holds a zone ID, hour (0–23), and trip count

Key class:
- `TripAnalyzer`
  - `void ingestFile(const std::string& csvPath);`
  - `std::vector<ZoneCount> topZones(int k = 10) const;`
  - `std::vector<SlotCount> topBusySlots(int k = 10) const;`

⚠️ **Do not change function signatures.**

---

### 2. `main.cpp`
A reference **driver program** used for:
- Manual testing
- Measuring execution time
- Producing deterministic output format

It:
1. Creates a `TripAnalyzer`
2. Calls `ingestFile("SmallTrips.csv")`
3. Prints:
   - Top zones
   - Top busy slots
   - Execution time in milliseconds

This file **does not contain grading logic**.

---

### 3. `test_trip_analyzer.cpp`
The **official grading test suite**, written using **Catch2**.

It evaluates:
- Robustness
- Correctness
- Sorting determinism
- Performance under adversarial inputs

This file is **read-only** for students.

---

### 4. `catch_amalgamated.cpp / .hpp`
The Catch2 testing framework (single-header + implementation).

Used for:
- Automated grading
- Local testing

---

### 5. `SmallTrips.csv`
A **small sample dataset** for local development and debugging.

⚠️ The grader will use **different and much larger hidden datasets**.

---

### 6. `Makefile`
Build configuration used by the autograder.

Key properties:
- C++17 standard
- Separate compilation
- Renames `main` if needed during testing

Do not modify unless explicitly instructed.

---

## CSV File Format

Input files follow this schema:

```
TripID,PickupZoneID,PickupTime
```

Example:
```
1,Z1,2024-01-01 10:30
2,Z2,2024-01-01 11:05
```

### Important Notes
- Header row is always present
- Rows may be malformed
- Time format: `YYYY-MM-DD HH:MM`
- Hour is extracted from `PickupTime`
- Zone IDs are **case-sensitive**

---

## Required Functionality

### 1. `ingestFile`
This function must:

- Open the CSV file safely
- Skip the header row
- Parse rows one-by-one
- **Never crash**, even if:
  - Fields are missing
  - Time format is invalid
  - Lines are malformed
- Count:
  - Trips per zone
  - Trips per (zone, hour) pair

❌ Do NOT:
- Assume all rows are valid
- Terminate on bad input
- Use third-party libraries

---

### 2. `topZones(k)`
Returns the top `k` zones sorted by:

1. Trip count (descending)
2. Zone ID (ascending, lexicographical)

If fewer than `k` zones exist, return all.

---

### 3. `topBusySlots(k)`
Returns the top `k` (zone, hour) slots sorted by:

1. Trip count (descending)
2. Zone ID (ascending)
3. Hour (ascending)

---

## Grading Breakdown (70% Skeleton Coverage)

### Category A – Robustness (15%)
- Empty files
- Dirty/malformed rows
- Boundary hour values (00, 23)

### Category B – Deterministic Sorting (20%)
- Tie-breaking correctness
- Case sensitivity
- Stable ordering

### Category C – Performance-Gated Correctness (35%)
These tests **will fail** inefficient solutions.

They include:
- Hundreds of thousands of unique zones
- Millions of rows
- Adversarial distributions designed to kill O(n²) algorithms

⚠️ Passing correctness but failing time limits = **FAIL**

---

## Performance Expectations

To pass all tests:

- Aggregation must be **near O(n)**
- Sorting should be **O(m log m)** where m is number of unique keys
- Avoid:
  - Nested linear searches
  - Re-scanning containers
  - Re-parsing strings unnecessarily

STL containers such as `unordered_map`, `map`, `vector`, and `sort` are allowed.

---

## Determinism Rules

Your output must be **exactly reproducible**:

- Same input → same output order
- No reliance on hash iteration order
- Always apply explicit sorting

---

## Common Reasons for Failure

- Using `vector` + linear search for counting
- Ignoring malformed rows
- Incorrect tie-breaking
- Parsing hour incorrectly
- Assuming clean input
- Passing small tests but timing out on large inputs

---

## Development Tips

- Start with correctness on `SmallTrips.csv`
- Add defensive parsing early
- Test with artificially large inputs
- Measure execution time locally
- Always sort explicitly before returning results

---

## Academic Integrity

- Do not share code
- Do not use AI-generated solutions directly
- Hidden tests will detect:
  - Hardcoded outputs
  - Non-scalable logic
  - Overfitted implementations

---

## Final Notes

This project is intentionally designed to:
- Separate correct-but-slow solutions from efficient ones
- Reward clean design and algorithmic thinking
- Reflect real-world data engineering challenges

Good luck, and write code that scales.