chrome.runtime.onMessageExternal.addListener(
  function(request, sender, sendResponse) {
    // alert('a');
    localStorage.setItem('user', JSON.stringify(request));
  });