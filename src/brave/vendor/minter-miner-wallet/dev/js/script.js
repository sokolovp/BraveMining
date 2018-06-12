console.log('hello');

// console.log(document.body);
// var div = document.createElement('div');
// div.className = "alert alert-success";
// div.setAttribute("style", "position:absolute; border: 1px solid blue; top: 100px; left: 200px; z-index: 999999999");
// div.innerHTML = "<strong>Ура!</strong> Вы прочитали это важное сообщение.";
//
// document.body.appendChild(div);
//
// console.log(document.body);

function getStorage() {
  chrome.storage.local.get(['currency'], function (result) {
    console.log(result);
  });
}

getStorage();

