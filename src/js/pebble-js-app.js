var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var cityLocation = "";

function locationSuccess(pos) {
  // Construct URL
  var url = "";
  if(cityLocation === ""){
	  url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
     	 pos.coords.latitude + "&lon=" + pos.coords.longitude + "&APPID=8c467bea8bafbdf81de33ba4aba6cabb";
  }else{
	  url =	"http://api.openweathermap.org/data/2.5/weather?q=" + cityLocation + "&APPID=8c467bea8bafbdf81de33ba4aba6cabb";
  }

  

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
        
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);

        // Temperature in Kelvin requires adjustment
        var temperature = Math.round(json.main.temp * 9 / 5 - 459.67);
        console.log("Temperature is " + temperature);

        // Assemble dictionary using our keys
        var dictionary = {
            "KEY_TEMPERATURE": temperature
        };

        // Send to Pebble
        Pebble.sendAppMessage(dictionary,
            function(e) {
                console.log("Weather info sent to Pebble successfully!");
            },
            function(e) {
                console.log("Error sending weather info to Pebble!");
            }
        );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  if(cityLocation === ""){
	  navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000}
	  );
  } else {
  	  locationSuccess(cityLocation);
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
	cityLocation = localStorage.getItem(100);
    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);

Pebble.addEventListener('showConfiguration', function(){
	var url = 'http://blackjack26.github.io/FestiveTimeWebpage';
	console.log("Showing configuration page: " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));
	console.log('Configuration page returned: ' + JSON.stringify(configData));

	localStorage.setItem(100, configData.location);
	if(configData.useLocation)
		localStorage.setItem(100, "");
	cityLocation = localStorage.getItem(100);
	getWeather();

	if(configData.temperatureFormat){
		Pebble.sendAppMessage({
			twentyFourHourFormat: configData.twentyFourHourFormat,
			batteryDisplayOnOff: configData.batteryDisplayOnOff,
			temperatureFormat: configData.temperatureFormat,
			birthdayList: configData.birthdayList,
			invertColor: configData.invertColor
		}, function(){
			console.log('Send config successful!');
		}, function(){
			console.log('Send config failed!');
		});		
	}
});

