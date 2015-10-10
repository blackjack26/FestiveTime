var $submitButton = $('#submitButton');

$submitButton.on('click', function(){
	console.log('Submit');

	var return_to = getQueryParam('return_to', 'pebblejs://close#');
	document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
});

var $batteryDisplayCheckbox = $('#batteryDisplayCheckbox');
var $timeFormatCheckbox = $('#timeFormatCheckbox');
var $temperatureTab = $('.temp-tab');

console.log('Loaded: ' + JSON.stringify(localStorage));

if(localStorage.twentyFourHourFormat){
	$batteryDisplayCheckbox[0].checked = localStorage.batteryDisplayOnOff === 'true';
	$timeFormatCheckbox[0].checked = localStorage.twentyFourHourFormat === 'true';
	for(var i = 0; i < $temperatureTab.length; i++){
		$($temperatureTab[i]).removeClass("active");
		if($($temperatureTab[i]).html() === localStorage.temperatureFormat){
			$($temperatureTab[i]).addClass("active");
		}
	}
}

function getAndStoreConfigData() {
	var $batteryDisplayCheckbox = $('#batteryDisplayCheckbox');
	var $timeFormatCheckbox = $('#timeFormatCheckbox');
	var $temperatureTab = $('.temp-tab.active');

	var options = {
		twentyFourHourFormat: $timeFormatCheckbox[0].checked,
		batteryDisplayOnOff: $batteryDisplayCheckbox[0].checked,
		temperatureFormat: $temperatureTab.html()
	};

	localStorage.twentyFourHourFormat = options.twentyFourHourFormat;
	localStorage.batteryDisplayOnOff = options.batteryDisplayOnOff;
	localStorage.temperatureFormat = options.temperatureFormat;

	console.log("Got options: " + JSON.stringify(options));
	return options;
}

function getQueryParam(variable, defaultValue){
	var query = location.search.substring(1);
	var vars = query.split('&');
	for(var i = 0; i < vars.length; i++){
		var pair = vars[i].split('=');
		if(pair[0] === variable) {
			return decodeURIComponent(pair[1]);
		}
	}
	return defaultValue || false;
}
