// Configuration

var setPebbleToken = 'EECF';
var options = {
  "1":0, // Temperature C|F
  "2":0, // Pivot
};

// OpenWeather 

degToCompas = ["N","NNW","NW","WNW","W","WSW", "SW", "SSW","S","SSE","SE","ESE","E","ENE","NE","NNE"];

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();

  if (options["1"] == 0) {
    // metric
    //console.log("http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=metric");  
    req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=metric", true);
  } else {
    // metric
    //console.log("http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=imperial");  
    req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=imperial", true);
  }
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        // console.log(req.responseText);
        response = JSON.parse(req.responseText);

        var dt, temperature, icon, city, description, sunrise, sunset, remaining;
        if (response) {
          time = new Date(response.dt * 1000);
          dt = time.toLocaleTimeString().substr(0,5);
          temperature = Math.round(response.main.temp);
          icon = response.weather[0].icon;
          city = response.name;
          description = response.weather[0].description;
          time = new Date(response.sys.sunrise * 1000);
          sunrise = time.toLocaleTimeString().substr(0,5);
          time = new Date(response.sys.sunset * 1000);
          sunset = time.toLocaleTimeString().substr(0,5);
          remaining = "Pressure: " + Math.round(response.main.pressure) + "hPa\n" +
                      "Humidity: " + Math.round(response.main.humidity) + "%\n" +
                      "Wind: " + Math.round(response.wind.speed) + "mps " + 
                      degToCompas[ Math.round(((response.wind.deg+(360/16)/2)%360)/(360/16)) ] + " " + Math.round(response.wind.deg) + "\u00B0\n";
          if ((response.rain > 0) && (response.snow > 0)) { remaining = remaining + "Precip: " + response.snow + response.rain + "mm mix"; }
          else if (response.snow > 0) { remaining = remaining + "Precip: " + response.snow + "mm snow"; }
          else if (response.rain > 0) { remaining = remaining + "Precip: " + response.rain + "mm rain"; }

          // console.log(dt);
          // console.log(temperature);
          // console.log(icon);
          // console.log(city);
          // console.log(description);
          // console.log(sunrise);
          // console.log(sunset);
          // console.log(remaining); <- Will not display

          if (options["1"] == 0) {
            Pebble.sendAppMessage({
              "icon":icon,
              "temperature":temperature + "\u00B0C",
              "temperature_hid":temperature + "\u00B0C", // We need two buffers as we display the temperature on two screens 
              "city":city,
              "description":description,
              "sunrise": sunrise,
              "sunset": sunset,
              "remaining": remaining,
              "dt":dt,
              "pivot":options["2"],
            });
          } else {
            Pebble.sendAppMessage({
              "icon":icon,
              "temperature":temperature + "\u00B0F",
              "temperature_hid":temperature + "\u00B0F", // We need two buffers as we display the temperature on two screens 
              "city":city,
              "description":description,
              "sunrise": sunrise,
              "sunset": sunset,
              "remaining": remaining,
              "dt":dt,
              "pivot":options["2"],
            });
          }
        }

      } else {
        console.warn("Error");
      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "temperature":"N/A"
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

// Events

Pebble.addEventListener("ready", function(e) {
  // console.log("connect!" + e.ready);
  // console.log(e.type);
  // read settings
  settings = localStorage.getItem(setPebbleToken);
  if ( typeof(settings) == 'string' && settings.length > 5 && settings.charAt(0) == "{" && settings.slice(-1) == "}" ) {
    options = JSON.parse(settings);
    //console.log("Options stored = " + JSON.stringify(options));
  }

  var request = new XMLHttpRequest();
  //console.log('http://x.SetPebble.com/api/' + setPebbleToken + '/' + Pebble.getAccountToken());
  request.open('GET', 'http://x.SetPebble.com/api/' + setPebbleToken + '/' + Pebble.getAccountToken(), true);
  request.onload = function(e) {
    if (request.readyState == 4 && request.status == 200) {
      if ( typeof(request.responseText) == 'string' && request.responseText.length > 5 && request.responseText.charAt(0) == "{" && request.responseText.slice(-1) == "}" ) {
        options = JSON.parse(request.responseText);
        //console.log("Options loaded = " + JSON.stringify(options));
        localStorage.setItem(setPebbleToken, JSON.stringify(options));
      }
    }
    // start watching position
    locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
  }
  request.send(null);
});

Pebble.addEventListener("appmessage", function(e) {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  // console.log(e.type);
  // console.log(e.payload.temperature);
  // console.log("message!");
});

Pebble.addEventListener("showConfiguration", function() {
  //console.log("showing configuration");
  //console.log('http://x.SetPebble.com/' + setPebbleToken + '/' + Pebble.getAccountToken());
  Pebble.openURL('http://x.SetPebble.com/' + setPebbleToken + '/' + Pebble.getAccountToken());
});

Pebble.addEventListener("webviewclosed", function(e) {
  // webview closed
  //console.log("configuration closed");
  //Using primitive JSON validity and non-empty check
  if ( typeof(e.response) == 'string' && e.response.length > 5 && e.response.charAt(0) == "{" && e.response.slice(-1) == "}" ) {
    options = JSON.parse(decodeURIComponent(e.response));
    //console.log("Options set = " + JSON.stringify(options));
    window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions); // update watchface
    localStorage.setItem(setPebbleToken, JSON.stringify(options));
  } else {
    //console.log("Cancelled");
  }});


