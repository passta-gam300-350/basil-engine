/******************************************************************************/
/*!
\file		PathingTestCase.cpp
\project	CS380/CS580 AI Framework
\author		Dustin Holmes
\edited     Saminathan Aaron Nicholas
\summary	A collection of pathfinding tests

Copyright (C) 2018 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/

#include <pch.h>
#include "PathingTestCase.h"

const std::string &PathingTestCase::get_name() const
{
    return name;
}

const PathRequest::Settings &PathingTestCase::get_settings() const
{
    return settings;
}

size_t PathingTestCase::get_num_tests()
{
    return expectedResults.size();
}

PathingTestData &PathingTestCase::pop_queue()
{
    auto &result = expectedResults[tickQueue.front()];
    tickQueue.pop();
    return result;
}
