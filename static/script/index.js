document.addEventListener('DOMContentLoaded', () => {
  let html = document.getElementsByTagName('body')[0];
  let newNode = document.createElement('div');
  let p = document.createElement('p');
  p.innerText = 'SCRIPT LOADED';
  newNode.appendChild(p)
  html.appendChild(newNode)
})
