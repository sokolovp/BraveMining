// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests the Timeline API instrumentation of an HTML script tag.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('timeline');
  await TestRunner.evaluateInPagePromise(`
      function performActions()
      {
          var iframe = document.createElement("iframe");
          iframe.src = "../resources/timeline-iframe-data.html";
          document.body.appendChild(iframe);
      }
  `);

  UI.panels.timeline._disableCaptureJSProfileSetting.set(true);
  await PerformanceTestRunner.startTimeline();
  TestRunner.evaluateInPage('performActions()');
  await ConsoleTestRunner.waitUntilMessageReceivedPromise();
  await PerformanceTestRunner.stopTimeline();

  PerformanceTestRunner.timelineModel().mainThreadEvents().forEach(event => {
    if (event.name === TimelineModel.TimelineModel.RecordType.EvaluateScript) {
      PerformanceTestRunner.printTraceEventProperties(event);
    } else if (event.name === TimelineModel.TimelineModel.RecordType.TimeStamp) {
      TestRunner.addResult(`----> ${Timeline.TimelineUIUtils.eventTitle(event)}`);
    }
  });
  TestRunner.completeTest();
})();