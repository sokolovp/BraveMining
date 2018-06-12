// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerNavigatorBudget_h
#define WorkerNavigatorBudget_h

#include "core/workers/WorkerNavigator.h"
#include "platform/Supplementable.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class BudgetService;
class ExecutionContext;
class WorkerNavigator;

// This exposes the budget object on the WorkerNavigator partial interface.
class WorkerNavigatorBudget final
    : public GarbageCollected<WorkerNavigatorBudget>,
      public Supplement<WorkerNavigator> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerNavigatorBudget);
  WTF_MAKE_NONCOPYABLE(WorkerNavigatorBudget);

 public:
  static const char kSupplementName[];

  static WorkerNavigatorBudget& From(WorkerNavigator&);

  static BudgetService* budget(ExecutionContext*, WorkerNavigator&);
  BudgetService* budget(ExecutionContext*);

  virtual void Trace(blink::Visitor*);

 private:
  explicit WorkerNavigatorBudget(WorkerNavigator&);

  Member<BudgetService> budget_;
};

}  // namespace blink

#endif  // WorkerNavigatorBudget_h