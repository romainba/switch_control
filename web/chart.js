const AJAX_FMT = "JSON";

var width;

google.charts.load('current', {'packages':['corechart']});
google.charts.setOnLoadCallback(drawCharts);

function drawCharts() {
    drawChart('chart',
	       'temperature',
	       "Temperature log",
	       '2017-01-01',
	       '2017-12-31',
	       true,
	       'deg. C');
}

function drawChart(elem, type, title, begin, end, isStacked, hTitle) {
    var req = {
	'type'   : type,
	'module' : 0,
	'begin'  : begin,
	'end'    : end,
    };

    console.log("drawing");
    
    $.ajax({
	type: 'POST',
	url: 'chart.php',
	data: req,
	dataType: "json",
	success: function(response) {
	    console.log(response);
	    var options = {
		'title': title,
		'width': width,
		'height': 300,
		'isStacked': isStacked,
		'hAxis': { 'title': hTitle },
		'backgroundColor': { 'fill': 'transparent' },
	    };
	    var data = new google.visualization.arrayToDataTable(response.data);
	    var chart = new google.visualization.ColumnChart(document.getElementById(elem));
            chart.draw(data, options);
	},
	error: function(response) {
	    alert("internal error");
	}
    })
}

$(document).ready(function() {

    width = $("#chart").css("width");
    console.log(width);
    
    $(window).on('resize', function() {
	console.log("resize");
	var w = $("#chart").css("width");
	if (w != width) {
    	    width = w;
	    drawCharts();
	}
    });
})
