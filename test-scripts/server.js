const express = require('express');
const bodyParser = require('body-parser');
const app = express();
const port = 3000;

// app.use(bodyParser.json());
app.use(bodyParser.text('*/*'));

app.post('/set-wifi', (req, res) => {
    console.log(req.body);
    res.send({'status': 'ok'});
});

app.use(express.static('public'));

app.use(function(req, res, next) {
    console.log('Not found');
    var err = new Error('Not Found');
    err.status = 404;
    next(err);
});

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
});
