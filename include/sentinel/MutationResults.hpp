/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_

#include <string>
#include "sentinel/Container.hpp"
#include "sentinel/MutationResult.hpp"


namespace sentinel {

/**
 * @brief MutationResults class
 */
class MutationResults : public Container<MutationResult> {
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
