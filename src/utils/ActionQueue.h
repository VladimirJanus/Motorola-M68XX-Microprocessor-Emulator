/*
 * Copyright (C) 2024 Vladimir Januš
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ACTIONQUEUE_H
#define ACTIONQUEUE_H

#include "src/core/Core.h"

#include <deque>
#include <mutex>

class ActionQueue {
public:
  void addAction(const Core::Action &action) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &info = Core::actionTypeInfo[action.type];
    if (info.canCoalesce) {
      auto it = std::remove_if(queue_.begin(), queue_.end(), [&action](const Core::Action &existingAction) { return existingAction.type == action.type; });
      queue_.erase(it, queue_.end());
    }
    queue_.push_back(action);
  }
  bool hasActions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !queue_.empty();
  }
  Core::Action getNextAction() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      throw std::out_of_range("No actions in the queue");
    }
    Core::Action action = queue_.front();
    queue_.pop_front();
    return action;
  }

private:
  std::deque<Core::Action> queue_;
  mutable std::mutex mutex_;
};

#endif // ACTIONQUEUE_H
