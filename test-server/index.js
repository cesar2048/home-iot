const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

app.use(bodyParser.json());

app.get('/', (req, res) => {
  res.send('Hello World!')
})

app.get('/data', (req, res) => {
    console.log(req.body);
    res.send({'status': 'ok'});
})

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})