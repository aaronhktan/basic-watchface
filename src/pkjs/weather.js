var exports = module.exports = {};
var keys = require('./keys.js');

var xhrRequest = function(url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function() {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};

function sendWeather(temperature, conditions) {
	var dictionary = {
		'temperature': parseInt(temperature),
		'conditions': String(conditions)
	};
	
	Pebble.sendAppMessage(dictionary, function (e) {
		console.log("Weather info sent!");
	}, function (e) {
		console.log("Weather info not sent!");
	});
}

function getOWMWeather(position) {
	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + position.coords.latitude + '&lon=' + position.coords.longitude + '&appid=' + keys.owmKey;
	xhrRequest(url, 'GET',
						function(responseText) {
							var json = JSON.parse(responseText);
							var temperature = 0; 
							var conditions = "Cloudy";
							temperature = Math.round(json.main.temp - 273.15);
							conditions = json.weather[0].main;
							sendWeather(temperature, conditions);
						});
}

function getWUWeather(position) {
	
}

function getDSWeather(position) {
	
}

function locationSuccess(position) {
	if (localStorage.getItem('clay-settings') !== null) {
		var settings = JSON.parse(localStorage.getItem('clay-settings'));
		switch (settings.weatherProvider) {
			case 1:
				getWUWeather(position);
				break;
			case 2:
				getDSWeather(position);
				break;
			default:
				getOWMWeather(position);
				break;
		}
	} else {
		getOWMWeather(position);
	}
}

function locationError(error) {
	console.log("Error determining position!");
}

exports.getWeather = function() {
	navigator.geolocation.getCurrentPosition(locationSuccess, locationError, {timeout: 15000, maximumAge: 60000});
};