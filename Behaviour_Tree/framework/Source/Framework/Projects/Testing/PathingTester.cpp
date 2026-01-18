/******************************************************************************/
/*!
\file		PathingTester.cpp
\project	CS380/CS580 AI Framework
\author		Dustin Holmes
\edited     Saminathan Aaron Nicholas
\summary	System for executing and recording test results

Copyright (C) 2018 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#include <pch.h>
#include "PathingTester.h"
#include "Core/Serialization.h"
#include <sstream>
#include "Misc/Stopwatch.h"
#include <iomanip>
#include <fstream>

namespace fs = std::filesystem;

const std::wstring screenshots[] = { L"Diagonal_", L"Smooth_", L"Rubber_", L"Smooth_Rubber_" };

bool PathTester::initialize()
{
    currentTest = -1;

    needScreenshot = -1;
    testsProcessed = 0;

    clearGuard = false;

    const bool result = load_tests();

    return result;
}

const std::wstring &PathTester::get_button_text()
{
    return buttonText;
}

const std::wstring &PathTester::get_status_text()
{
    return statusText;
}

const std::wstring &PathTester::get_failed_text()
{
    return failedText;
}

void PathTester::clear()
{
    if (clearGuard == false)
    {
        statusText = L"";
        currentFailed = 0;
        failedData.clear();
    }
}

void PathTester::bootstrap(const std::string &name, Method method, Heuristic heuristic, float weight)
{
    PathRequest::Settings settings;
    settings.debugColoring = true;
    settings.method = method;
    settings.heuristic = heuristic;
    settings.rubberBanding = false;
    settings.singleStep = false;
    settings.smoothing = false;
    settings.weight = weight;

    const auto smallName = name + "_Small";
    const auto largeName = name + "_Large";

    PathingTestCase smallTest;
    // smallTest.bootstrap(agent, settings, 10, name);
    const std::string smallFilename = smallName + ".txt";
    const auto smallFilepath = Serialization::testsPath / smallFilename;
    Serialization::serialize(smallTest, smallFilepath);

    PathingTestCase largeTest;
    // largeTest.bootstrap(agent, settings, 50, name + "++");
    const std::string largeFilename = largeName + ".txt";
    const auto largeFilepath = Serialization::testsPath / largeFilename;
    Serialization::serialize(largeTest, largeFilepath);
}

void PathTester::bootstrap_speed()
{
    terrain->goto_map(1);

    const int maxRow = terrain->get_map_height() - 1;
    const int maxCol = terrain->get_map_width() - 1;

    static constexpr unsigned numTests = 100;

    std::vector<std::tuple<GridPos, GridPos>> points;
    points.reserve(numTests);

    for (unsigned t = 0; t < numTests; ++t)
    {
        GridPos start;
        GridPos goal;

        while (true)
        {
            start.row = RNG::range(0, maxRow);
            start.col = RNG::range(0, maxCol);

            if (terrain->is_wall(start) == false)
            {
                break;
            }
        }

        while (true)
        {
            goal.row = RNG::range(0, maxRow);
            goal.col = RNG::range(0, maxCol);

            if (goal != start && terrain->is_wall(goal) == false)
            {
                break;
            }
        }

        points.emplace_back(std::make_tuple(start, goal));
    }

    const std::string filename = "Speed.txt";
    const auto filepath = Serialization::testsPath / filename;
    Serialization::serialize(points, filepath);
}

void PathTester::bootstrap()
{
    const unsigned numHeuristics = static_cast<unsigned>(Heuristic::NUM_ENTRIES);

    for (unsigned h = 0; h < numHeuristics; ++h)
    {
        bootstrap(get_heuristic_text(static_cast<Heuristic>(h)), Method::ASTAR, static_cast<Heuristic>(h), 1.0f);
    }

    bootstrap("Dijkstra", Method::ASTAR, Heuristic::EUCLIDEAN, 0.0f);
    bootstrap("RFW", Method::FLOYD_WARSHALL, Heuristic::EUCLIDEAN, 0.0f);
    bootstrap("JPSPlus", Method::JPS_PLUS, Heuristic::EUCLIDEAN, 0.0f);
    bootstrap("Goalbounding", Method::GOAL_BOUNDING, Heuristic::EUCLIDEAN, 0.0f);
    bootstrap_speed();
}

void PathTester::on_test_end()
{
    clearGuard = false;
    Messenger::send_message(Messages::PATH_TEST_END);
    build_status_message(results);

    std::stringstream filename;
    filename << outputName << "_";

    Serialization::generate_time_stamp(filename);
    filename << ".txt";

    const auto filepath = Serialization::outputPath / filename.str();

    Serialization::serialize(results, filepath);

    if (failedData.size() > 0)
    {
        currentFailed = 0;
        recreate_failed_scenario(failedData.front());
        build_status_message(failedData.front());
    }
}

bool PathTester::load_tests()
{
    std::cout << "    Initializing Path Testing System..." << std::endl;

    const fs::directory_iterator dir(Serialization::testsPath);

    for (auto && entry : dir)
    {
        if (fs::is_regular_file(entry) == true)
        {
            // TODO: Find some better way of handling this?
            const fs::path path(entry);
            if (path.filename() == "Speed.txt")
            {
                Serialization::deserialize(speedPaths, path);
            }
            else
            {
                PathingTestCase temp;
                if (Serialization::deserialize(temp, path) == true)
                {
                    tests.emplace_back(std::move(temp));
                }
            }
        }
    }

    Callback clearCB = std::bind(&PathTester::clear, this);
    Messenger::listen_for_message(Messages::PATH_REQUEST_BEGIN, clearCB);
    Messenger::listen_for_message(Messages::MAP_CHANGE, clearCB);

    results.reserve(tests.size());

    return tests.size() > 0;
}

void PathTester::build_status_message(const std::vector<PathingTestResult> &results)
{
    size_t numPassed = 0;
    size_t numFailed = 0;
    size_t numVisual = 0;

    // count the total number of tests run, and their state
    for (const auto & r : results)
    {
        const size_t f = r.num_failing_tests();

        numPassed += r.num_passing_tests();
        numFailed += f;
        numVisual += r.num_visual_tests();
    }

    const size_t total = numPassed + numFailed;

    std::wstringstream stream;

    stream << L"Out of " << total << L" tests: ";

    const unsigned elementCount = static_cast<unsigned>(numPassed > 0) +
        static_cast<unsigned>(numFailed > 0) +
        static_cast<unsigned>(numVisual > 0);
    
    if (numPassed > 0)
    {
        stream << numPassed << L" passed";

        if (elementCount == 3)
        {
            stream << L", ";
        }
        else if (elementCount == 2)
        {
            stream << L" and ";
        }
    }

    if (numFailed > 0)
    {
        stream << numFailed << L" failed";

        if (elementCount == 3)
        {
            stream << L", and ";
        }
        else if (elementCount == 2 && numPassed == 0)
        {
            stream << L" and ";
        }

        // set to -1, so the next click brings it to 0
        currentFailed = -1;
    }

    if (numVisual > 0)
    {
        stream << numVisual << L" additional screenshots were generated for validation";
    }

    statusText = stream.str();
}

void PathTester::build_status_message(const PathingTestData &failed)
{
    const std::string temp(failed.get_message_text());
    failedText = std::wstring(temp.begin(), temp.end());
}

void PathTester::build_status_message()
{
    statusText = std::to_wstring(testsProcessed) + L" of " + std::to_wstring(totalTests) + L" tests have been executed";
}

void PathTester::recreate_failed_scenario(const PathingTestData &failed)
{
    build_status_message(failed);

    // make a copy of the failed test
    PathingTestData local(failed);

    // prevent the map change from wiping our data
    clearGuard = true;
    clearGuard = false;
}

std::filesystem::path PathTester::build_filename(const std::string & title)
{
    std::stringstream filename;

    filename << title << "_";
    Serialization::generate_time_stamp(filename);
    filename << ".txt";

    return Serialization::outputPath / filename.str();
}
