var exports = module.exports = {};
var keys = require('./keys.js');
var temperature = 0; 
var conditions = "Cloudy";
var settings;

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
	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + position.coords.latitude + '&lon=' + position.coords.longitude + '&appid=' + ((settings.owmKey === "") ? keys.owmKey : settings.owmKey);
	xhrRequest(url, 'GET',
						function(responseText) {
							var json = JSON.parse(responseText);
							temperature = Math.round(json.main.temp - 273.15);
							if (settings.temperatureUnit == "1") {
								temperature = (temperature * 9 / 5) + 32;
							}
							conditions = json.weather[0].main;
							sendWeather(temperature, conditions);
						});
}

function getWUWeather(position) {
	var url = 'http://api.wunderground.com/api/' + settings.wuKey + '/conditions/q/' + position.coords.latitude + ',' + position.coords.longitude + '.json';
	xhrRequest(url, 'GET',
						 function(responseText) {
							 var json = JSON.parse(responseText);
							 if (settings.temperatureUnit == "1") {
								 temperature = json.current_observation.temp_f;
							 } else {
								 temperature = json.current_observation.temp_c;
							 }
							 conditions = json.current_observation.weather;
							 sendWeather(temperature, conditions);
						 });
}

function getDSWeather(position) {
	var url = 'https://api.darksky.net/forecast/' + settings.dsKey + '/' + position.coords.latitude + ',' + position.coords.longitude;
	xhrRequest(url, 'GET',
						 function(responseText) {
							 var json = JSON.parse(responseText);
							 if (settings.temperatureUnit == "1") {
								 temperature = json.currently.temperature;
							 } else {
								 temperature = Math.round((json.currently.temperature - 32) * 5 / 9);
							 }
							 conditions = json.currently.summary;
							 sendWeather(temperature, conditions);
						 });
}

function locationSuccess(position) {
	if (localStorage.getItem('clay-settings') !== null) {
		switch (parseInt(settings.weatherProvider)) {
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
	if (localStorage.getItem('clay-settings') !== null) {
		settings = JSON.parse(localStorage.getItem('clay-settings'));
		if (settings.weatherEnabled === true) {
			navigator.geolocation.getCurrentPosition(locationSuccess, locationError, {timeout: 15000, maximumAge: 60000});
		}
	} else {
		navigator.geolocation.getCurrentPosition(locationSuccess, locationError, {timeout: 15000, maximumAge: 60000});
	}
};