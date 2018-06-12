const btn = document.getElementById('test');

btn.addEventListener('click', () => {
  console.log('click')
  chrome.runtime.sendMessage({ currency: 'usd' });
});

$(document).ready(function(){
  $("#myBtn").click(function(){
    $("#myModal").modal();
  });
});