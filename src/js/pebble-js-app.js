degToCompas = ["N","NNW","NW","WNW","W","WSW", "SW", "SSW","S","SSE","SE","ESE","E","ENE","NE","NNE"];

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  // console.log("http://api.openweathermap.org/data/2.5/weather?" +
  //   "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=metric");
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1&units=metric", true);
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
                      "Wind: " + Math.round(response.wind.speed) + "kn " + 
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
          //console.log(remaining); <- Will not display

          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature + "\u00B0C",
            "temperature_hid":temperature + "\u00B0C", // We need two buffers as we display the temperature on two screens 
            "city":city,
            "description":description,
            "sunrise": sunrise,
            "sunset": sunset,
            "remaining": remaining,
            "dt":dt
          });
        }

      } else {
        console.log("Error");
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


Pebble.addEventListener("ready",
                        function(e) {
                          // console.log("connect!" + e.ready);
                          locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                          // console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          // console.log(e.type);
                          // console.log(e.payload.temperature);
                          // console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     // console.log("webview closed");
                                     // console.log(e.type);
                                     // console.log(e.response);
                                     });


