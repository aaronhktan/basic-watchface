var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
var weather =  require('./weather.js');

Pebble.addEventListener('ready', function (e) {
		console.log('PebbleKit JS ready!');
		weather.getWeather();
	}
);