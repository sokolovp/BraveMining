document.addEventListener('DOMContentLoaded', function() {
  const deposit = document.getElementById('deposit');

  deposit.addEventListener('click', () => {
    //alert('Your transaction is in progress');
    chrome.runtime.sendMessage({ text: 'Your transaction is in progress' })
  });
});
