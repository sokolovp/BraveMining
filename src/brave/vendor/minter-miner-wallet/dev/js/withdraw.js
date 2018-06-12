document.addEventListener('DOMContentLoaded', function() {
  const withDraw = document.getElementById('withdraw');

  withDraw.addEventListener('click', () => {
    //alert('Your transaction is in progress');
    chrome.runtime.sendMessage({ text: 'Your transaction is in progress' })
  });
});
