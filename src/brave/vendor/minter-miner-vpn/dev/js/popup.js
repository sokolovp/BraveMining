const loadData = (url) => {
    return new Promise((resolve) => {
        const xhr = new XMLHttpRequest();
        xhr.open('GET', url);
        xhr.responseType = 'json';
        xhr.send();
        xhr.addEventListener('load', () => {
            resolve(xhr.response);
        });
    })
};

const groupBy = function(xs, key) {
    return xs.reduce(function(rv, x) {
        (rv[x[key]] = rv[x[key]] || []).push(x);
        return rv;
    }, {});
};



document.addEventListener('DOMContentLoaded', function() {
  let vpnAddressesData = {
    // 'Albania': ['vpn1', 'vpn2', 'vpn3'],
    // 'Austria': ['vpn1', 'vpn2', 'vpn3'],
    // 'Russia': ['vpn1', 'vpn2', 'vpn3'],
    // 'Brunei': ['vpn1', 'vpn2', 'vpn3'],
    // 'Argentina': ['vpn1', 'vpn2', 'vpn3'],
    // 'Germany': ['vpn1', 'vpn2', 'vpn3'],
    // 'England': ['vpn1', 'vpn2', 'vpn3','vpn4']
  };

  loadData(`http://94.130.229.152:3002/vpn`)
      .then((response) => {
          vpnAddressesData = groupBy(response, 'country');
          console.log(vpnAddressesData);
      }).then(() => {
      const addressList = document.getElementById('vpn-address-list');

      for(var key in vpnAddressesData) {
          let addressGroup = document.createElement('div');
          addressGroup.className = 'vpm-address-group';

          let addressCountry = document.createElement('div');
          addressCountry.innerText = key;
          addressCountry.className = 'vpm-address-country';

          addressGroup.appendChild(addressCountry);

          for(let i = 0; i < vpnAddressesData[key].length; ++i) {
              let addressItem = document.createElement('div');
              addressItem.innerText = vpnAddressesData[key][i].url;
              addressItem.className = 'vpn-address-item';
              addressGroup.appendChild(addressItem);

          }
          addressList.appendChild(addressGroup);
      }
      })
});