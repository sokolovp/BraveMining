
let currency = '';

chrome.runtime.onInstalled.addListener(function() {
  //prompt('Enter login', '');
  // chrome.tabs.create({'url': chrome.extension.getURL('index.html')}, function(tab) {
  //   // Tab opened.
  // });
});

chrome.runtime.onMessage.addListener(function(message, sender, reply) {
  //alert(message.currency);
  alert(message.text)
});