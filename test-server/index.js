const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

app.use(bodyParser.json());

app.get('/', (req, res) => {
  res.send('Hello World!')
})

app.post('/data', (req, res) => {
    console.log(req.body);
    res.send({'status': 'ok'});
});

app.use(function(req, res, next) {
    console.log('Not found');
    var err = new Error('Not Found');
    err.status = 404;
    next(err);
});

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})