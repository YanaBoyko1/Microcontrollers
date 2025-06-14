#ifndef WEBPAGE_H
#define WEBPAGE_H

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <title>ESP8266 Control Panel</title>
  <style>
    body { font-family: sans-serif; text-align: center; margin-top: 50px; }
    button {
      font-size: 18px;
      padding: 10px 20px;
      margin: 10px;
      border: none;
      border-radius: 8px;
      background-color: #4CAF50;
      color: white;
      cursor: pointer;
    }
    button.partner {
      background-color: #2196F3;
    }
    #status {
      margin-top: 20px;
      font-family: monospace;
      white-space: pre-wrap;
    }
  </style>
  <script>
    function sendCommand(type) {
      fetch(`/control?type=${type}`)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').textContent = data;
        });
    }
    function updateStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').textContent = data;
        });
    }
    setInterval(updateStatus, 2000);
  </script>
</head>
<body>
  <h1>ESP8266 LED Controller</h1>
  <button onclick="sendCommand('client')">Клієнтський алгоритм</button>
  <button class="partner" onclick="sendCommand('partner')">Партнерський алгоритм</button>
  <div id="status">Очікування...</div>
</body>
</html>
)rawliteral";

#endif