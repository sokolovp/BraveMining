// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that computed styles are cached across synchronous requests.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <style>
      #inspected {
          background-color: green;
      }
      </style>
      <div>
        <div id="inspected">Test</div>
      </div>
    `);
  await TestRunner.evaluateInPagePromise(`
      function updateStyle()
      {
          document.getElementById("style").textContent = "#inspected { color: red }";
      }
  `);

  ElementsTestRunner.nodeWithId('inspected', step1);
  var backendCallCount = 0;
  var nodeId;

  function onBackendCall(domain, method, params) {
    if (method === 'CSS.getComputedStyleForNode' && params.nodeId === nodeId)
      ++backendCallCount;
  }

  function step1(node) {
    var callsLeft = 2;
    nodeId = node.id;
    TestRunner.addSniffer(Protocol.TargetBase.prototype, '_wrapCallbackAndSendMessageObject', onBackendCall, true);
    TestRunner.cssModel.computedStylePromise(nodeId).then(styleCallback);
    TestRunner.cssModel.computedStylePromise(nodeId).then(styleCallback);
    function styleCallback() {
      if (--callsLeft)
        return;
      TestRunner.addResult('# of backend calls sent [2 requests]: ' + backendCallCount);
      TestRunner.evaluateInPage('updateStyle()', step2);
    }
  }

  function step2() {
    TestRunner.cssModel.computedStylePromise(nodeId).then(callback);
    function callback() {
      TestRunner.addResult('# of backend calls sent [style update + another request]: ' + backendCallCount);
      TestRunner.completeTest();
    }
  }
})();