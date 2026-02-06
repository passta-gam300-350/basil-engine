/******************************************************************************/
/*!
\file		PathingTestData.cpp
\project	CS380/CS580 AI Framework
\author		Dustin Holmes
\edited     Saminathan Aaron Nicholas
\summary	A single specific instance of a pathfinding test

Copyright (C) 2018 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#include <pch.h>
#include "PathingTestData.h"

PathingTestData::PathingTestData() :
    map(-1), start { -1, -1 }, goal { -1, -1 }, distCard(-1), distDiag(-1), message(nullptr),
    hasSolution(false), requiresVisualConfirmation(false)
{}

PathingTestData::PathingTestData(const PathingTestData &other) :
    map(other.map), start(other.start), goal(other.goal), distCard(other.distCard), distDiag(other.distDiag), message(other.message), hasSolution(other.hasSolution),
    requiresVisualConfirmation(other.requiresVisualConfirmation)
{}

PathingTestData &PathingTestData::operator=(const PathingTestData &rhs)
{
    map = rhs.map;
    start = rhs.start;
    goal = rhs.goal;
    distCard = 0;
    distDiag = 0;
    message = nullptr;
    hasSolution = rhs.hasSolution;
    requiresVisualConfirmation = rhs.requiresVisualConfirmation;

    return *this;
}

PathingTestData::Outcome PathingTestData::operator==(const PathingTestData &rhs)
{
    Outcome outcome = Outcome::INVALID;

    if (requiresVisualConfirmation == false)
    {
        if (hasSolution == true)
        {
            const bool equivalentPath = std::fabs((distCard + distDiag * std::sqrt(2)) - (rhs.distCard + rhs.distDiag * std::sqrt(2))) < 0.001f;

            if (equivalentPath == true)
            {
                outcome = Outcome::PASSED;
                message = "Passed: Path is equivalent to expected";
            }
            else
            {
                outcome = Outcome::FAILED;
                message = "Failed: Path is not optimal for heuristic and weight";
            }
        }
        else if (message == nullptr)
        {
            outcome = Outcome::PASSED;
            message = "Passed: No path was found to impossible goal";
        }
    }
    else
    {
        outcome = Outcome::SCREEN;
    }

    return outcome;
}

const char *PathingTestData::get_message_text() const
{
    return message;
}

bool PathingTestData::calculate_distance(const WaypointList &path)
{
    distCard = 0;
    distDiag = 0;
    auto p0 = path.begin();
    auto p1 = p0;
    ++p1;

    while (p1 != path.end())
    {
        const auto g0 = terrain->get_grid_position(*p0);
        const auto g1 = terrain->get_grid_position(*p1);

        if (calculate_distance(g0, g1) == false)
        {
            return false;
        }

        p0 = p1;
        ++p1;
    }

    return true;
}

bool PathingTestData::calculate_distance(const GridPos &p0, const GridPos &p1)
{
    bool outcome = true;

    const int rowDiff = std::abs(p0.row - p1.row);
    const int colDiff = std::abs(p0.col - p1.col);

    // if greater than 1 the cells aren't neighbors
    if (rowDiff > 1 || colDiff > 1)
    {
        outcome = false;
        message = "Failed: Two consecutive points in the path are not neighbors";
    }
    // or if the two points are the same
    else if (p0 == p1)
    {
        outcome = false;
        message = "Failed: Two consecutive points in the path are the same";
    }
    else
    {
        (rowDiff == colDiff ? distDiag : distCard)++;
    }

    return outcome;
}
